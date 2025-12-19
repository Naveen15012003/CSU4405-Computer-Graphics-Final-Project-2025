#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

// Phase 2 includes
#include "Camera.h"
#include "Model.h"
#include "Skybox.h"

// Window dimensions
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// FPS counter variables
double lastTime = 0.0;
int frameCount = 0;

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
std::string loadShaderFromFile(const char* filePath);
unsigned int compileShader(unsigned int type, const char* source);
unsigned int createShaderProgram(const char* vertexPath, const char* fragmentPath);
void updateFPS(GLFWwindow* window);

int main()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configure GLFW: OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Project - Phase 2", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Load OpenGL function pointers with GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLFW Version: " << glfwGetVersionString() << std::endl;
    std::cout << "=== Phase 2: Model Loading + Textures + Skybox ===" << std::endl;

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Build and compile shader programs
    std::cout << "\nLoading shaders..." << std::endl;
    
    // Phase 1 triangle shader (kept for compatibility)
    unsigned int triangleShader = createShaderProgram("shaders/vertex.glsl", "shaders/fragment.glsl");
    
    // Phase 2 model shader
    unsigned int modelShader = createShaderProgram("shaders/model.vert", "shaders/model.frag");
    
    // Phase 2 skybox shader
    unsigned int skyboxShader = createShaderProgram("shaders/skybox.vert", "shaders/skybox.frag");

    if (triangleShader == 0 || modelShader == 0 || skyboxShader == 0)
    {
        std::cerr << "Failed to create shader programs" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Phase 1: Triangle vertices (kept for compatibility)
    float triangleVertices[] = {
        // positions         // colors
        -0.8f, -0.8f, 0.0f,  1.0f, 0.0f, 0.0f,  // bottom left  - red
        -0.6f, -0.8f, 0.0f,  0.0f, 1.0f, 0.0f,  // bottom right - green
        -0.7f, -0.6f, 0.0f,  0.0f, 0.0f, 1.0f   // top          - blue
    };

    unsigned int triangleVAO, triangleVBO;
    glGenVertexArrays(1, &triangleVAO);
    glGenBuffers(1, &triangleVBO);

    glBindVertexArray(triangleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // Phase 2: Setup camera
    Camera camera(glm::vec3(3.0f, 3.0f, 5.0f));  // Position camera at an angle to see 3D shape better

    // Phase 2: Load model
    std::cout << "\nLoading model..." << std::endl;
    Model* model = nullptr;
    try
    {
        // Try to load a model - will create a simple test model if file doesn't exist
        model = new Model("assets/models/cube.obj");
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to load model: " << e.what() << std::endl;
        std::cerr << "Continuing without 3D model..." << std::endl;
    }

    // Phase 2: Load skybox
    std::cout << "\nLoading skybox..." << std::endl;
    Skybox skybox;
    std::string skyboxFaces[6] = {
        "right.jpg",
        "left.jpg",
        "top.jpg",
        "bottom.jpg",
        "front.jpg",
        "back.jpg"
    };
    
    bool skyboxLoaded = skybox.Load("assets/skybox", skyboxFaces);
    if (!skyboxLoaded)
    {
        std::cerr << "Failed to load skybox - continuing without it" << std::endl;
    }

    // Initialize FPS timer
    lastTime = glfwGetTime();

    std::cout << "\n=== Rendering Started ===" << std::endl;
    std::cout << "Controls: ESC to exit" << std::endl;

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        // Input
        processInput(window);

        // Clear buffers
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Projection and view matrices
        glm::mat4 projection = camera.GetProjectionMatrix((float)SCR_WIDTH / (float)SCR_HEIGHT);
        glm::mat4 view = camera.GetViewMatrix();

        // Phase 2: Render skybox first
        if (skyboxLoaded)
        {
            glUseProgram(skyboxShader);
            
            // Remove translation from view matrix for skybox
            glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
            
            glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "view"), 1, GL_FALSE, glm::value_ptr(skyboxView));
            glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniform1i(glGetUniformLocation(skyboxShader, "skybox"), 0);
            
            skybox.Draw(skyboxShader);
        }

        // Phase 2: Render 3D model
        if (model != nullptr)
        {
            glUseProgram(modelShader);

            // Set matrices
            glUniformMatrix4fv(glGetUniformLocation(modelShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniformMatrix4fv(glGetUniformLocation(modelShader, "view"), 1, GL_FALSE, glm::value_ptr(view));

            // Model matrix - rotate over time for demonstration
            glm::mat4 modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
            modelMatrix = glm::rotate(modelMatrix, (float)glfwGetTime() * glm::radians(30.0f), glm::vec3(0.5f, 1.0f, 0.0f));  // Rotate on multiple axes
            modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f));  // Scale down to 50% for better view

            glUniformMatrix4fv(glGetUniformLocation(modelShader, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

            // Draw model
            model->Draw(modelShader);
        }

        // Phase 1: Render original triangle (in corner)
        glUseProgram(triangleShader);
        glBindVertexArray(triangleVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);

        // Update FPS counter
        updateFPS(window);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    std::cout << "\nCleaning up..." << std::endl;
    
    glDeleteVertexArrays(1, &triangleVAO);
    glDeleteBuffers(1, &triangleVBO);
    glDeleteProgram(triangleShader);
    glDeleteProgram(modelShader);
    glDeleteProgram(skyboxShader);

    if (model != nullptr)
    {
        model->Delete();
        delete model;
    }

    skybox.Delete();

    glfwTerminate();
    
    std::cout << "Program terminated successfully" << std::endl;
    return 0;
}

// Process keyboard input
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// Callback for window resize
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Load shader source from file
std::string loadShaderFromFile(const char* filePath)
{
    std::ifstream shaderFile;
    std::stringstream shaderStream;

    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        shaderFile.open(filePath);
        shaderStream << shaderFile.rdbuf();
        shaderFile.close();
        return shaderStream.str();
    }
    catch (std::ifstream::failure& e)
    {
        std::cerr << "ERROR: Failed to read shader file: " << filePath << std::endl;
        return "";
    }
}

