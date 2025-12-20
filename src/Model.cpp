#include "Model.h"
#include <iostream>

Model::Model(const std::string& path, const std::string& fallbackTexturePath)
    : usingFallbackTexture(false), hasFallbackTexture(false), fallbackTexturePath(fallbackTexturePath)
{
    // Try to load fallback texture if path provided
    if (!fallbackTexturePath.empty())
    {
        hasFallbackTexture = loadFallbackTexture(fallbackTexturePath);
    }
    
    loadModel(path);
}

bool Model::loadFallbackTexture(const std::string& path)
{
    std::cout << "\n[Fallback Texture] Attempting to load: " << path << std::endl;
    
    if (fallbackTexture.LoadFromFile(path, true))
    {
        fallbackTexture.Type = "diffuse";
        std::cout << "  [OK] Fallback texture loaded successfully" << std::endl;
        return true;
    }
    else
    {
        std::cerr << "  [ERROR] Failed to load fallback texture" << std::endl;
        return false;
    }
}

void Model::applyFallbackTextureIfNeeded(std::vector<Texture>& textures)
{
    if (textures.empty() && hasFallbackTexture)
    {
        std::cout << "  [INFO] Applying fallback texture (no material textures found)" << std::endl;
        textures.push_back(fallbackTexture);
        usingFallbackTexture = true;
    }
}

void Model::Draw(unsigned int shaderProgram)
{
    for (unsigned int i = 0; i < meshes.size(); i++)
        meshes[i].Draw(shaderProgram);
}

void Model::Delete()
{
    for (auto& mesh : meshes)
    {
        mesh.Delete();
    }
}

