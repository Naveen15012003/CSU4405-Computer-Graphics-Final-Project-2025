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

// Phase 2 + 3 + 4 includes
#include "Camera.h"
#include "Model.h"
#include "Skybox.h"
#include "ShadowMap.h"
#include "HUD.h"

// Window dimensions
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Shadow map resolution
const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

// FPS counter variables
double lastTime = 0.0;
int frameCount = 0;
int currentFPS = 0;

// Phase 3 toggles
bool enableShadows = true;
bool enablePCF = true;
bool showDepthMap = false;
bool enableGammaCorrection = false;

// Phase 4 additions
bool mouseCaptured = true;
bool animationPaused = false;
float cubeRotationAngle = 0.0f;

// Key press tracking
bool f1Pressed = false;
bool f2Pressed = false;
bool f3Pressed = false;
bool f4Pressed = false;
bool rPressed = false;
bool spacePressed = false;
bool pPressed = false;

// Light control variables
glm::vec3 lightDirection = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
float lightAzimuth = 0.0f;
float lightElevation = -45.0f;

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window, Camera& camera, float deltaTime);
void processDebugKeys(GLFWwindow* window);
void processLightControls(GLFWwindow* window);
std::string loadShaderFromFile(const char* filePath);
unsigned int compileShader(unsigned int type, const char* source);
unsigned int createShaderProgram(const char* vertexPath, const char* fragmentPath);
void updateFPS(GLFWwindow* window);
void renderQuad();
void renderGroundPlane(unsigned int shader, const glm::mat4& model);
void updateLightDirection();

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

    std::cout << "\n===============================================" << std::endl;
    std::cout << "|  PHASE 4 - COMPLETE INTERACTIVE DEMO       |" << std::endl;
    std::cout << "|  (Lighting + Shadows + Controls + HUD)     |" << std::endl;
    std::cout << "===============================================" << std::endl;
    std::cout << "\nOpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLFW Version: " << glfwGetVersionString() << std::endl;
    
    std::cout << "\n===================================" << std::endl;
    std::cout << "  CONTROLS (See docs/CONTROLS.md)" << std::endl;
    std::cout << "===================================" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;
    std::cout << "  WASD - Move camera" << std::endl;
    std::cout << "  SHIFT - Speed boost | CTRL - Precision" << std::endl;
    std::cout << "  R - Reset camera | SPACE - Toggle mouse" << std::endl;
    std::cout << "  P - Pause animation" << std::endl;
    std::cout << "  F1-F4 - Toggle features" << std::endl;
    std::cout << "  Arrow Keys/[/] - Adjust light" << std::endl;
    std::cout << "===================================" << std::endl;

    std::cout << "Shadow Map Resolution: " << SHADOW_WIDTH << "x" << SHADOW_HEIGHT << std::endl;
    std::cout << "\n=== Phase 4: Interactive Demo + HUD + Controls ===\n" << std::endl;

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

    std::cout << "[OK] All shaders compiled successfully\n" << std::endl;

    // Phase 4: Initialize HUD
    HUD hud;
    hud.Initialize();
    std::cout << "[OK] HUD system initialized\n" << std::endl;

    // Create shadow map
    ShadowMap shadowMap(SHADOW_WIDTH, SHADOW_HEIGHT);

    // Load model
    std::cout << "Loading model..." << std::endl;
    Model* model = nullptr;
    try {
        model = new Model("assets/models/cube.obj", "assets/textures/default.png");
        std::cout << "[OK] Model loaded successfully\n" << std::endl;
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
            std::cout << "[OK] Skybox loaded\n" << std::endl;
        }
        else
        {
            delete skybox;
            skybox = nullptr;
            std::cout << "[WARN] Skybox not available (optional)\n" << std::endl;
        }
    }
    catch (...) {
        if (skybox) delete skybox;
        skybox = nullptr;
        std::cout << "[WARN] Skybox not available (optional)\n" << std::endl;
    }

    // Camera setup
    Camera camera(glm::vec3(0.0f, 2.0f, 5.0f));

    // Phase 4: Enable mouse capture by default
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    // Light setup - using global lightDirection variable
    glm::vec3 dirLightColor(1.0f, 1.0f, 0.95f);
    
    glm::vec3 pointLightPos(2.0f, 3.0f, 2.0f);
    glm::vec3 pointLightColor(1.0f, 0.9f, 0.7f);

    std::cout << "=== Rendering Started ===" << std::endl;
    std::cout << "Scene configured with:" << std::endl;
    std::cout << "  * Directional light with shadows" << std::endl;
    std::cout << "  * Point light (no shadows)" << std::endl;
    std::cout << "  * Ground plane for shadow testing" << std::endl;
    std::cout << "  * Multiple cubes casting shadows" << std::endl;
    std::cout << "  * On-screen HUD overlay\n" << std::endl;

    // Phase 4: Delta time tracking
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        // Calculate delta time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Update animation
        if (!animationPaused) {
            cubeRotationAngle += deltaTime * 0.5f;
        }

        // Input
        processInput(window, camera, deltaTime);
        processDebugKeys(window);
        processLightControls(window);

        // Update FPS
        updateFPS(window);

        // === PASS 1: SHADOW MAP (DEPTH PASS) ===
        
        // Light space matrix (orthographic for directional light)
        // Use the global lightDirection variable
        float near_plane = 0.5f, far_plane = 10.0f;
        glm::mat4 lightProjection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, near_plane, far_plane);
        glm::mat4 lightView = glm::lookAt(
            -lightDirection * 5.0f,
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

        // Render main animated cube (center)
        glm::mat4 cubeModel = glm::mat4(1.0f);
        cubeModel = glm::translate(cubeModel, glm::vec3(0.0f, 1.5f, 0.0f));
        cubeModel = glm::rotate(cubeModel, cubeRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(shadowShader, "model"), 1, GL_FALSE, glm::value_ptr(cubeModel));
        model->Draw(shadowShader);

        // Phase 4: Add extra cubes for better shadow demonstration
        // Cube 2 (left)
        glm::mat4 cube2Model = glm::mat4(1.0f);
        cube2Model = glm::translate(cube2Model, glm::vec3(-3.0f, 1.0f, -2.0f));
        cube2Model = glm::rotate(cube2Model, cubeRotationAngle * 0.5f, glm::vec3(1.0f, 0.5f, 0.0f));
        cube2Model = glm::scale(cube2Model, glm::vec3(0.8f));
        glUniformMatrix4fv(glGetUniformLocation(shadowShader, "model"), 1, GL_FALSE, glm::value_ptr(cube2Model));
        model->Draw(shadowShader);

        // Cube 3 (right)
        glm::mat4 cube3Model = glm::mat4(1.0f);
        cube3Model = glm::translate(cube3Model, glm::vec3(3.0f, 0.8f, 1.0f));
        cube3Model = glm::rotate(cube3Model, cubeRotationAngle * -0.7f, glm::vec3(0.0f, 1.0f, 1.0f));
        cube3Model = glm::scale(cube3Model, glm::vec3(0.6f));
        glUniformMatrix4fv(glGetUniformLocation(shadowShader, "model"), 1, GL_FALSE, glm::value_ptr(cube3Model));
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

            // Set lights (use global lightDirection variable)
            glUniform3fv(glGetUniformLocation(modelShader, "dirLightDir"), 1, glm::value_ptr(lightDirection));
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

            // Render main animated cube
            glUniformMatrix4fv(glGetUniformLocation(modelShader, "model"), 1, GL_FALSE, glm::value_ptr(cubeModel));
            model->Draw(modelShader);

            // Render extra cubes
            glUniformMatrix4fv(glGetUniformLocation(modelShader, "model"), 1, GL_FALSE, glm::value_ptr(cube2Model));
            model->Draw(modelShader);
            
            glUniformMatrix4fv(glGetUniformLocation(modelShader, "model"), 1, GL_FALSE, glm::value_ptr(cube3Model));
            model->Draw(modelShader);
        }

        // Phase 4: Render HUD overlay
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        float hudY = 580.0f;
        float hudScale = 1.5f;
        glm::vec3 hudColor(0.0f, 1.0f, 0.0f);  // Green

        hud.RenderText("FPS: " + std::to_string(currentFPS), 10.0f, hudY, hudScale, hudColor);
        hudY -= 20.0f;
        hud.RenderText("Shadows: " + std::string(enableShadows ? "ON" : "OFF"), 10.0f, hudY, hudScale, hudColor);
        hudY -= 20.0f;
        hud.RenderText("PCF: " + std::string(enablePCF ? "ON" : "OFF"), 10.0f, hudY, hudScale, hudColor);
        hudY -= 20.0f;
        hud.RenderText("Debug: " + std::string(showDepthMap ? "ON" : "OFF"), 10.0f, hudY, hudScale, hudColor);
        hudY -= 20.0f;
        hud.RenderText("Gamma: " + std::string(enableGammaCorrection ? "ON" : "OFF"), 10.0f, hudY, hudScale, hudColor);
        hudY -= 20.0f;
        
        char posBuf[64];
        snprintf(posBuf, sizeof(posBuf), "Cam: (%.1f, %.1f, %.1f)", 
                 camera.Position.x, camera.Position.y, camera.Position.z);
        hud.RenderText(posBuf, 10.0f, hudY, hudScale, hudColor);
        hudY -= 20.0f;
        
        hud.RenderText("Mouse: " + std::string(mouseCaptured ? "Captured" : "Free"), 
                      10.0f, hudY, hudScale, hudColor);
        
        if (animationPaused) {
            hudY -= 20.0f;
            hud.RenderText("[PAUSED]", 10.0f, hudY, hudScale, glm::vec3(1.0f, 1.0f, 0.0f));
        }

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    delete model;
    if (skybox) delete skybox;
    
    hud.Cleanup();
    
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
    std::cout << "\n[OK] Application closed successfully" << std::endl;
    return 0;
}