// Compile a shader and check for errors
unsigned int compileShader(unsigned int type, const char* source)
{
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    // Check for compilation errors
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR: Shader compilation failed\n" << infoLog << std::endl;
        return 0;
    }

    return shader;
}

// Create shader program from vertex and fragment shader files
unsigned int createShaderProgram(const char* vertexPath, const char* fragmentPath)
{
    // Load shader source code
    std::string vertexCode = loadShaderFromFile(vertexPath);
    std::string fragmentCode = loadShaderFromFile(fragmentPath);

    if (vertexCode.empty() || fragmentCode.empty())
    {
        std::cerr << "ERROR: Failed to load shader files" << std::endl;
        return 0;
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    // Compile shaders
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vShaderCode);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fShaderCode);

    if (vertexShader == 0 || fragmentShader == 0)
    {
        std::cerr << "ERROR: Shader compilation failed" << std::endl;
        return 0;
    }

    // Link shaders into program
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR: Shader program linking failed\n" << infoLog << std::endl;
        return 0;
    }

    // Delete shaders (no longer needed after linking)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    std::cout << "Shaders compiled and linked: " << vertexPath << ", " << fragmentPath << std::endl;
    return shaderProgram;
}

// Update FPS counter in window title (once per second)
void updateFPS(GLFWwindow* window)
{
    double currentTime = glfwGetTime();
    frameCount++;

    // Update title every second
    if (currentTime - lastTime >= 1.0)
    {
        std::string title = "OpenGL Project - Phase 2 - FPS: " + std::to_string(frameCount);
        glfwSetWindowTitle(window, title.c_str());

        frameCount = 0;
        lastTime = currentTime;
    }
}
