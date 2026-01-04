#include "SkinnedCharacter.h"

// Don't define TINYGLTF_IMPLEMENTATION to avoid linking issues
// tinygltf will be header-only for this usage
#include <tiny_gltf.h>

#include <glm/gtc/type_ptr.hpp>  // For value_ptr

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <unordered_map>

// Constructor
SkinnedCharacter::SkinnedCharacter()
    : m_vao(0), m_vbo_positions(0), m_vbo_normals(0), m_vbo_uvs(0),
      m_vbo_boneIDs(0), m_vbo_weights(0), m_ebo(0), m_indexCount(0),
      m_indexType(GL_UNSIGNED_SHORT), m_animIndex(-1), m_animTime(0.0f),
      m_animDuration(0.0f), m_shaderProgram(0), m_depthShaderProgram(0),
      m_debugLineShader(0), m_position(0.0f, 0.0f, 0.0f), m_yaw(0.0f),
      m_scale(1.0f), m_meshHeight(1.0f), m_meshOffset(0.0f), m_yLift(0.0f),
      m_autoScale(1.0f), m_visible(true), m_drawSkeleton(false),
      m_drawBounds(false), m_forceEmissive(false), m_lineVAO(0), m_lineVBO(0)
{
}

// Destructor
SkinnedCharacter::~SkinnedCharacter()
{
    cleanup();
}

// Initialize character
bool SkinnedCharacter::init(const std::string& modelPath, const std::string& animNameOptional)
{
    std::cout << "[SkinnedCharacter] Initializing..." << std::endl;
    
    m_modelPath = modelPath;
    
    // Load model
    if (!loadModel(modelPath)) {
        std::cerr << "[SkinnedCharacter] Failed to load model" << std::endl;
        return false;
    }
    
    // Load shaders
    if (!loadShaders()) {
        std::cerr << "[SkinnedCharacter] Failed to load shaders" << std::endl;
        return false;
    }
    
    // Setup skeleton
    prepareSkeleton();
    
    // Setup mesh
    setupMesh();
    
    // Setup animation
    prepareAnimation(animNameOptional);
    
    // Compute auto-fit parameters
    computeAutoFit();
    
    std::cout << "[SkinnedCharacter] Initialization complete!" << std::endl;
    std::cout << "[SkinnedCharacter]   Joints: " << m_joints.size() << std::endl;
    std::cout << "[SkinnedCharacter]   Animation: " << m_currentAnimName << " (" << m_animDuration << "s)" << std::endl;
    std::cout << "[SkinnedCharacter]   Auto-scale: " << m_autoScale << std::endl;
    std::cout << "[SkinnedCharacter]   Mesh height: " << m_meshHeight << std::endl;
    
    return true;
}

// Cleanup
void SkinnedCharacter::cleanup()
{
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo_positions) glDeleteBuffers(1, &m_vbo_positions);
    if (m_vbo_normals) glDeleteBuffers(1, &m_vbo_normals);
    if (m_vbo_uvs) glDeleteBuffers(1, &m_vbo_uvs);
    if (m_vbo_boneIDs) glDeleteBuffers(1, &m_vbo_boneIDs);
    if (m_vbo_weights) glDeleteBuffers(1, &m_vbo_weights);
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
    if (m_lineVAO) glDeleteVertexArrays(1, &m_lineVAO);
    if (m_lineVBO) glDeleteBuffers(1, &m_lineVBO);
    if (m_shaderProgram) glDeleteProgram(m_shaderProgram);
    if (m_depthShaderProgram) glDeleteProgram(m_depthShaderProgram);
    if (m_debugLineShader) glDeleteProgram(m_debugLineShader);
}

// Load model from GLTF file
bool SkinnedCharacter::loadModel(const std::string& path)
{
    tinygltf::TinyGLTF loader;
    std::string err, warn;
    
    bool success = loader.LoadASCIIFromFile(&m_model, &err, &warn, path);
    
    if (!warn.empty()) {
        std::cout << "[SkinnedCharacter] Warning: " << warn << std::endl;
    }
    
    if (!err.empty()) {
        std::cerr << "[SkinnedCharacter] Error: " << err << std::endl;
        return false;
    }
    
    if (!success) {
        std::cerr << "[SkinnedCharacter] Failed to load GLTF: " << path << std::endl;
        return false;
    }
    
    std::cout << "[SkinnedCharacter] Loaded model: " << path << std::endl;
    std::cout << "[SkinnedCharacter]   Nodes: " << m_model.nodes.size() << std::endl;
    std::cout << "[SkinnedCharacter]   Meshes: " << m_model.meshes.size() << std::endl;
    std::cout << "[SkinnedCharacter]   Skins: " << m_model.skins.size() << std::endl;
    std::cout << "[SkinnedCharacter]   Animations: " << m_model.animations.size() << std::endl;
    
    return true;
}

