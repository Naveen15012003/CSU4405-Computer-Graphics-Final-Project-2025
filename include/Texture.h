#pragma once

#include <glad/glad.h>
#include <string>
#include <vector>

class Texture
{
public:
    unsigned int ID;
    std::string Type;  // "diffuse", "specular", etc.
    std::string Path;

    // Constructor
    Texture();

    // Load texture from file
    bool LoadFromFile(const std::string& path, bool flipVertically = true);

    // Load cubemap from 6 faces
    bool LoadCubemap(const std::string faces[6]);

    // Bind texture
    void Bind(unsigned int slot = 0) const;

    // Unbind texture
    void Unbind() const;

    // Delete texture
    void Delete();

    // Helper: Get project root directory (repo root)
    static std::string GetProjectRoot();

    // Helper: Resolve path with multiple search locations
    static std::string ResolvePath(const std::string& relativePath, const std::vector<std::string>& searchPaths);
    
    // Helper: List all files in a directory
    static std::vector<std::string> ListFilesInDirectory(const std::string& directoryPath);
    
    // Helper: Auto-detect and load skybox from common naming conventions
    static bool LoadSkyboxAuto(const std::string& skyboxDirectory, Texture& cubemapTexture);

private:
    bool isCubemap;
    
    // Helper: Load single texture file with path resolution
    bool loadTextureFile(const std::string& path, int& width, int& height, int& nrChannels, unsigned char** data, bool flipVertically);
};
