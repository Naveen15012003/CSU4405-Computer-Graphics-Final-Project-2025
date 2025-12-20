#pragma once

#include <vector>
#include <string>
#include <map>
#include "Mesh.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class Model
{
public:
    // Model data
    std::vector<Mesh> meshes;
    std::string directory;
    bool usingFallbackTexture;

    // Constructor
    Model(const std::string& path, const std::string& fallbackTexturePath = "");

    // Draw the model
    void Draw(unsigned int shaderProgram);

    // Cleanup
    void Delete();

private:
    // Texture cache to avoid loading duplicates
    std::map<std::string, Texture> texturesLoaded;
    Texture fallbackTexture;
    bool hasFallbackTexture;
    std::string fallbackTexturePath;

    // Load model with Assimp
    void loadModel(const std::string& path);

    // Process a node in Assimp's scene structure
    void processNode(aiNode* node, const aiScene* scene);

    // Process a mesh
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);

    // Load material textures
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
    
    // Load fallback texture
    bool loadFallbackTexture(const std::string& path);
    
    // Apply fallback texture to mesh if needed
    void applyFallbackTextureIfNeeded(std::vector<Texture>& textures);
};
