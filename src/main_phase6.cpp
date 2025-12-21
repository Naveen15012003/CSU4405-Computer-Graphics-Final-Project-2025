#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>

// Phase 2 + 3 + 4 + 5 + 6 includes
#include "Camera.h"
#include "Model.h"
#include "Skybox.h"
#include "ShadowMap.h"
#include "HUD.h"
#include "PostProcessor.h"
#include "City.h"
#include "SkyboxAtlas.h"

// Window dimensions
const unsigned int SCR_WIDTH = 1920;  // Increased from 800 to 1920 (Full HD width)
const unsigned int SCR_HEIGHT = 1080; // Increased from 600 to 1080 (Full HD height)

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

// Phase 5 additions
bool enablePostProcessing = true;  // POST-PROCESSING NOW ON BY DEFAULT
bool enableBloom = true;
float exposure = 1.0f;
const float EXPOSURE_STEP = 0.1f;
float bloomStrength = 0.4f;
const float BLOOM_STRENGTH_STEP = 0.05f;
float bloomThreshold = 1.0f;
const float THRESHOLD_STEP = 0.1f;
int debugViewMode = 0; // 0=normal, 1=HDR only, 2=bright pass, 3=bloom blur

// Phase 6 additions
bool enableCity = true;
bool useSkyboxAtlas = false; // DEFAULT TO CUBEMAP for final demo (most robust)
bool cPressed = false;
bool kPressed = false;

// Key press tracking
bool f1Pressed = false;
bool f2Pressed = false;
bool f3Pressed = false;
bool f4Pressed = false;
bool f5Pressed = false;
bool f6Pressed = false;
bool f7Pressed = false;
bool f8Pressed = false;
bool bPressed = false;
bool oPressed = false;
bool vPressed = false;  // Changed from dPressed to vPressed
bool tPressed = false;
bool gPressed = false;
bool leftBracketPressed = false;
bool rightBracketPressed = false;
bool plusPressed = false;
bool minusPressed = false;
bool rPressed = false;
bool spacePressed = false;
bool pPressed = false;

// Light control variables
glm::vec3 lightDirection = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
float lightAzimuth = 0.0f;
float lightElevation = -45.0f;

