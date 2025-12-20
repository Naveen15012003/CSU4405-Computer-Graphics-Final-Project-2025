#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>

// Phase 2 + 3 includes
#include "Camera.h"
#include "Model.h"
#include "Skybox.h"
#include "ShadowMap.h"

// Window dimensions
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Shadow map resolution
const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

// FPS counter variables
double lastTime = 0.0;
int frameCount = 0;

// Phase 3 toggles
bool enableShadows = true;
bool enablePCF = true;
bool showDepthMap = false;
bool enableGammaCorrection = false;

// Key press tracking
bool f1Pressed = false;
bool f2Pressed = false;
bool f3Pressed = false;
bool f4Pressed = false;

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void processDebugKeys(GLFWwindow* window);
std::string loadShaderFromFile(const char* filePath);
unsigned int compileShader(unsigned int type, const char* source);
unsigned int createShaderProgram(const char* vertexPath, const char* fragmentPath);
void updateFPS(GLFWwindow* window);
void renderQuad();
void renderGroundPlane(unsigned int shader, const glm::mat4& model);

// Ground plane VAO
unsigned int groundPlaneVAO = 0;
unsigned int groundPlaneVBO = 0;

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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Project - Phase 3", NULL, NULL);
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

    std::cout << "\n??????????????????????????????????????????????" << std::endl;
    std::cout << "?  PHASE 3 - LIGHTING & SHADOW MAPPING                  ?" << std::endl;
    std::cout << "??????????????????????????????????????????????" << std::endl;
    std::cout << "\nOpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLFW Version: " << glfwGetVersionString() << std::endl;
    
    std::cout << "\n???????????????????????????????????" << std::endl;
    std::cout << "  CONTROLS" << std::endl;
    std::cout << "???????????????????????????????????" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;
    std::cout << "  F1  - Toggle shadows [" << (enableShadows ? "ON" : "OFF") << "]" << std::endl;
    std::cout << "  F2  - Toggle PCF filtering [" << (enablePCF ? "ON" : "OFF") << "]" << std::endl;
    std::cout << "  F3  - Show depth map debug view [" << (showDepthMap ? "ON" : "OFF") << "]" << std::endl;
    std::cout << "  F4  - Toggle gamma correction [" << (enableGammaCorrection ? "ON" : "OFF") << "]" << std::endl;
    std::cout << "???????????????????????????????????\n" << std::endl;
    
    std::cout << "Shadow Map Resolution: " << SHADOW_WIDTH << "x" << SHADOW_HEIGHT << std::endl;
    std::cout << "\n=== Phase 3: Blinn-Phong + Shadow Mapping (PCF) ===\n" << std::endl;

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Build and compile shader programs
    std::cout << "Loading shaders..." << std::endl;
    
    // Phase 3 shaders
    unsigned int modelShader = createShaderProgram("shaders/model.vert", "shaders/model.frag");
    unsigned int skyboxShader = createShaderProgram("shaders/skybox.vert", "shaders/skybox.frag");
    unsigned int shadowShader = createShaderProgram("shaders/shadow_depth.vert", "shaders/shadow_depth.frag");
    unsigned int debugDepthShader = createShaderProgram("shaders/debug_depth.vert", "shaders/debug_depth.frag");

    if (modelShader == 0 || skyboxShader == 0 || shadowShader == 0 || debugDepthShader == 0)
    {
        std::cerr << "Failed to create shader programs" << std::endl;
        glfwTerminate();
        return -1;
    }

    std::cout << "? All shaders compiled successfully\n" << std::endl;

    // Create shadow map
    ShadowMap shadowMap(SHADOW_WIDTH, SHADOW_HEIGHT);

    // Load model
    std::cout << "Loading model..." << std::endl;
    Model* model = nullptr;
    try {
        model = new Model("assets/models/cube.obj", "assets/textures/default.png");
        std::cout << "? Model loaded successfully\n" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load model: " << e.what() << std::endl;
        glfwTerminate();
        return -1;
    }

    // Load skybox
    std::cout << "Loading skybox..." << std::endl;
    Skybox* skybox = nullptr;
    try {
        skybox = new Skybox();
        std::string faces[6] = {
            "assets/skybox/right.jpg",
            "assets/skybox/left.jpg",
            "assets/skybox/top.jpg",
            "assets/skybox/bottom.jpg",
            "assets/skybox/front.jpg",
            "assets/skybox/back.jpg"
        };
        if (skybox->LoadFromPaths(faces))
        {
            std::cout << "? Skybox loaded\n" << std::endl;
        }
        else
        {
            delete skybox;
            skybox = nullptr;
            std::cout << "? Skybox not available (optional)\n" << std::endl;
        }
    }
    catch (...) {
        if (skybox) delete skybox;
        skybox = nullptr;
        std::cout << "? Skybox not available (optional)\n" << std::endl;
    }

    // Camera setup
    Camera camera(glm::vec3(0.0f, 2.0f, 5.0f));

    // Light setup
    glm::vec3 dirLightDir = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
    glm::vec3 dirLightColor(1.0f, 1.0f, 0.95f);
    
    glm::vec3 pointLightPos(2.0f, 3.0f, 2.0f);
    glm::vec3 pointLightColor(1.0f, 0.9f, 0.7f);

    std::cout << "=== Rendering Started ===" << std::endl;
    std::cout << "Scene configured with:" << std::endl;
    std::cout << "  • Directional light with shadows" << std::endl;
    std::cout << "  • Point light (no shadows)" << std::endl;
    std::cout << "  • Ground plane for shadow testing" << std::endl;
    std::cout << "  • Elevated cube casting shadow\n" << std::endl;

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        // Input
        processInput(window);
        processDebugKeys(window);

        // Update FPS
        updateFPS(window);

        // === PASS 1: SHADOW MAP (DEPTH PASS) ===
        
        // Light space matrix (orthographic for directional light)
        float near_plane = 0.5f, far_plane = 10.0f;
        glm::mat4 lightProjection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, near_plane, far_plane);
        glm::mat4 lightView = glm::lookAt(
            -dirLightDir * 5.0f,
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        // Render scene to shadow map
        shadowMap.BindForWriting();
        glCullFace(GL_FRONT);
        glUseProgram(shadowShader);
        glUniformMatrix4fv(glGetUniformLocation(shadowShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

        // Render ground plane
        glm::mat4 groundModel = glm::mat4(1.0f);
        groundModel = glm::scale(groundModel, glm::vec3(10.0f, 1.0f, 10.0f));
        renderGroundPlane(shadowShader, groundModel);

        // Render elevated cube
        glm::mat4 cubeModel = glm::mat4(1.0f);
        cubeModel = glm::translate(cubeModel, glm::vec3(0.0f, 1.5f, 0.0f));
        cubeModel = glm::rotate(cubeModel, (float)glfwGetTime() * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(shadowShader, "model"), 1, GL_FALSE, glm::value_ptr(cubeModel));
        model->Draw(shadowShader);

        shadowMap.Unbind();
        glCullFace(GL_BACK);

        // === PASS 2: MAIN RENDER OR DEBUG VIEW ===
        
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

        if (showDepthMap)
        {
            // Debug: Show depth map
            glDisable(GL_DEPTH_TEST);
            glUseProgram(debugDepthShader);
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, shadowMap.GetDepthTexture());
            glUniform1i(glGetUniformLocation(debugDepthShader, "shadowMap"), 0);
            
            renderQuad();
            
            glEnable(GL_DEPTH_TEST);
        }
        else
        {
            // Normal rendering
            
            // Matrices
            glm::mat4 projection = camera.GetProjectionMatrix((float)SCR_WIDTH / (float)SCR_HEIGHT);
            glm::mat4 view = camera.GetViewMatrix();

            // Render skybox first
            if (skybox)
            {
                glDepthFunc(GL_LEQUAL);
                glUseProgram(skyboxShader);
                glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
                glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "view"), 1, GL_FALSE, glm::value_ptr(skyboxView));
                glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
                skybox->Draw(skyboxShader);
                glDepthFunc(GL_LESS);
            }

            // Render scene with lighting and shadows
            glUseProgram(modelShader);

            // Set matrices
            glUniformMatrix4fv(glGetUniformLocation(modelShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniformMatrix4fv(glGetUniformLocation(modelShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(modelShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

            // Set lights
            glUniform3fv(glGetUniformLocation(modelShader, "dirLightDir"), 1, glm::value_ptr(dirLightDir));
            glUniform3fv(glGetUniformLocation(modelShader, "dirLightColor"), 1, glm::value_ptr(dirLightColor));
            
            glUniform3fv(glGetUniformLocation(modelShader, "pointLightPos"), 1, glm::value_ptr(pointLightPos));
            glUniform3fv(glGetUniformLocation(modelShader, "pointLightColor"), 1, glm::value_ptr(pointLightColor));
            glUniform1f(glGetUniformLocation(modelShader, "pointLightConstant"), 1.0f);
            glUniform1f(glGetUniformLocation(modelShader, "pointLightLinear"), 0.09f);
            glUniform1f(glGetUniformLocation(modelShader, "pointLightQuadratic"), 0.032f);

            // Set camera
            glUniform3fv(glGetUniformLocation(modelShader, "viewPos"), 1, glm::value_ptr(camera.Position));

            // Set shadow map
            shadowMap.BindForReading(GL_TEXTURE1);
            glUniform1i(glGetUniformLocation(modelShader, "shadowMap"), 1);

            // Set toggles
            glUniform1i(glGetUniformLocation(modelShader, "enableShadows"), enableShadows);
            glUniform1i(glGetUniformLocation(modelShader, "uUsePCF"), enablePCF);
            glUniform1i(glGetUniformLocation(modelShader, "enableGammaCorrection"), enableGammaCorrection);

            // Set material
            glUniform1f(glGetUniformLocation(modelShader, "material.shininess"), 32.0f);

            // Render ground plane
            renderGroundPlane(modelShader, groundModel);

            // Render elevated cube
            glUniformMatrix4fv(glGetUniformLocation(modelShader, "model"), 1, GL_FALSE, glm::value_ptr(cubeModel));
            model->Draw(modelShader);
        }

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    delete model;
    if (skybox) delete skybox;
    
    if (groundPlaneVAO != 0)
    {
        glDeleteVertexArrays(1, &groundPlaneVAO);
        glDeleteBuffers(1, &groundPlaneVBO);
    }

    glDeleteProgram(modelShader);
    glDeleteProgram(skyboxShader);
    glDeleteProgram(shadowShader);
    glDeleteProgram(debugDepthShader);

    glfwTerminate();
    std::cout << "\n? Application closed successfully" << std::endl;
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
    char infoLog[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 1024, NULL, infoLog);
        const char* shaderType = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
        std::cerr << "ERROR: " << shaderType << " Shader compilation failed\n" << infoLog << std::endl;
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
        std::cerr << "ERROR: Failed to load shader files: " << vertexPath << " or " << fragmentPath << std::endl;
        return 0;
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    // Compile shaders
    std::cout << "Compiling shaders: " << vertexPath << ", " << fragmentPath << std::endl;
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vShaderCode);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fShaderCode);

    if (vertexShader == 0 || fragmentShader == 0)
    {
        std::cerr << "ERROR: Shader compilation failed for: " << vertexPath << " or " << fragmentPath << std::endl;
        return 0;
    }

    // Link shaders into program
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    int success;
    char infoLog[1024];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 1024, NULL, infoLog);
        std::cerr << "ERROR: Shader program linking failed\n" << infoLog << std::endl;
        return 0;
    }

    // Delete shaders (no longer needed after linking)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    std::cout << "? Shaders compiled and linked: " << vertexPath << ", " << fragmentPath << std::endl;
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
        std::string title = "OpenGL Project - Phase 3 - FPS: " + std::to_string(frameCount);
        glfwSetWindowTitle(window, title.c_str());

        frameCount = 0;
        lastTime = currentTime;
    }
}