// Load and compile shaders
bool SkinnedCharacter::loadShaders()
{
    // Main skinning shader
    std::string vertSource = loadShaderSource("shaders/skinning.vert");
    std::string fragSource = loadShaderSource("shaders/skinning.frag");
    
    if (vertSource.empty() || fragSource.empty()) {
        std::cerr << "[SkinnedCharacter] Failed to load shader sources" << std::endl;
        return false;
    }
    
    GLuint vertShader = compileShader(GL_VERTEX_SHADER, vertSource.c_str());
    GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, fragSource.c_str());
    
    if (vertShader == 0 || fragShader == 0) {
        return false;
    }
    
    m_shaderProgram = linkProgram(vertShader, fragShader);
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    
    if (m_shaderProgram == 0) {
        return false;
    }
    
    // Depth shader for shadows
    std::string depthVertSource = loadShaderSource("shaders/skinning_depth.vert");
    std::string depthFragSource = loadShaderSource("shaders/skinning_depth.frag");
    
    if (depthVertSource.empty() || depthFragSource.empty()) {
        std::cerr << "[SkinnedCharacter] Warning: Depth shaders not found, shadows will not work" << std::endl;
    } else {
        GLuint depthVertShader = compileShader(GL_VERTEX_SHADER, depthVertSource.c_str());
        GLuint depthFragShader = compileShader(GL_FRAGMENT_SHADER, depthFragSource.c_str());
        
        if (depthVertShader != 0 && depthFragShader != 0) {
            m_depthShaderProgram = linkProgram(depthVertShader, depthFragShader);
            glDeleteShader(depthVertShader);
            glDeleteShader(depthFragShader);
        }
    }
    
    std::cout << "[SkinnedCharacter] Shaders loaded successfully" << std::endl;
    return true;
}

