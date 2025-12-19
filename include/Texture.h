#pragma once

#include <glad/glad.h>
#include <string>

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

private:
    bool isCubemap;
};