// Process keyboard input
void processInput(GLFWwindow* window, Camera& camera, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    // Phase 4: Speed modifiers
    float speed = camera.MovementSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        speed *= 2.0f;  // Speed boost
    if (glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
        speed *= 0.3f;  // Precision mode
    
    // Direct camera movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.Position += speed * camera.Front;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.Position -= speed * camera.Front;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.Position -= speed * camera.Right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.Position += speed * camera.Right;
    
    // Phase 4: Reset camera (R key)
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !rPressed) {
        camera.Position = glm::vec3(0.0f, 2.0f, 5.0f);
        camera.Yaw = -90.0f;
        camera.Pitch = 0.0f;
        // Call updateCameraVectors through a public method or directly modify
        camera.Front = glm::vec3(0.0f, 0.0f, -1.0f);
        camera.Right = glm::normalize(glm::cross(camera.Front, camera.WorldUp));
        camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));
        std::cout << "Camera reset to default position" << std::endl;
        rPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) rPressed = false;
    
    // Phase 4: Toggle mouse capture (SPACE)
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !spacePressed) {
        mouseCaptured = !mouseCaptured;
        glfwSetInputMode(window, GLFW_CURSOR, 
            mouseCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        std::cout << "Mouse " << (mouseCaptured ? "captured" : "released") << std::endl;
        spacePressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) spacePressed = false;
    
    // Phase 4: Pause animation (P key)
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !pPressed) {
        animationPaused = !animationPaused;
        std::cout << "Animation " << (animationPaused ? "PAUSED" : "RESUMED") << std::endl;
        pPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) pPressed = false;
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

    std::cout << "[OK] Shaders compiled and linked: " << vertexPath << ", " << fragmentPath << std::endl;
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
        std::string title = "OpenGL Project - Phase 4 - FPS: " + std::to_string(frameCount);
        glfwSetWindowTitle(window, title.c_str());

        currentFPS = frameCount; // Update currentFPS variable

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
        std::cout << "\n=====================================================" << std::endl;
        if (enablePCF) {
            std::cout << "|  PCF: ON  - 7x7 Soft Shadows (49 samples)       |" << std::endl;
            std::cout << "|  Shadow edges = SMOOTH and REALISTIC             |" << std::endl;
            std::cout << "|  Noticeable blur with gradual transition         |" << std::endl;
        } else {
            std::cout << "|  PCF: OFF - Hard Shadows (1 sample)              |" << std::endl;
            std::cout << "|  Shadow edges = SHARP and CRISP                  |" << std::endl;
            std::cout << "|  Clean cut with no blur                          |" << std::endl;
        }
        std::cout << "=====================================================\n" << std::endl;
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

