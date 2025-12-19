#include "Skybox.h"
#include <iostream>

Skybox::Skybox() : VAO(0), VBO(0)
{
    setupSkybox();
}

void Skybox::setupSkybox()
{
    // Skybox vertices (cube centered at origin)
    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindVertexArray(0);
}

bool Skybox::Load(const std::string& directory, const std::string faces[6])
{
    std::string fullPaths[6];
    for (int i = 0; i < 6; i++)
    {
        fullPaths[i] = directory + "/" + faces[i];
    }
    return LoadFromPaths(fullPaths);
}

bool Skybox::LoadFromPaths(const std::string paths[6])
{
    bool success = cubemapTexture.LoadCubemap(paths);
    if (success)
    {
        std::cout << "Skybox loaded successfully" << std::endl;
    }
    else
    {
        std::cerr << "ERROR: Failed to load skybox cubemap" << std::endl;
    }
    return success;
}

void Skybox::Draw(unsigned int shaderProgram)
{
    glDepthFunc(GL_LEQUAL);  // Change depth function so skybox renders behind everything
    glUseProgram(shaderProgram);

    cubemapTexture.Bind(0);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthFunc(GL_LESS);  // Set depth function back to default
}

void Skybox::Delete()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    cubemapTexture.Delete();
}