// Setup skeleton from GLTF skin
void SkinnedCharacter::prepareSkeleton()
{
    if (m_model.skins.empty()) {
        std::cerr << "[SkinnedCharacter] No skins found in model" << std::endl;
        return;
    }
    
    const tinygltf::Skin& skin = m_model.skins[0];
    m_joints = skin.joints;
    
    // Store the skeleton root node index (important for proper transforms)
    int skeletonRoot = skin.skeleton;
    if (skeletonRoot < 0 && !m_joints.empty()) {
        // If no skeleton root specified, use the first joint
        skeletonRoot = m_joints[0];
    }
    
    std::cout << "[SkinnedCharacter] Skeleton root node: " << skeletonRoot << std::endl;
    
    // Load inverse bind matrices
    if (skin.inverseBindMatrices >= 0) {
        const tinygltf::Accessor& accessor = m_model.accessors[skin.inverseBindMatrices];
        const tinygltf::BufferView& bufferView = m_model.bufferViews[accessor.bufferView];
        const tinygltf::Buffer& buffer = m_model.buffers[bufferView.buffer];
        
        const float* data = reinterpret_cast<const float*>(
            &buffer.data[bufferView.byteOffset + accessor.byteOffset]);
        
        m_inverseBindMatrices.resize(accessor.count);
        for (size_t i = 0; i < accessor.count; ++i) {
            m_inverseBindMatrices[i] = glm::make_mat4(&data[i * 16]);
        }
        std::cout << "[SkinnedCharacter] Loaded " << m_inverseBindMatrices.size() << " inverse bind matrices" << std::endl;
    } else {
        std::cout << "[SkinnedCharacter] Warning: No inverse bind matrices, using identity" << std::endl;
        m_inverseBindMatrices.resize(m_joints.size(), glm::mat4(1.0f));
    }
    
    // Build parent hierarchy using node children relationships
    m_jointParents.resize(m_joints.size(), -1);
    
    // Create a map from node index to joint index for fast lookup
    std::unordered_map<int, int> nodeToJoint;
    for (size_t i = 0; i < m_joints.size(); ++i) {
        nodeToJoint[m_joints[i]] = (int)i;
    }
    
    // For each joint, find its parent in the joint list
    for (size_t i = 0; i < m_joints.size(); ++i) {
        int jointNodeIdx = m_joints[i];
        
        // Search all nodes to find which one has this joint as a child
        for (size_t nodeIdx = 0; nodeIdx < m_model.nodes.size(); ++nodeIdx) {
            const tinygltf::Node& node = m_model.nodes[nodeIdx];
            for (int childIdx : node.children) {
                if (childIdx == jointNodeIdx) {
                    // Found the parent node, check if it's also a joint
                    auto it = nodeToJoint.find((int)nodeIdx);
                    if (it != nodeToJoint.end()) {
                        m_jointParents[i] = it->second;
                    }
                    break;
                }
            }
        }
    }
    
    // Debug: print hierarchy
    std::cout << "[SkinnedCharacter] Joint hierarchy:" << std::endl;
    for (size_t i = 0; i < m_joints.size() && i < 5; ++i) {
        const tinygltf::Node& node = m_model.nodes[m_joints[i]];
        std::cout << "  Joint " << i << " (node " << m_joints[i] << "): \"" 
                  << node.name << "\" parent=" << m_jointParents[i] << std::endl;
    }
    if (m_joints.size() > 5) {
        std::cout << "  ... and " << (m_joints.size() - 5) << " more joints" << std::endl;
    }
    
    // Initialize runtime transforms
    m_jointPositions.resize(m_joints.size(), glm::vec3(0.0f));
    m_jointRotations.resize(m_joints.size(), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    m_jointScales.resize(m_joints.size(), glm::vec3(1.0f));
    m_jointLocalTransforms.resize(m_joints.size(), glm::mat4(1.0f));
    m_jointGlobalTransforms.resize(m_joints.size(), glm::mat4(1.0f));
    m_jointMatrices.resize(m_joints.size(), glm::mat4(1.0f));
    
    // Extract bind pose from nodes
    for (size_t i = 0; i < m_joints.size(); ++i) {
        const tinygltf::Node& node = m_model.nodes[m_joints[i]];
        
        // Check if node has a matrix directly
        if (node.matrix.size() == 16) {
            // Decompose matrix to TRS (we'll recompose it later)
            glm::mat4 mat = glm::make_mat4(node.matrix.data());
            m_jointPositions[i] = glm::vec3(mat[3]);
            m_jointScales[i] = glm::vec3(
                glm::length(glm::vec3(mat[0])),
                glm::length(glm::vec3(mat[1])),
                glm::length(glm::vec3(mat[2]))
            );
            glm::mat3 rotMat(
                glm::vec3(mat[0]) / m_jointScales[i].x,
                glm::vec3(mat[1]) / m_jointScales[i].y,
                glm::vec3(mat[2]) / m_jointScales[i].z
            );
            m_jointRotations[i] = glm::quat_cast(rotMat);
        } else {
            if (node.translation.size() == 3) {
                m_jointPositions[i] = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
            }
            if (node.rotation.size() == 4) {
                // GLTF quaternion is (x, y, z, w)
                m_jointRotations[i] = glm::quat((float)node.rotation[3], (float)node.rotation[0], 
                                               (float)node.rotation[1], (float)node.rotation[2]);
            }
            if (node.scale.size() == 3) {
                m_jointScales[i] = glm::vec3(node.scale[0], node.scale[1], node.scale[2]);
            }
        }
    }
    
    std::cout << "[SkinnedCharacter] Skeleton prepared with " << m_joints.size() << " joints" << std::endl;
}

// Setup animation
void SkinnedCharacter::prepareAnimation(const std::string& animNameOptional)
{
    if (m_model.animations.empty()) {
        std::cout << "[SkinnedCharacter] No animations found" << std::endl;
        return;
    }
    
    // Find animation by name or use first one
    m_animIndex = 0;
    if (!animNameOptional.empty()) {
        for (size_t i = 0; i < m_model.animations.size(); ++i) {
            if (m_model.animations[i].name == animNameOptional) {
                m_animIndex = (int)i;
                break;
            }
        }
    }
    
    const tinygltf::Animation& anim = m_model.animations[m_animIndex];
    m_currentAnimName = anim.name.empty() ? "Animation_0" : anim.name;
    
    // Parse animation channels
    m_channels.clear();
    m_animDuration = 0.0f;
    
    for (const tinygltf::AnimationChannel& channel : anim.channels) {
        const tinygltf::AnimationSampler& sampler = anim.samplers[channel.sampler];
        
        // Find or create channel for this node
        int nodeIndex = channel.target_node;
        int channelIdx = -1;
        for (size_t i = 0; i < m_channels.size(); ++i) {
            if (m_channels[i].nodeIndex == nodeIndex) {
                channelIdx = (int)i;
                break;
            }
        }
        if (channelIdx == -1) {
            Channel ch;
            ch.nodeIndex = nodeIndex;
            m_channels.push_back(ch);
            channelIdx = (int)m_channels.size() - 1;
        }
        
        // Load input times
        const tinygltf::Accessor& timeAccessor = m_model.accessors[sampler.input];
        const tinygltf::BufferView& timeBufferView = m_model.bufferViews[timeAccessor.bufferView];
        const tinygltf::Buffer& timeBuffer = m_model.buffers[timeBufferView.buffer];
        const float* times = reinterpret_cast<const float*>(
            &timeBuffer.data[timeBufferView.byteOffset + timeAccessor.byteOffset]);
        
        std::vector<float> timeVec(times, times + timeAccessor.count);
        if (!timeVec.empty()) {
            m_animDuration = std::max(m_animDuration, timeVec.back());
        }
        
        // Load output values
        const tinygltf::Accessor& valueAccessor = m_model.accessors[sampler.output];
        const tinygltf::BufferView& valueBufferView = m_model.bufferViews[valueAccessor.bufferView];
        const tinygltf::Buffer& valueBuffer = m_model.buffers[valueBufferView.buffer];
        const float* values = reinterpret_cast<const float*>(
            &valueBuffer.data[valueBufferView.byteOffset + valueAccessor.byteOffset]);
        
        if (channel.target_path == "translation") {
            m_channels[channelIdx].translationTimes = timeVec;
            m_channels[channelIdx].translationValues.clear();
            for (size_t i = 0; i < valueAccessor.count; ++i) {
                m_channels[channelIdx].translationValues.push_back(
                    glm::vec3(values[i * 3], values[i * 3 + 1], values[i * 3 + 2]));
            }
        }
        else if (channel.target_path == "rotation") {
            m_channels[channelIdx].rotationTimes = timeVec;
            m_channels[channelIdx].rotationValues.clear();
            for (size_t i = 0; i < valueAccessor.count; ++i) {
                m_channels[channelIdx].rotationValues.push_back(
                    glm::quat(values[i * 4 + 3], values[i * 4], values[i * 4 + 1], values[i * 4 + 2]));
            }
        }
        else if (channel.target_path == "scale") {
            m_channels[channelIdx].scaleTimes = timeVec;
            m_channels[channelIdx].scaleValues.clear();
            for (size_t i = 0; i < valueAccessor.count; ++i) {
                m_channels[channelIdx].scaleValues.push_back(
                    glm::vec3(values[i * 3], values[i * 3 + 1], values[i * 3 + 2]));
            }
        }
    }
    
    std::cout << "[SkinnedCharacter] Animation prepared: " << m_currentAnimName << std::endl;
    std::cout << "[SkinnedCharacter]   Duration: " << m_animDuration << "s" << std::endl;
    std::cout << "[SkinnedCharacter]   Channels: " << m_channels.size() << std::endl;
}

// Setup mesh VAO/VBO
void SkinnedCharacter::setupMesh()
{
    // Find the node that has both a mesh AND a skin
    int skinnedMeshNodeIdx = -1;
    int skinnedMeshIdx = -1;
    int skinIdx = -1;
    
    for (size_t i = 0; i < m_model.nodes.size(); ++i) {
        const tinygltf::Node& node = m_model.nodes[i];
        if (node.mesh >= 0 && node.skin >= 0) {
            skinnedMeshNodeIdx = (int)i;
            skinnedMeshIdx = node.mesh;
            skinIdx = node.skin;
            std::cout << "[SkinnedCharacter] Found skinned mesh node " << i 
                      << " with mesh " << skinnedMeshIdx << " and skin " << skinIdx << std::endl;
            break;
        }
    }
    
    // Fallback: if no skinned node found, use first mesh
    if (skinnedMeshIdx < 0) {
        if (m_model.meshes.empty()) {
            std::cerr << "[SkinnedCharacter] No meshes found" << std::endl;
            return;
        }
        skinnedMeshIdx = 0;
        std::cout << "[SkinnedCharacter] No skinned mesh node found, using mesh 0" << std::endl;
    }
    
    const tinygltf::Mesh& mesh = m_model.meshes[skinnedMeshIdx];
    if (mesh.primitives.empty()) {
        std::cerr << "[SkinnedCharacter] No primitives in mesh" << std::endl;
        return;
    }
    
    std::cout << "[SkinnedCharacter] Mesh has " << mesh.primitives.size() << " primitives" << std::endl;

    // Collect all vertex data from ALL primitives
    std::vector<float> allPositions;
    std::vector<float> allNormals;
    std::vector<float> allUVs;
    std::vector<float> allBoneIDs;
    std::vector<float> allWeights;
    std::vector<uint32_t> allIndices;  // Use 32-bit for combined indices
    
    size_t vertexOffset = 0;
    
    // Helper to extract attribute data
    auto getAccessorData = [&](int accessorIdx) -> const unsigned char* {
        if (accessorIdx < 0) return nullptr;
        const tinygltf::Accessor& accessor = m_model.accessors[accessorIdx];
        const tinygltf::BufferView& bufferView = m_model.bufferViews[accessor.bufferView];
        const tinygltf::Buffer& buffer = m_model.buffers[bufferView.buffer];
        return &buffer.data[bufferView.byteOffset + accessor.byteOffset];
    };
    
    // Process each primitive
    for (size_t primIdx = 0; primIdx < mesh.primitives.size(); ++primIdx) {
        const tinygltf::Primitive& primitive = mesh.primitives[primIdx];
        
        // Skip non-triangle primitives (default mode is triangles when mode is -1 or 4)
        int mode = primitive.mode;
        if (mode == -1) mode = TINYGLTF_MODE_TRIANGLES; // Default
        if (mode != TINYGLTF_MODE_TRIANGLES) {
            std::cout << "[SkinnedCharacter] Skipping primitive " << primIdx 
                      << " with mode " << mode << " (not triangles)" << std::endl;
            continue;
        }
        
        // Get attributes
        int posAccessor = primitive.attributes.count("POSITION") ? primitive.attributes.at("POSITION") : -1;
        int normAccessor = primitive.attributes.count("NORMAL") ? primitive.attributes.at("NORMAL") : -1;
        int uvAccessor = primitive.attributes.count("TEXCOORD_0") ? primitive.attributes.at("TEXCOORD_0") : -1;
        int jointsAccessor = primitive.attributes.count("JOINTS_0") ? primitive.attributes.at("JOINTS_0") : -1;
        int weightsAccessor = primitive.attributes.count("WEIGHTS_0") ? primitive.attributes.at("WEIGHTS_0") : -1;
        
        std::cout << "[SkinnedCharacter] Primitive " << primIdx << " attributes: "
                  << "POSITION=" << posAccessor 
                  << ", NORMAL=" << normAccessor 
                  << ", TEXCOORD_0=" << uvAccessor
                  << ", JOINTS_0=" << jointsAccessor 
                  << ", WEIGHTS_0=" << weightsAccessor << std::endl;
        
        if (posAccessor < 0) {
            std::cerr << "[SkinnedCharacter] Primitive " << primIdx << " has no POSITION attribute" << std::endl;
            continue;
        }
        
        const tinygltf::Accessor& posAcc = m_model.accessors[posAccessor];
        size_t vertexCount = posAcc.count;
        
        std::cout << "[SkinnedCharacter] Primitive " << primIdx << ": " << vertexCount << " vertices" << std::endl;
        
        // Positions
        const float* positions = reinterpret_cast<const float*>(getAccessorData(posAccessor));
        for (size_t i = 0; i < vertexCount; ++i) {
            glm::vec3 pos(positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2]);
            allPositions.push_back(pos.x);
            allPositions.push_back(pos.y);
            allPositions.push_back(pos.z);
            m_aabb.expand(pos);
        }
        
        // Normals
        if (normAccessor >= 0) {
            const float* normals = reinterpret_cast<const float*>(getAccessorData(normAccessor));
            for (size_t i = 0; i < vertexCount * 3; ++i) {
                allNormals.push_back(normals[i]);
            }
        } else {
            for (size_t i = 0; i < vertexCount; ++i) {
                allNormals.push_back(0.0f);
                allNormals.push_back(1.0f);
                allNormals.push_back(0.0f);
            }
        }
        
        // UVs
        if (uvAccessor >= 0) {
            const float* uvs = reinterpret_cast<const float*>(getAccessorData(uvAccessor));
            for (size_t i = 0; i < vertexCount * 2; ++i) {
                allUVs.push_back(uvs[i]);
            }
        } else {
            for (size_t i = 0; i < vertexCount; ++i) {
                allUVs.push_back(0.0f);
                allUVs.push_back(0.0f);
            }
        }
        
        // Bone IDs
        if (jointsAccessor >= 0) {
            const tinygltf::Accessor& jointsAcc = m_model.accessors[jointsAccessor];
            
            if (jointsAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                const uint16_t* joints = reinterpret_cast<const uint16_t*>(getAccessorData(jointsAccessor));
                for (size_t i = 0; i < vertexCount * 4; ++i) {
                    allBoneIDs.push_back((float)joints[i]);
                }
            } else if (jointsAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                const uint8_t* joints = reinterpret_cast<const uint8_t*>(getAccessorData(jointsAccessor));
                for (size_t i = 0; i < vertexCount * 4; ++i) {
                    allBoneIDs.push_back((float)joints[i]);
                }
            }
        } else {
            for (size_t i = 0; i < vertexCount; ++i) {
                allBoneIDs.push_back(0.0f);
                allBoneIDs.push_back(0.0f);
                allBoneIDs.push_back(0.0f);
                allBoneIDs.push_back(0.0f);
            }
        }
        
        // Weights
        if (weightsAccessor >= 0) {
            const tinygltf::Accessor& weightsAcc = m_model.accessors[weightsAccessor];
            
            if (weightsAcc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                const float* weights = reinterpret_cast<const float*>(getAccessorData(weightsAccessor));
                for (size_t i = 0; i < vertexCount * 4; ++i) {
                    allWeights.push_back(weights[i]);
                }
            } else if (weightsAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                const uint16_t* weights = reinterpret_cast<const uint16_t*>(getAccessorData(weightsAccessor));
                for (size_t i = 0; i < vertexCount * 4; ++i) {
                    allWeights.push_back((float)weights[i] / 65535.0f);
                }
            } else if (weightsAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                const uint8_t* weights = reinterpret_cast<const uint8_t*>(getAccessorData(weightsAccessor));
                for (size_t i = 0; i < vertexCount * 4; ++i) {
                    allWeights.push_back((float)weights[i] / 255.0f);
                }
            }
        } else {
            for (size_t i = 0; i < vertexCount; ++i) {
                allWeights.push_back(1.0f);
                allWeights.push_back(0.0f);
                allWeights.push_back(0.0f);
                allWeights.push_back(0.0f);
            }
        }
        
        // Indices
        if (primitive.indices >= 0) {
            const tinygltf::Accessor& indexAcc = m_model.accessors[primitive.indices];
            const unsigned char* indexData = getAccessorData(primitive.indices);
            
            if (indexAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                const uint16_t* indices = reinterpret_cast<const uint16_t*>(indexData);
                for (size_t i = 0; i < indexAcc.count; ++i) {
                    allIndices.push_back((uint32_t)(indices[i] + vertexOffset));
                }
            } else if (indexAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                const uint32_t* indices = reinterpret_cast<const uint32_t*>(indexData);
                for (size_t i = 0; i < indexAcc.count; ++i) {
                    allIndices.push_back(indices[i] + (uint32_t)vertexOffset);
                }
            } else if (indexAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                const uint8_t* indices = reinterpret_cast<const uint8_t*>(indexData);
                for (size_t i = 0; i < indexAcc.count; ++i) {
                    allIndices.push_back((uint32_t)(indices[i] + vertexOffset));
                }
            }
        }
        
        vertexOffset += vertexCount;
    }
    
    if (allPositions.empty()) {
        std::cerr << "[SkinnedCharacter] No vertex data collected from primitives" << std::endl;
        return;
    }
    
    GLsizei totalVertexCount = (GLsizei)(allPositions.size() / 3);
    m_indexCount = (GLsizei)allIndices.size();
    m_indexType = GL_UNSIGNED_INT;  // Always use 32-bit for merged mesh
    
    // Create VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    
    // Position (location 0)
    glGenBuffers(1, &m_vbo_positions);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo_positions);
    glBufferData(GL_ARRAY_BUFFER, allPositions.size() * sizeof(float), allPositions.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
    // Normal (location 1)
    glGenBuffers(1, &m_vbo_normals);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo_normals);
    glBufferData(GL_ARRAY_BUFFER, allNormals.size() * sizeof(float), allNormals.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
    // UV (location 2)
    glGenBuffers(1, &m_vbo_uvs);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo_uvs);
    glBufferData(GL_ARRAY_BUFFER, allUVs.size() * sizeof(float), allUVs.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
    // Bone IDs (location 3)
    glGenBuffers(1, &m_vbo_boneIDs);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo_boneIDs);
    glBufferData(GL_ARRAY_BUFFER, allBoneIDs.size() * sizeof(float), allBoneIDs.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
    // Weights (location 4)
    glGenBuffers(1, &m_vbo_weights);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo_weights);
    glBufferData(GL_ARRAY_BUFFER, allWeights.size() * sizeof(float), allWeights.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
    // Indices
    glGenBuffers(1, &m_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, allIndices.size() * sizeof(uint32_t), allIndices.data(), GL_STATIC_DRAW);
    
    glBindVertexArray(0);
    
    std::cout << "[SkinnedCharacter] Mesh setup complete" << std::endl;
    std::cout << "[SkinnedCharacter]   Total Vertices: " << totalVertexCount << std::endl;
    std::cout << "[SkinnedCharacter]   Total Indices: " << m_indexCount << std::endl;
    std::cout << "[SkinnedCharacter]   AABB: min(" << m_aabb.min.x << ", " << m_aabb.min.y << ", " << m_aabb.min.z << ")" << std::endl;
    std::cout << "[SkinnedCharacter]   AABB: max(" << m_aabb.max.x << ", " << m_aabb.max.y << ", " << m_aabb.max.z << ")" << std::endl;
}