// Update light direction based on azimuth and elevation
void updateLightDirection()
{
    float azimuthRad = glm::radians(lightAzimuth);
    float elevationRad = glm::radians(lightElevation);
    
    lightDirection.x = cos(elevationRad) * sin(azimuthRad);
    lightDirection.y = sin(elevationRad);
    lightDirection.z = cos(elevationRad) * cos(azimuthRad);
    lightDirection = glm::normalize(lightDirection);
    
    // Throttle console output (only print occasionally)
    static double lastPrint = 0.0;
    double now = glfwGetTime();
    if (now - lastPrint > 0.2) {
        char buf[128];
        snprintf(buf, sizeof(buf), "Light: (%.2f, %.2f, %.2f) | Az: %.1f Elev: %.1f", 
                 lightDirection.x, lightDirection.y, lightDirection.z, 
                 lightAzimuth, lightElevation);
        std::cout << buf << std::endl;
        lastPrint = now;
    }
}

// Phase 4: Process light direction controls
void processLightControls(GLFWwindow* window)
{
    bool changed = false;
    
    // Arrow keys rotate light azimuth
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        lightAzimuth -= 2.0f;
        changed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        lightAzimuth += 2.0f;
        changed = true;
    }
    
    // Up/Down arrows change elevation
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        lightElevation += 1.0f;
        if (lightElevation > -5.0f) lightElevation = -5.0f;  // Keep some downward angle
        changed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        lightElevation -= 1.0f;
        if (lightElevation < -89.0f) lightElevation = -89.0f;
        changed = true;
    }
    
    // Bracket keys for fine elevation adjustment
    if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS) {
        lightElevation -= 0.5f;
        if (lightElevation < -89.0f) lightElevation = -89.0f;
        changed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) {
        lightElevation += 0.5f;
        if (lightElevation > -5.0f) lightElevation = -5.0f;
        changed = true;
    }
    
    // Wrap azimuth to keep it in range
    if (lightAzimuth >= 360.0f) lightAzimuth -= 360.0f;
    if (lightAzimuth < 0.0f) lightAzimuth += 360.0f;
    
    if (changed) {
        updateLightDirection();
    }
}
