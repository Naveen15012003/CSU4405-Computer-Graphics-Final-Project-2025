#include "Model.h"
#include <iostream>

Model::Model(const std::string& path)
{
    loadModel(path);
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

    std::cout << "Model loading: " << path << std::endl;
    std::cout << "  Meshes: " << scene->mNumMeshes << std::endl;
    std::cout << "  Materials: " << scene->mNumMaterials << std::endl;

    processNode(scene->mRootNode, scene);

    std::cout << "Model loaded successfully: " << meshes.size() << " meshes" << std::endl;
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

        // Diffuse maps
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    }

    return Mesh(vertices, indices, textures);
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
{
    std::vector<Texture> textures;

    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);
        std::string texturePath = std::string(str.C_Str());

        // Check if texture was loaded before
        if (texturesLoaded.find(texturePath) != texturesLoaded.end())
        {
            textures.push_back(texturesLoaded[texturePath]);
            continue;
        }

        // Load new texture
        Texture texture;
        std::string fullPath = directory + "/" + texturePath;

        // Try with backslash as well
        if (!texture.LoadFromFile(fullPath, true))
        {
            fullPath = directory + "\\" + texturePath;
            if (!texture.LoadFromFile(fullPath, true))
            {
                std::cerr << "Failed to load texture: " << texturePath << std::endl;
                continue;
            }
        }

        texture.Type = typeName;
        texture.Path = texturePath;
        textures.push_back(texture);
        texturesLoaded[texturePath] = texture;
    }

    return textures;
}