// Compute auto-fit parameters
void SkinnedCharacter::computeAutoFit()
{
    // Get mesh dimensions
    glm::vec3 meshSize = m_aabb.size();
    m_meshHeight = meshSize.y;
    
    // Compute center of AABB
    glm::vec3 center = (m_aabb.min + m_aabb.max) * 0.5f;
    
    // Target height: 2.0 units (human-scale) - reasonable for a city scene
    float desiredHeight = 2.0f;
    
    // Only auto-scale if mesh height is valid
    if (m_meshHeight > 0.001f) {
        m_autoScale = desiredHeight / m_meshHeight;
    } else {
        m_autoScale = 1.0f;
    }
    
    // Clamp auto-scale to reasonable values
    if (m_autoScale < 0.001f) m_autoScale = 0.001f;
    if (m_autoScale > 100.0f) m_autoScale = 100.0f;
    
    // CRITICAL: The mesh is NOT centered at origin!
    // We need to offset it so the mesh center is at origin (in XZ)
    // and the feet touch Y=0
    // meshOffset will be applied in the shader AFTER skinning
    m_meshOffset = glm::vec3(-center.x, -m_aabb.min.y, -center.z);
    
    // No Y lift needed since meshOffset handles grounding
    m_yLift = 0.0f;
    
    // Store the RAW AABB min Y for external calculations
    // But since we now apply meshOffset in shader, external callers should use 0
    m_groundOffsetY = 0.0f;  // After meshOffset, feet are at Y=0 in model space
    
    std::cout << "[SkinnedCharacter] Auto-fit computed:" << std::endl;
    std::cout << "[SkinnedCharacter]   AABB min: (" << m_aabb.min.x << ", " << m_aabb.min.y << ", " << m_aabb.min.z << ")" << std::endl;
    std::cout << "[SkinnedCharacter]   AABB max: (" << m_aabb.max.x << ", " << m_aabb.max.y << ", " << m_aabb.max.z << ")" << std::endl;
    std::cout << "[SkinnedCharacter]   Mesh center: (" << center.x << ", " << center.y << ", " << center.z << ")" << std::endl;
    std::cout << "[SkinnedCharacter]   Mesh offset (shader): (" << m_meshOffset.x << ", " << m_meshOffset.y << ", " << m_meshOffset.z << ")" << std::endl;
    std::cout << "[SkinnedCharacter]   Mesh height (model space): " << m_meshHeight << std::endl;
    std::cout << "[SkinnedCharacter]   Auto-scale: " << m_autoScale << std::endl;
    std::cout << "[SkinnedCharacter]   Ground offset Y (after meshOffset): " << m_groundOffsetY << std::endl;
}