// Render a fullscreen quad (for debug view)
void renderQuad()
{
    static unsigned int quadVAO = 0, quadVBO;
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions    // texCoords
            -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
             1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
             1.0f,  1.0f, 0.0f,  1.0f, 1.0f
        };
        
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        
        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        // Texture coord attribute  
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
}

// Render the ground plane
void renderGroundPlane(unsigned int shader, const glm::mat4& model)
{
    if (groundPlaneVAO == 0)
    {
        // Generate buffers only once
        float planeVertices[] = {
            // Positions          // Normals           // TexCoords
            -5.0f, 0.0f, -5.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
             5.0f, 0.0f, -5.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
             5.0f, 0.0f,  5.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            -5.0f, 0.0f,  5.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f
        };

        glGenVertexArrays(1, &groundPlaneVAO);
        glGenBuffers(1, &groundPlaneVBO);
        glBindVertexArray(groundPlaneVAO);
        glBindBuffer(GL_ARRAY_BUFFER, groundPlaneVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    glBindVertexArray(groundPlaneVAO);
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
}

// Process debug keys (F1-F4)
void processDebugKeys(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS && !f1Pressed)
    {
        enableShadows = !enableShadows;
        std::cout << "Shadows " << (enableShadows ? "ENABLED" : "DISABLED") << std::endl;
        f1Pressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_RELEASE)
    {
        f1Pressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS && !f2Pressed)
    {
        enablePCF = !enablePCF;
        std::cout << "\n????????????????????????????????????????????????????" << std::endl;
        if (enablePCF) {
            std::cout << "?  PCF: ON  - 9x9 ULTRA SOFT SHADOWS (81 samples!)  ?" << std::endl;
            std::cout << "?  Shadow edges should be VERY BLURRY and SMOOTH     ?" << std::endl;
        } else {
            std::cout << "?  PCF: OFF - SHARP HARD SHADOWS (1 sample)          ?" << std::endl;
            std::cout << "?  Shadow edges should be CRISP and DEFINED          ?" << std::endl;
        }
        std::cout << "????????????????????????????????????????????????????\n" << std::endl;
        f2Pressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_RELEASE)
    {
        f2Pressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS && !f3Pressed)
    {
        showDepthMap = !showDepthMap;
        std::cout << "Depth Map Debug View " << (showDepthMap ? "ENABLED" : "DISABLED") << std::endl;
        f3Pressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_RELEASE)
    {
        f3Pressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_F4) == GLFW_PRESS && !f4Pressed)
    {
        enableGammaCorrection = !enableGammaCorrection;
        std::cout << "Gamma Correction " << (enableGammaCorrection ? "ENABLED" : "DISABLED") << std::endl;
        f4Pressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_F4) == GLFW_RELEASE)
    {
        f4Pressed = false;
    }
}
