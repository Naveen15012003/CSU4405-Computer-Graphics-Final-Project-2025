#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "Texture.h"

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

class Mesh
{
public:
    // Mesh data
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    // Constructor
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);

    // Render the mesh
    void Draw(unsigned int shaderProgram);

    // Cleanup
    void Delete();

private:
    // Render data
    unsigned int VAO, VBO, EBO;

    // Setup mesh
    void setupMesh();
};