void Model::loadModel(const std::string& path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }

    directory = path.substr(0, path.find_last_of('/'));
    if (directory == path)  // No '/' found, try backslash
        directory = path.substr(0, path.find_last_of('\\'));

    std::cout << "\n=== MODEL LOADING ===" << std::endl;
    std::cout << "Model path: " << path << std::endl;
    std::cout << "  Meshes: " << scene->mNumMeshes << std::endl;
    std::cout << "  Materials: " << scene->mNumMaterials << std::endl;

    processNode(scene->mRootNode, scene);

    // Count total textures loaded
    int totalTextures = 0;
    for (const auto& mesh : meshes)
    {
        totalTextures += mesh.textures.size();
    }
    
    std::cout << "Model loaded successfully:" << std::endl;
    std::cout << "  " << meshes.size() << " meshes" << std::endl;
    std::cout << "  " << totalTextures << " textures total" << std::endl;
    
    if (usingFallbackTexture)
    {
        std::cout << "  [INFO] FALLBACK TEXTURE APPLIED (no material textures found)" << std::endl;
    }
    else if (totalTextures > 0)
    {
        std::cout << "  [OK] USING MATERIAL TEXTURES" << std::endl;
    }
    else
    {
        std::cout << "  [WARN] NO TEXTURES (will use shader fallback)" << std::endl;
    }
    
    std::cout << "=====================\n" << std::endl;
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
    // Process all the node's meshes
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }

    // Process children nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    std::cout << "\nProcessing mesh: " << (mesh->mName.length > 0 ? mesh->mName.C_Str() : "unnamed") << std::endl;
    std::cout << "  Vertices: " << mesh->mNumVertices << std::endl;
    std::cout << "  Faces: " << mesh->mNumFaces << std::endl;
    std::cout << "  Has normals: " << (mesh->HasNormals() ? "yes" : "no") << std::endl;
    std::cout << "  Has UVs: " << (mesh->mTextureCoords[0] != nullptr ? "yes" : "NO - will use (0,0)") << std::endl;

    // Process vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;

        // Position
        glm::vec3 vector;
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;

        // Normals
        if (mesh->HasNormals())
        {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
        }
        else
        {
            vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        // Texture coordinates
        if (mesh->mTextureCoords[0])
        {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
        }
        else
        {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

    // Process indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    // Process material
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        
        std::cout << "  Material index: " << mesh->mMaterialIndex << std::endl;
        
        // Print all available texture types in this material
        std::cout << "  Material texture counts:" << std::endl;
        std::cout << "    DIFFUSE: " << material->GetTextureCount(aiTextureType_DIFFUSE) << std::endl;
        std::cout << "    SPECULAR: " << material->GetTextureCount(aiTextureType_SPECULAR) << std::endl;
        std::cout << "    AMBIENT: " << material->GetTextureCount(aiTextureType_AMBIENT) << std::endl;
        std::cout << "    EMISSIVE: " << material->GetTextureCount(aiTextureType_EMISSIVE) << std::endl;
        std::cout << "    HEIGHT: " << material->GetTextureCount(aiTextureType_HEIGHT) << std::endl;
        std::cout << "    NORMALS: " << material->GetTextureCount(aiTextureType_NORMALS) << std::endl;
        std::cout << "    SHININESS: " << material->GetTextureCount(aiTextureType_SHININESS) << std::endl;
        std::cout << "    OPACITY: " << material->GetTextureCount(aiTextureType_OPACITY) << std::endl;
        std::cout << "    DISPLACEMENT: " << material->GetTextureCount(aiTextureType_DISPLACEMENT) << std::endl;
        std::cout << "    LIGHTMAP: " << material->GetTextureCount(aiTextureType_LIGHTMAP) << std::endl;
        std::cout << "    REFLECTION: " << material->GetTextureCount(aiTextureType_REFLECTION) << std::endl;
        std::cout << "    BASE_COLOR: " << material->GetTextureCount(aiTextureType_BASE_COLOR) << std::endl;

        // Try diffuse maps
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        
        // Try base color maps (for PBR materials)
        std::vector<Texture> baseColorMaps = loadMaterialTextures(material, aiTextureType_BASE_COLOR, "diffuse");
        textures.insert(textures.end(), baseColorMaps.begin(), baseColorMaps.end());
        
        // If still no textures, try ambient as fallback
        if (textures.empty())
        {
            std::cout << "  No DIFFUSE or BASE_COLOR found, trying AMBIENT as fallback..." << std::endl;
            std::vector<Texture> ambientMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "diffuse");
            textures.insert(textures.end(), ambientMaps.begin(), ambientMaps.end());
        }
        
        std::cout << "  Total material textures loaded: " << textures.size() << std::endl;
        
        // Apply fallback texture if no material textures found
        applyFallbackTextureIfNeeded(textures);
        
        if (textures.empty())
        {
            std::cout << "  [WARN] No textures available for this mesh (no material textures, no fallback)" << std::endl;
            std::cout << "  Model will render with shader fallback color (normal-based shading)" << std::endl;
        }
        else if (usingFallbackTexture)
        {
            std::cout << "  [INFO] Using FALLBACK TEXTURE for this mesh" << std::endl;
        }
        else
        {
            std::cout << "  [OK] Using MATERIAL TEXTURES for this mesh" << std::endl;
        }
    }
    else
    {
        std::cout << "  No material assigned to this mesh" << std::endl;
        
        // Apply fallback texture for meshes without materials
        applyFallbackTextureIfNeeded(textures);
        
        if (!textures.empty() && usingFallbackTexture)
        {
            std::cout << "  [INFO] Applied FALLBACK TEXTURE (no material)" << std::endl;
        }
    }

    return Mesh(vertices, indices, textures);
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
{
    std::vector<Texture> textures;
    unsigned int textureCount = mat->GetTextureCount(type);
    
    if (textureCount > 0)
    {
        std::cout << "    Found " << textureCount << " " << typeName << " texture(s)" << std::endl;
    }

    for (unsigned int i = 0; i < textureCount; i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);
        std::string texturePath = std::string(str.C_Str());

        std::cout << "    Texture path from material: " << texturePath << std::endl;

        // Check if texture was loaded before
        if (texturesLoaded.find(texturePath) != texturesLoaded.end())
        {
            std::cout << "      Using cached texture" << std::endl;
            textures.push_back(texturesLoaded[texturePath]);
            continue;
        }

        // Load new texture
        Texture texture;
        std::string fullPath = directory + "/" + texturePath;

        std::cout << "      Attempting to load from: " << fullPath << std::endl;
        
        // Try with forward slash
        if (texture.LoadFromFile(fullPath, true))
        {
            texture.Type = typeName;
            texture.Path = texturePath;
            textures.push_back(texture);
            texturesLoaded[texturePath] = texture;
            std::cout << "      [OK] Texture loaded successfully" << std::endl;
        }
        // Try with backslash
        else
        {
            fullPath = directory + "\\" + texturePath;
            std::cout << "      Trying alternate path: " << fullPath << std::endl;
            
            if (texture.LoadFromFile(fullPath, true))
            {
                texture.Type = typeName;
                texture.Path = texturePath;
                textures.push_back(texture);
                texturesLoaded[texturePath] = texture;
                std::cout << "      [OK] Texture loaded successfully" << std::endl;
            }
            else
            {
                std::cerr << "      [ERROR] Failed to load texture: " << texturePath << std::endl;
            }
        }
    }

    return textures;
}