// Update animation
void SkinnedCharacter::update(float dt)
{
    if (m_animDuration > 0.0f) {
        m_animTime += dt;
        while (m_animTime >= m_animDuration) {
            m_animTime -= m_animDuration;
        }
        
        evaluateAnimation(m_animTime);
        computeJointMatrices();
    }
}

// Evaluate animation at given time
void SkinnedCharacter::evaluateAnimation(float time)
{
    // For each channel, interpolate TRS
    for (const Channel& ch : m_channels) {
        // Find joint index for this node
        int jointIdx = -1;
        for (size_t i = 0; i < m_joints.size(); ++i) {
            if (m_joints[i] == ch.nodeIndex) {
                jointIdx = (int)i;
                break;
            }
        }
        
        if (jointIdx < 0) continue;
        
        // Interpolate translation
        if (!ch.translationValues.empty()) {
            m_jointPositions[jointIdx] = interpolateVec3(ch.translationValues, ch.translationTimes, time);
        }
        
        // Interpolate rotation
        if (!ch.rotationValues.empty()) {
            m_jointRotations[jointIdx] = interpolateQuat(ch.rotationValues, ch.rotationTimes, time);
        }
        
        // Interpolate scale
        if (!ch.scaleValues.empty()) {
            m_jointScales[jointIdx] = interpolateVec3(ch.scaleValues, ch.scaleTimes, time);
        }
    }
}