// Mouse control variables
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
Camera* g_camera = nullptr; // Global camera pointer for callbacks

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window, Camera& camera, float deltaTime);
void processDebugKeys(GLFWwindow* window, SkyboxAtlas* skyboxAtlas);
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
unsigned int groundPlaneTexture = 0; // Dedicated ground texture

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
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Load OpenGL function pointers with GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    std::cout << "\n====================================================" << std::endl;
    std::cout << "|  PHASE 6 - LAB2 INTEGRATION + PROCEDURAL CITY   |" << std::endl;
    std::cout << "|  (Textured Buildings + Atlas Skybox)            |" << std::endl;
    std::cout << "====================================================" << std::endl;

    std::cout << "\nOpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLFW Version: " << glfwGetVersionString() << std::endl;
    
    std::cout << "\n===================================" << std::endl;
    std::cout << "  HDR + BLOOM CONTROLS" << std::endl;
    std::cout << "===================================" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;
    std::cout << "  WASD - Move camera" << std::endl;
    std::cout << "  SHIFT - Speed boost | CTRL - Precision" << std::endl;
    std::cout << "  R - Reset camera | SPACE - Toggle mouse" << std::endl;
    std::cout << "  P - Pause animation" << std::endl;
    std::cout << "\n  POST-PROCESSING:" << std::endl;
    std::cout << "  O/F5 - Toggle Post-Processing" << std::endl;
    std::cout << "  B/F6 - Toggle Bloom" << std::endl;
    std::cout << "  +/-  - Exposure adjust" << std::endl;
    std::cout << "  [/]  - Bloom strength" << std::endl;
    std::cout << "  T/G  - Bloom threshold" << std::endl;
    std::cout << "  V    - Cycle debug views" << std::endl;
    std::cout << "  F4   - Toggle Gamma" << std::endl;
    std::cout << "\n  PHASE 6 (LAB2):" << std::endl;
    std::cout << "  C    - Toggle City ON/OFF" << std::endl;
    std::cout << "  K    - Toggle Skybox (Cubemap/Atlas)" << std::endl;
    std::cout << "\n  SHADOWS:" << std::endl;
    std::cout << "  F1 - Toggle shadows" << std::endl;
    std::cout << "  F2 - Toggle PCF (soft shadows)" << std::endl;
    std::cout << "  F3 - Toggle depth map debug" << std::endl;
    std::cout << "  Arrow Keys/[/] - Adjust light" << std::endl;
    std::cout << "===================================" << std::endl;

    std::cout << "Shadow Map Resolution: " << SHADOW_WIDTH << "x" << SHADOW_HEIGHT << std::endl;
    std::cout << "\n=== Phase 5: HDR + Tone Mapping + Bloom ===\n" << std::endl;

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

    // Phase 5: Initialize Post-Processor
    PostProcessor postProcessor(SCR_WIDTH, SCR_HEIGHT);
    postProcessor.Initialize();

    // Phase 6: Initialize City and SkyboxAtlas
    std::cout << "Loading Phase 6 components..." << std::endl;
    
    unsigned int buildingShader = createShaderProgram("shaders/building.vert", "shaders/building.frag");
    unsigned int skyboxAtlasShader = createShaderProgram("shaders/skybox_atlas.vert", "shaders/skybox_atlas.frag");
    
    if (buildingShader == 0 || skyboxAtlasShader == 0)
    {
        std::cerr << "Failed to create Phase 6 shader programs" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    City city;
    city.Initialize(buildingShader);
    std::cout << "[OK] City system initialized\n" << std::endl;
    
    SkyboxAtlas* skyboxAtlas = new SkyboxAtlas();
    bool atlasAvailable = skyboxAtlas->LoadFromAtlas("assets/skybox/skybox_atlas.jpg");
    
    if (!atlasAvailable || !skyboxAtlas->IsInitialized())
    {
        std::cout << "[WARN] Skybox atlas not available - defaulting to CUBEMAP mode" << std::endl;
        useSkyboxAtlas = false; // Force cubemap mode if atlas missing
    }
    
    std::cout << "[OK] Skybox atlas system initialized (Available: " << (atlasAvailable ? "YES" : "NO - using fallback") << ")\n" << std::endl;

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
    Camera camera(glm::vec3(0.0f, 5.0f, 15.0f));  // Zoomed out even more: Y=5, Z=15 (was Y=4, Z=12)
    g_camera = &camera; // Set global pointer for mouse callbacks

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
        processDebugKeys(window, skyboxAtlas);
        processLightControls(window);

        // Update FPS
        updateFPS(window);

        // Phase 5: Begin post-processing render (if enabled AND not in debug mode)
        bool usePostProcessing = enablePostProcessing && postProcessor.IsInitialized() && !showDepthMap;
        if (usePostProcessing) {
            postProcessor.BeginRender();
        }

        // === PASS 1: SHADOW MAP (DEPTH PASS) ===
        // CRITICAL: Shadow pass must NOT affect the current framebuffer clear
        
        // Light space matrix (orthographic for directional light)
        // Use the global lightDirection variable
        // EXPANDED: Larger frustum to cover city buildings
        float near_plane = 0.5f, far_plane = 50.0f;  // Increased from 10.0f to 50.0f
        glm::mat4 lightProjection = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, near_plane, far_plane);  // Expanded from -5/-5 to -50/-50
        glm::mat4 lightView = glm::lookAt(
            -lightDirection * 25.0f,  // Increased from 5.0f to 25.0f
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

        // Phase 6: Render city buildings in shadow pass
        if (enableCity)
        {
            // CRITICAL: Buildings must cast shadows
            // Use dedicated shadow rendering method for efficiency
            city.RenderShadow(lightSpaceMatrix, shadowShader);
        }

        shadowMap.Unbind();
        glCullFace(GL_BACK);

        // === PASS 2: MAIN RENDER OR DEBUG VIEW ===
        
        // Set up the target framebuffer for scene rendering
        if (!usePostProcessing) {
            // Render directly to screen
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
            glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
        }
        // If using post-processing, HDR FBO is already bound by BeginRender()

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

            // Render skybox first (choose mode)
            if (useSkyboxAtlas && skyboxAtlas->IsInitialized())
            {
                // Lab2-style atlas skybox
                glDepthFunc(GL_LEQUAL);
                glUseProgram(skyboxAtlasShader);
                glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
                glUniformMatrix4fv(glGetUniformLocation(skyboxAtlasShader, "view"), 1, GL_FALSE, glm::value_ptr(skyboxView));
                glUniformMatrix4fv(glGetUniformLocation(skyboxAtlasShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
                skyboxAtlas->Draw(skyboxAtlasShader);
                glDepthFunc(GL_LESS);
            }
            else if (skybox)
            {
                // Original cubemap skybox
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
            
            // Set bloom threshold for MRT
            glUniform1f(glGetUniformLocation(modelShader, "bloomThreshold"), bloomThreshold);

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
            
            // Phase 6: Render city
            if (enableCity)
            {
                glUseProgram(buildingShader);
                
                // Set matrices for buildings
                glUniformMatrix4fv(glGetUniformLocation(buildingShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
                glUniformMatrix4fv(glGetUniformLocation(buildingShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
                glUniformMatrix4fv(glGetUniformLocation(buildingShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
                
                // Set lights
                glUniform3fv(glGetUniformLocation(buildingShader, "dirLightDir"), 1, glm::value_ptr(lightDirection));
                glUniform3fv(glGetUniformLocation(buildingShader, "dirLightColor"), 1, glm::value_ptr(dirLightColor));
                glUniform3fv(glGetUniformLocation(buildingShader, "pointLightPos"), 1, glm::value_ptr(pointLightPos));
                glUniform3fv(glGetUniformLocation(buildingShader, "pointLightColor"), 1, glm::value_ptr(pointLightColor));
                glUniform1f(glGetUniformLocation(buildingShader, "pointLightConstant"), 1.0f);
                glUniform1f(glGetUniformLocation(buildingShader, "pointLightLinear"), 0.09f);
                glUniform1f(glGetUniformLocation(buildingShader, "pointLightQuadratic"), 0.032f);
                
                // Set camera and shadow
                glUniform3fv(glGetUniformLocation(buildingShader, "viewPos"), 1, glm::value_ptr(camera.Position));
                shadowMap.BindForReading(GL_TEXTURE1);
                glUniform1i(glGetUniformLocation(buildingShader, "shadowMap"), 1);
                
                // Set toggles
                glUniform1i(glGetUniformLocation(buildingShader, "enableShadows"), enableShadows);
                glUniform1i(glGetUniformLocation(buildingShader, "uUsePCF"), enablePCF);
                glUniform1f(glGetUniformLocation(buildingShader, "bloomThreshold"), bloomThreshold);
                
                // Render city
                city.Render(view, projection, lightSpaceMatrix, camera.Position);
            }
        }

        // Phase 5: End post-processing render and apply effects (if it was started)
        if (usePostProcessing) {
            postProcessor.EndRender();
            postProcessor.Render(exposure, enableBloom, enableGammaCorrection, bloomStrength, debugViewMode);
        }

        // Phase 4: Render HUD overlay (always on top)
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        float hudY = 580.0f;
        float hudScale = 1.3f;
        glm::vec3 hudColor(0.0f, 1.0f, 0.0f);  // Green

        hud.RenderText("FPS: " + std::to_string(currentFPS), 10.0f, hudY, hudScale, hudColor);
        hudY -= 18.0f;
        
        // Phase 5: HDR Post-processing info
        hud.RenderText("Post: " + std::string(enablePostProcessing ? "ON" : "OFF") + " (O)", 10.0f, hudY, hudScale, hudColor);
        hudY -= 18.0f;
        hud.RenderText("Bloom: " + std::string(enableBloom ? "ON" : "OFF") + " (B)", 10.0f, hudY, hudScale, hudColor);
        hudY -= 18.0f;
        
        char expBuf[32];
        snprintf(expBuf, sizeof(expBuf), "Exposure: %.1f (+/-)", exposure);
        hud.RenderText(expBuf, 10.0f, hudY, hudScale, hudColor);
        hudY -= 18.0f;
        
        char bloomStrBuf[32];
        snprintf(bloomStrBuf, sizeof(bloomStrBuf), "Bloom Str: %.2f ([/])", bloomStrength);
        hud.RenderText(bloomStrBuf, 10.0f, hudY, hudScale, hudColor);
        hudY -= 18.0f;
        
        char threshBuf[32];
        snprintf(threshBuf, sizeof(threshBuf), "Threshold: %.1f (T/G)", bloomThreshold);
        hud.RenderText(threshBuf, 10.0f, hudY, hudScale, hudColor);
        hudY -= 18.0f;
        
        const char* debugModes[] = {"Normal", "HDR Only", "Bright", "Bloom"};
        hud.RenderText(std::string("View: ") + debugModes[debugViewMode] + " (V)", 10.0f, hudY, hudScale, hudColor);
        hudY -= 18.0f;
        
        // Phase 6: City and Skybox mode
        hud.RenderText("City: " + std::string(enableCity ? "ON" : "OFF") + " (C)", 10.0f, hudY, hudScale, hudColor);
        hudY -= 18.0f;
        
        std::string skyboxModeText = "Skybox: ";
        if (useSkyboxAtlas && skyboxAtlas->IsInitialized()) {
            skyboxModeText += "Atlas";
        } else if (!useSkyboxAtlas) {
            skyboxModeText += "Cubemap";
        } else {
            skyboxModeText += "Cubemap (Atlas N/A)";
        }
        skyboxModeText += " (K)";
        hud.RenderText(skyboxModeText, 10.0f, hudY, hudScale, hudColor);
        hudY -= 18.0f;
        
        hud.RenderText("Shadows: " + std::string(enableShadows ? "ON" : "OFF") + " (F1)", 10.0f, hudY, hudScale, hudColor);
        hudY -= 18.0f;
        hud.RenderText("PCF: " + std::string(enablePCF ? "ON" : "OFF") + " (F2)", 10.0f, hudY, hudScale, hudColor);
        hudY -= 18.0f;
        hud.RenderText("Gamma: " + std::string(enableGammaCorrection ? "ON" : "OFF") + " (F4)", 10.0f, hudY, hudScale, hudColor);
        hudY -= 18.0f;
        
        char posBuf[64];
        snprintf(posBuf, sizeof(posBuf), "Cam: (%.1f, %.1f, %.1f)", 
                 camera.Position.x, camera.Position.y, camera.Position.z);
        hud.RenderText(posBuf, 10.0f, hudY, hudScale * 0.9f, hudColor);
        hudY -= 18.0f;
        
        hud.RenderText("Mouse: " + std::string(mouseCaptured ? "Captured" : "Free") + " (SPC)", 
                      10.0f, hudY, hudScale * 0.9f, hudColor);
        
        if (animationPaused) {
            hudY -= 18.0f;
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
    if (skyboxAtlas) delete skyboxAtlas;
    
    city.Cleanup();
    hud.Cleanup();
    postProcessor.Cleanup();
    
    if (groundPlaneVAO != 0)
    {
        glDeleteVertexArrays(1, &groundPlaneVAO);
        glDeleteBuffers(1, &groundPlaneVBO);
    }
    
    if (groundPlaneTexture != 0)
    {
        glDeleteTextures(1, &groundPlaneTexture);
    }

    glDeleteProgram(modelShader);
    glDeleteProgram(skyboxShader);
    glDeleteProgram(shadowShader);
    glDeleteProgram(debugDepthShader);
    glDeleteProgram(buildingShader);
    glDeleteProgram(skyboxAtlasShader);

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

// Mouse movement callback
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    if (!g_camera) return; // Safety check
    
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    // Only process mouse movement if mouse is captured
    if (mouseCaptured)
    {
        g_camera->ProcessMouseMovement(xoffset, yoffset);
    }
}

// Mouse scroll callback
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (!g_camera) return; // Safety check
    
    // Only process scroll if mouse is captured
    if (mouseCaptured)
    {
        g_camera->ProcessMouseScroll(static_cast<float>(yoffset));
    }
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
        std::string title = "OpenGL Project - Phase 5 (HDR + Bloom) - FPS: " + std::to_string(frameCount);
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
    // Create ground texture once from file
    if (groundPlaneTexture == 0)
    {
        std::cout << "[Ground] Loading ground texture from file..." << std::endl;
        
        // Try to load ground texture from assets/textures/ground.*
        const char* groundPaths[] = {
            "assets/textures/ground.jpg",
            "assets/textures/ground.png",
            "assets/textures/ground.jpeg"
        };
        
        bool textureLoaded = false;
        
        for (const char* path : groundPaths)
        {
            std::filesystem::path absPath = std::filesystem::absolute(path);
            std::cout << "[Ground] Trying: " << path << std::endl;
            std::cout << "[Ground]   Absolute: " << absPath << std::endl;
            std::cout << "[Ground]   Exists: " << (std::filesystem::exists(absPath) ? "YES" : "NO") << std::endl;
            
            int width, height, nrChannels;
            stbi_set_flip_vertically_on_load(true); // Flip for correct orientation
            unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
            
            if (data)
            {
                GLenum format = GL_RGB;
                if (nrChannels == 1)
                    format = GL_RED;
                else if (nrChannels == 3)
                    format = GL_RGB;
                else if (nrChannels == 4)
                    format = GL_RGBA;
                
                glGenTextures(1, &groundPlaneTexture);
                glBindTexture(GL_TEXTURE_2D, groundPlaneTexture);
                glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
                glGenerateMipmap(GL_TEXTURE_2D);
                
                // CRITICAL: Proper texture parameters for ground
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                
                stbi_image_free(data);
                
                std::cout << "[Ground] ? SUCCESS! Ground texture loaded" << std::endl;
                std::cout << "[Ground]   Path: " << path << std::endl;
                std::cout << "[Ground]   Resolution: " << width << "x" << height << std::endl;
                std::cout << "[Ground]   Channels: " << nrChannels << " (format: ";
                if (format == GL_RED) std::cout << "RED";
                else if (format == GL_RGB) std::cout << "RGB";
                else if (format == GL_RGBA) std::cout << "RGBA";
                std::cout << ")" << std::endl;
                std::cout << "[Ground]   Texture ID: " << groundPlaneTexture << std::endl;
                std::cout << "[Ground]   Wrap: GL_REPEAT, Filter: MIPMAP_LINEAR" << std::endl;
                
                textureLoaded = true;
                break;
            }
            else
            {
                const char* reason = stbi_failure_reason();
                std::cout << "[Ground]   ? FAILED: " << (reason ? reason : "Unknown error") << std::endl;
            }
        }
        
        // Fallback to procedural texture if loading failed
        if (!textureLoaded)
        {
            std::cout << "[Ground] WARNING: Could not load ground texture from file!" << std::endl;
            std::cout << "[Ground] Creating procedural fallback texture..." << std::endl;
            
            const int texSize = 256;
            unsigned char* texData = new unsigned char[texSize * texSize * 3];
            
            for (int y = 0; y < texSize; ++y)
            {
                for (int x = 0; x < texSize; ++x)
                {
                    int idx = (y * texSize + x) * 3;
                    
                    // Create subtle checkerboard pattern
                    bool checker = ((x / 32) % 2) ^ ((y / 32) % 2);
                    unsigned char baseColor = checker ? 60 : 80;
                    
                    texData[idx + 0] = baseColor;     // R
                    texData[idx + 1] = baseColor;     // G
                    texData[idx + 2] = baseColor;     // B
                }
            }
            
            glGenTextures(1, &groundPlaneTexture);
            glBindTexture(GL_TEXTURE_2D, groundPlaneTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texSize, texSize, 0, GL_RGB, GL_UNSIGNED_BYTE, texData);
            glGenerateMipmap(GL_TEXTURE_2D);
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            delete[] texData;
            
            std::cout << "[Ground] Fallback texture created (ID: " << groundPlaneTexture << ")" << std::endl;
        }
    }
    
    if (groundPlaneVAO == 0)
    {
        // Generate buffers only once
        // UVs scaled for tiling (10x10 repeat to match ground scale)
        float planeVertices[] = {
            // Positions          // Normals           // TexCoords (scaled for tiling)
            -5.0f, 0.0f, -5.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
             5.0f, 0.0f, -5.0f,   0.0f, 1.0f, 0.0f,   10.0f, 0.0f,
             5.0f, 0.0f,  5.0f,   0.0f, 1.0f, 0.0f,   10.0f, 10.0f,
            -5.0f, 0.0f,  5.0f,   0.0f, 1.0f, 0.0f,   0.0f, 10.0f
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
        
        std::cout << "[Ground] Ground plane geometry initialized (10x10 UV tiling)" << std::endl;
    }

    // CRITICAL: Bind ground texture to correct unit BEFORE drawing
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, groundPlaneTexture);
    
    // Set model matrix and draw
    glBindVertexArray(groundPlaneVAO);
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
}

// Process debug keys (F1-F8, B, O, V, T, G, C, K, +/-, [/])
void processDebugKeys(GLFWwindow* window, SkyboxAtlas* skyboxAtlas)
{
    // F1: Toggle Shadows
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

    // F2: Toggle PCF
    if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS && !f2Pressed)
    {
        enablePCF = !enablePCF;
        std::cout << "PCF " << (enablePCF ? "ENABLED" : "DISABLED") << std::endl;
        f2Pressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_RELEASE)
    {
        f2Pressed = false;
    }

    // F3: Toggle Depth Map Debug
    if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS && !f3Pressed)
    {
        showDepthMap = !showDepthMap;
        std::cout << "Depth Map Debug " << (showDepthMap ? "ENABLED" : "DISABLED") << std::endl;
        f3Pressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_RELEASE)
    {
        f3Pressed = false;
    }

    // F4: Toggle Gamma Correction
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

    // O: Toggle Post-Processing
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS && !oPressed)
    {
        enablePostProcessing = !enablePostProcessing;
        std::cout << "Post-processing " << (enablePostProcessing ? "ENABLED" : "DISABLED") << std::endl;
        oPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_O) == GLFW_RELEASE)
    {
        oPressed = false;
    }

    // B: Toggle Bloom
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !bPressed)
    {
        enableBloom = !enableBloom;
        std::cout << "Bloom " << (enableBloom ? "ENABLED" : "DISABLED") << std::endl;
        bPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
    {
        bPressed = false;
    }

    // D: Cycle Debug Views
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS && !vPressed)  // Changed from D to V
    {
        debugViewMode = (debugViewMode + 1) % 4;
        const char* modes[] = {"Normal", "HDR Only", "Bright Pass", "Bloom Blur"};
        std::cout << "Debug View: " << modes[debugViewMode] << std::endl;
        vPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_V) == GLFW_RELEASE)
    {
        vPressed = false;
    }

    // +/=: Increase Exposure
    if ((glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS || 
         glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS) && !plusPressed)
    {
        exposure += EXPOSURE_STEP;
        std::cout << "Exposure: " << exposure << std::endl;
        plusPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_RELEASE && 
             glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_RELEASE)
    {
        plusPressed = false;
    }

    // -: Decrease Exposure
    if ((glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS || 
         glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS) && !minusPressed)
    {
        exposure -= EXPOSURE_STEP;
        if (exposure < 0.1f) exposure = 0.1f;
        std::cout << "Exposure: " << exposure << std::endl;
        minusPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_RELEASE && 
             glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_RELEASE)
    {
        minusPressed = false;
    }

    // [: Decrease Bloom Strength
    if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS && !leftBracketPressed)
    {
        bloomStrength -= BLOOM_STRENGTH_STEP;
        if (bloomStrength < 0.0f) bloomStrength = 0.0f;
        std::cout << "Bloom Strength: " << bloomStrength << std::endl;
        leftBracketPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_RELEASE)
    {
        leftBracketPressed = false;
    }

    // ]: Increase Bloom Strength
    if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS && !rightBracketPressed)
    {
        bloomStrength += BLOOM_STRENGTH_STEP;
        if (bloomStrength > 2.0f) bloomStrength = 2.0f;
        std::cout << "Bloom Strength: " << bloomStrength << std::endl;
        rightBracketPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_RELEASE)
    {
        rightBracketPressed = false;
    }

    // T: Decrease Threshold
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !tPressed)
    {
        bloomThreshold -= THRESHOLD_STEP;
        if (bloomThreshold < 0.1f) bloomThreshold = 0.1f;
        std::cout << "Bloom Threshold: " << bloomThreshold << std::endl;
        tPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE)
    {
        tPressed = false;
    }

    // G: Increase Threshold
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS && !gPressed)
    {
        bloomThreshold += THRESHOLD_STEP;
        if (bloomThreshold > 5.0f) bloomThreshold = 5.0f;
        std::cout << "Bloom Threshold: " << bloomThreshold << std::endl;
        gPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE)
    {
        gPressed = false;
    }

    // C: Toggle City
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS && !cPressed)
    {
        enableCity = !enableCity;
        std::cout << "City " << (enableCity ? "ENABLED" : "DISABLED") << std::endl;
        cPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE)
    {
        cPressed = false;
    }

    // K: Toggle Skybox Mode
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS && !kPressed)
    {
        // Only allow atlas mode if it's available
        if (!useSkyboxAtlas && skyboxAtlas->IsInitialized())
        {
            useSkyboxAtlas = true;
            std::cout << "Skybox Mode: ATLAS" << std::endl;
        }
        else if (useSkyboxAtlas)
        {
            useSkyboxAtlas = false;
            std::cout << "Skybox Mode: CUBEMAP" << std::endl;
        }
        else
        {
            std::cout << "Skybox Mode: CUBEMAP (Atlas not available)" << std::endl;
        }
        kPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_K) == GLFW_RELEASE)
    {
        kPressed = false;
    }

    // Legacy F5-F8 keys still work
    if (glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS && !f5Pressed)
    {
        enablePostProcessing = !enablePostProcessing;
        std::cout << "Post-processing " << (enablePostProcessing ? "ENABLED" : "DISABLED") << std::endl;
        f5Pressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_F5) == GLFW_RELEASE)
    {
        f5Pressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_F6) == GLFW_PRESS && !f6Pressed)
    {
        enableBloom = !enableBloom;
        std::cout << "Bloom " << (enableBloom ? "ENABLED" : "DISABLED") << std::endl;
        f6Pressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_F6) == GLFW_RELEASE)
    {
        f6Pressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_F7) == GLFW_PRESS && !f7Pressed)
    {
        exposure += EXPOSURE_STEP;
        std::cout << "Exposure: " << exposure << std::endl;
        f7Pressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_F7) == GLFW_RELEASE)
    {
        f7Pressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_F8) == GLFW_PRESS && !f8Pressed)
    {
        exposure -= EXPOSURE_STEP;
        if (exposure < 0.1f) exposure = 0.1f;
        std::cout << "Exposure: " << exposure << std::endl;
        f8Pressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_F8) == GLFW_RELEASE)
    {
        f8Pressed = false;
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
}

// Process light direction controls
void processLightControls(GLFWwindow* window)
{
    bool changed = false;
    
    // Arrow keys rotate light azimuth (only when not used for other controls)
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
        if (lightElevation > -5.0f) lightElevation = -5.0f;
        changed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        lightElevation -= 1.0f;
        if (lightElevation < -89.0f) lightElevation = -89.0f;
        changed = true;
    }
    
    // Wrap azimuth
    if (lightAzimuth >= 360.0f) lightAzimuth -= 360.0f;
    if (lightAzimuth < 0.0f) lightAzimuth += 360.0f;
    
    if (changed) {
        updateLightDirection();
    }
}
