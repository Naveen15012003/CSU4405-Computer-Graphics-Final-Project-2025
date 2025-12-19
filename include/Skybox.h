#pragma once

#include <glad/glad.h>
#include <string>
#include "Texture.h"

class Skybox
{
public:
    Texture cubemapTexture;

    // Constructor
    Skybox();

    // Load skybox from 6 images
    bool Load(const std::string& directory, const std::string faces[6]);

    // Load skybox from full paths
    bool LoadFromPaths(const std::string paths[6]);

    // Render the skybox
    void Draw(unsigned int shaderProgram);

    // Cleanup
    void Delete();

private:
    unsigned int VAO, VBO;
    void setupSkybox();
};