// Compute joint matrices for GPU
void SkinnedCharacter::computeJointMatrices()
{
    // Compute local transforms from TRS
    for (size_t i = 0; i < m_joints.size(); ++i) {
        glm::mat4 T = glm::translate(glm::mat4(1.0f), m_jointPositions[i]);
        glm::mat4 R = glm::mat4_cast(m_jointRotations[i]);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), m_jointScales[i]);
        m_jointLocalTransforms[i] = T * R * S;
    }
    
    // Reset global transforms
    for (size_t i = 0; i < m_joints.size(); ++i) {
        m_jointGlobalTransforms[i] = glm::mat4(1.0f);
    }
    
    // Compute global transforms via recursive traversal (handles any ordering)
    std::vector<bool> computed(m_joints.size(), false);
    
    // Helper lambda for recursive computation
    std::function<void(int)> computeGlobalTransform = [&](int jointIdx) {
        if (jointIdx < 0 || jointIdx >= (int)m_joints.size()) return;
        if (computed[jointIdx]) return;
        
        int parentIdx = m_jointParents[jointIdx];
        if (parentIdx >= 0 && !computed[parentIdx]) {
            computeGlobalTransform(parentIdx);
        }
        
        if (parentIdx < 0) {
            m_jointGlobalTransforms[jointIdx] = m_jointLocalTransforms[jointIdx];
        } else {
            m_jointGlobalTransforms[jointIdx] = m_jointGlobalTransforms[parentIdx] * m_jointLocalTransforms[jointIdx];
        }
        computed[jointIdx] = true;
    };
    
    // Compute all joints
    for (size_t i = 0; i < m_joints.size(); ++i) {
        computeGlobalTransform((int)i);
    }
    
    // Compute final matrices: global * inverseBindMatrix
    for (size_t i = 0; i < m_joints.size(); ++i) {
        if (i < m_inverseBindMatrices.size()) {
            m_jointMatrices[i] = m_jointGlobalTransforms[i] * m_inverseBindMatrices[i];
        } else {
            // Fallback if inverse bind matrix is missing
            m_jointMatrices[i] = m_jointGlobalTransforms[i];
        }
    }
}

// Render character
void SkinnedCharacter::render(const glm::mat4& view, const glm::mat4& proj,
                              const DirectionalLight& dirLight, const PointLight& ptLight,
                              const ShadowData& shadows, const RenderOptions& opts)
{
    if (!m_visible || m_shaderProgram == 0 || m_vao == 0 || m_indexCount == 0) {
        return;
    }
    
    glUseProgram(m_shaderProgram);
    
    float finalScale = m_scale.x * m_autoScale;
    
    // Build model matrix: T(worldPos) * R(yaw) * S(scale)
    // No Y lift - the caller (PathNavigator) sets the correct Y position
    glm::mat4 model(1.0f);
    model = glm::translate(model, m_position);  // Direct position from setWorldTransform
    model = glm::rotate(model, m_yaw, glm::vec3(0.0f, 1.0f, 0.0f));  // Yaw rotation
    model = glm::scale(model, glm::vec3(finalScale));  // Scale to desired size

    // Set uniforms
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "uModel"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "uView"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "uProjection"), 1, GL_FALSE, glm::value_ptr(proj));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "uLightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(shadows.lightSpaceMatrix));
    
    // Pass mesh offset to shader (always zero now)
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "uMeshOffset"), 1, glm::value_ptr(m_meshOffset));
    
    // Upload joint matrices - make sure we have valid matrices
    if (!m_jointMatrices.empty()) {
        glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "uBones"), 
                           (GLsizei)m_jointMatrices.size(), GL_FALSE, glm::value_ptr(m_jointMatrices[0]));
    }
    
    // Lighting
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "uDirLightDir"), 1, glm::value_ptr(dirLight.direction));
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "uDirLightColor"), 1, glm::value_ptr(dirLight.color));
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "uPointLightPos"), 1, glm::value_ptr(ptLight.position));
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "uPointLightColor"), 1, glm::value_ptr(ptLight.color));
    glUniform1f(glGetUniformLocation(m_shaderProgram, "uPointLightConstant"), ptLight.constant);
    glUniform1f(glGetUniformLocation(m_shaderProgram, "uPointLightLinear"), ptLight.linear);
    glUniform1f(glGetUniformLocation(m_shaderProgram, "uPointLightQuadratic"), ptLight.quadratic);
    
    glUniform1i(glGetUniformLocation(m_shaderProgram, "uEnableShadows"), opts.enableShadows ? 1 : 0);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "uUsePCF"), opts.enablePCF ? 1 : 0);
    glUniform1f(glGetUniformLocation(m_shaderProgram, "uBloomThreshold"), opts.bloomThreshold);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "uForceEmissive"), m_forceEmissive ? 1 : 0);
    
    // Bind shadow map
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadows.shadowMapTexture);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "uShadowMap"), 1);
    
    // Draw with proper state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, m_indexType, 0);
    glBindVertexArray(0);
    
    // Debug visualizations
    if (m_drawSkeleton) {
        renderDebugSkeleton(proj * view);
    }
    if (m_drawBounds) {
        renderDebugBounds(proj * view);
    }
}

// Render depth-only for shadow pass
void SkinnedCharacter::renderDepth(const glm::mat4& lightSpaceMatrix)
{
    if (!m_visible || m_depthShaderProgram == 0 || m_vao == 0) return;
    
    glUseProgram(m_depthShaderProgram);
    
    float finalScale = m_scale.x * m_autoScale;
    
    // Build model matrix: T(worldPos) * R(yaw) * S(scale)
    // No Y lift - the caller (PathNavigator) sets the correct Y position
    glm::mat4 model(1.0f);
    model = glm::translate(model, m_position);  // Direct position from setWorldTransform
    model = glm::rotate(model, m_yaw, glm::vec3(0.0f, 1.0f, 0.0f));  // Yaw rotation
    model = glm::scale(model, glm::vec3(finalScale));  // Scale to desired size
    
    // Remove Y lift adjustment - directly use m_position
    
    glUniformMatrix4fv(glGetUniformLocation(m_depthShaderProgram, "uModel"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(m_depthShaderProgram, "uLightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    
    // Pass mesh offset to shader (always zero now)
    glUniform3fv(glGetUniformLocation(m_depthShaderProgram, "uMeshOffset"), 1, glm::value_ptr(m_meshOffset));
    
    // Upload joint matrices
    glUniformMatrix4fv(glGetUniformLocation(m_depthShaderProgram, "uBones"), 
                       (GLsizei)m_jointMatrices.size(), GL_FALSE, glm::value_ptr(m_jointMatrices[0]));
    
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, m_indexType, 0);
    glBindVertexArray(0);
}

// Debug skeleton rendering (stub - implement if needed)
void SkinnedCharacter::renderDebugSkeleton(const glm::mat4& vp)
{
    // TODO: Implement skeleton line rendering
}

// Debug AABB rendering (stub - implement if needed)
void SkinnedCharacter::renderDebugBounds(const glm::mat4& vp)
{
    // TODO: Implement AABB line rendering
}

// Transform setters
void SkinnedCharacter::setWorldTransform(const glm::vec3& pos, float yawRadians)
{
    m_position = pos;
    m_yaw = yawRadians;
}

void SkinnedCharacter::setScale(float uniformScale)
{
    m_scale = glm::vec3(uniformScale);
}

void SkinnedCharacter::setVisible(bool v)
{
    m_visible = v;
}

void SkinnedCharacter::setDrawSkeleton(bool v)
{
    m_drawSkeleton = v;
}

void SkinnedCharacter::setDrawBounds(bool v)
{
    m_drawBounds = v;
}

void SkinnedCharacter::setForceEmissive(bool v)
{
    m_forceEmissive = v;
}

AABB SkinnedCharacter::getWorldAABB() const
{
    AABB worldAABB;
    float finalScale = m_scale.x * m_autoScale;
    worldAABB.min = m_position + (m_aabb.min * finalScale);
    worldAABB.max = m_position + (m_aabb.max * finalScale);
    return worldAABB;
}

// Utility: Get node transform
glm::mat4 SkinnedCharacter::getNodeTransform(const tinygltf::Node& node)
{
    if (node.matrix.size() == 16) {
        return glm::make_mat4(node.matrix.data());
    }
    
    glm::mat4 T(1.0f), R(1.0f), S(1.0f);
    if (node.translation.size() == 3) {
        T = glm::translate(glm::mat4(1.0f), glm::vec3(node.translation[0], node.translation[1], node.translation[2]));
    }
    if (node.rotation.size() == 4) {
        glm::quat q((float)node.rotation[3], (float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2]);
        R = glm::mat4_cast(q);
    }
    if (node.scale.size() == 3) {
        S = glm::scale(glm::mat4(1.0f), glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
    }
    return T * R * S;
}

// Utility: Find keyframe index
int SkinnedCharacter::findKeyframeIndex(const std::vector<float>& times, float t)
{
    if (times.empty()) return -1;
    if (t <= times.front()) return 0;
    if (t >= times.back()) return (int)times.size() - 2;
    
    for (size_t i = 0; i < times.size() - 1; ++i) {
        if (t >= times[i] && t < times[i + 1]) {
            return (int)i;
        }
    }
    return (int)times.size() - 2;
}

// Utility: Interpolate vec3
glm::vec3 SkinnedCharacter::interpolateVec3(const std::vector<glm::vec3>& values,
                                            const std::vector<float>& times, float t)
{
    if (values.empty()) return glm::vec3(0.0f);
    if (values.size() == 1) return values[0];
    
    int idx = findKeyframeIndex(times, t);
    if (idx < 0) return values[0];
    
    float t0 = times[idx];
    float t1 = times[idx + 1];
    float alpha = (t - t0) / (t1 - t0);
    
    return glm::mix(values[idx], values[idx + 1], alpha);
}

// Utility: Interpolate quaternion (slerp)
glm::quat SkinnedCharacter::interpolateQuat(const std::vector<glm::quat>& values,
                                            const std::vector<float>& times, float t)
{
    if (values.empty()) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    if (values.size() == 1) return values[0];
    
    int idx = findKeyframeIndex(times, t);
    if (idx < 0) return values[0];
    
    float t0 = times[idx];
    float t1 = times[idx + 1];
    float alpha = (t - t0) / (t1 - t0);
    
    return glm::normalize(glm::slerp(values[idx], values[idx + 1], alpha));
}

// Shader utilities
std::string SkinnedCharacter::loadShaderSource(const char* path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[SkinnedCharacter] Failed to open shader: " << path << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint SkinnedCharacter::compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "[SkinnedCharacter] Shader compilation failed:\n" << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

GLuint SkinnedCharacter::linkProgram(GLuint vertShader, GLuint fragShader)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "[SkinnedCharacter] Shader linking failed:\n" << infoLog << std::endl;
        glDeleteProgram(program);
        return 0;
    }
    
    return program;
}
