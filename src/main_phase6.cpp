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

// Phase 8 includes
#include "EndlessSceneManager.h"
#include "EndlessCityManager.h"

// Phase 7 includes
#include "SkinnedCharacter.h"

// Phase 10 includes
#include "ParticleSystem.h"

// Phase 11 includes - Cascaded Shadow Maps (Hard Feature)
#include "CascadedShadowMap.h"

// Window dimensions
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

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
bool enablePostProcessing = true;
bool enableBloom = true;
float exposure = 1.0f;
const float EXPOSURE_STEP = 0.1f;
float bloomStrength = 0.4f;
const float BLOOM_STRENGTH_STEP = 0.05f;
float bloomThreshold = 1.0f;
const float THRESHOLD_STEP = 0.1f;
int debugViewMode = 0;

// Phase 9: Depth of Field (Hard Feature)
bool enableDoF = false;
float dofFocusDistance = 10.0f;
float dofFocusRange = 5.0f;
float dofMaxBlur = 1.0f;
float dofAperture = 1.0f;
const float DOF_FOCUS_STEP = 1.0f;
const float DOF_RANGE_STEP = 0.5f;
const float DOF_BLUR_STEP = 0.1f;
bool f9Pressed = false;
bool nPressed = false;  // Focus distance decrease
bool jPressed = false;  // Focus distance increase  
bool hPressed = false;  // Focus range adjust

// Phase 10: Particle System (Medium Feature)
bool enableParticles = true;
int currentParticleType = 0;  // 0=fire, 1=smoke, 2=spark, 3=magic
bool xPressed = false;   // Toggle particles
bool zPressed = false;   // Change particle type
bool f10Pressed = false; // Burst particles

// Phase 11: Cascaded Shadow Maps (Hard Feature)
bool enableCSM = false;  // Start disabled for testing
bool visualizeCSMCascades = false;
float csmSplitLambda = 0.5f;  // 0=linear, 1=logarithmic
bool f11Pressed = false;  // Toggle CSM
bool f12Pressed = false;  // Toggle cascade visualization

// Phase 6 additions
bool enableCity = true;
bool useSkyboxAtlas = false;
bool cPressed = false;
bool kPressed = false;

// Phase 8 additions
bool enableEndlessScene = false;  // DISABLED: Props hidden, only show 3 animated cubes
bool enableEndlessCity = true;    // Re-enabled
bool ePressed = false;

// Phase 7 additions
bool enableCharacter = true;
bool yPressed = false;
bool iPressed = false;
bool lPressed = false;
bool mPressed = false;  // Force emissive toggle

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
bool vPressed = false;
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
Camera* g_camera = nullptr;

// Shadow map pointer (will be initialized after OpenGL context)
ShadowMap* shadowMap = nullptr;

// Phase 8: Endless scene manager
EndlessSceneManager* endlessScene = nullptr;
EndlessCityManager* endlessCity = nullptr;  // NEW: Endless city manager

// Phase 7: Skinned character
SkinnedCharacter* character = nullptr;

// Phase 10: Particle system
ParticleSystem* particleSystem = nullptr;

// Phase 11: Cascaded Shadow Maps
CascadedShadowMap* csmShadowMap = nullptr;
unsigned int modelCSMShader = 0;  // CSM-enabled model shader
unsigned int buildingCSMShader = 0;  // CSM-enabled building shader

 // Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window, Camera& camera, float deltaTime);
void processDebugKeys(GLFWwindow* window, SkyboxAtlas* skyboxAtlas);
void toggleEndlessScene(GLFWwindow* window);
void toggleEndlessCity(GLFWwindow* window);  // NEW: Toggle endless city
void processLightControls(GLFWwindow* window);
void processCharacterControls(GLFWwindow* window);  // NEW: Character controls
void processDoFControls(GLFWwindow* window);  // NEW: DoF controls
void processParticleControls(GLFWwindow* window);  // NEW: Particle controls
void processCSMControls(GLFWwindow* window);  // NEW: CSM controls
std::string loadShaderFromFile(const char* filePath);
unsigned int compileShader(unsigned int type, const char* source);
unsigned int createShaderProgram(const char* vertexPath, const char* fragmentPath);
void updateFPS(GLFWwindow* window);
void renderQuad();
void renderGroundPlane(unsigned int shader, const glm::mat4& model);
void updateLightDirection();
unsigned int loadGroundTexture(const char* path);

// Ground plane VAO
unsigned int groundPlaneVAO = 0;
unsigned int groundPlaneVBO = 0;
unsigned int groundPlaneTexture = 0;

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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Project - Phase 6 (City + Skybox Atlas)", NULL, NULL);
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
    std::cout << "|  PHASE 6 - CITY GENERATION & SKYBOX ATLAS     |" << std::endl;
    std::cout << "|  (Procedural city with atlas skybox)          |" << std::endl;
    std::cout << "====================================================" << std::endl;

    std::cout << "\nOpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLFW Version: " << glfwGetVersionString() << std::endl;
    
    std::cout << "\n===================================" << std::endl;
    std::cout << "  CONTROLS" << std::endl;
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
    std::cout << "\n  PHASE 6 (CITY + SKYBOX ATLAS):" << std::endl;
    std::cout << "  C    - Toggle City ON/OFF" << std::endl;
    std::cout << "  K    - Toggle Skybox (Cubemap/Atlas)" << std::endl;
    std::cout << "\n  PHASE 8 (ENDLESS SCENE):" << std::endl;
    std::cout << "  E    - Toggle Endless Scene ON/OFF" << std::endl;
    std::cout << "  U    - Toggle Endless City ON/OFF" << std::endl;
    std::cout << "\n  PHASE 7 (CHARACTER ANIMATION):" << std::endl;
    std::cout << "  Y    - Toggle Character ON/OFF" << std::endl;
    std::cout << "  I    - Toggle Skeleton Debug" << std::endl;
    std::cout << "  M    - Toggle Emissive Debug (cyan)" << std::endl;
    std::cout << "\n  PHASE 9 (DEPTH OF FIELD - HARD):" << std::endl;
    std::cout << "  F9   - Toggle DoF ON/OFF" << std::endl;
    std::cout << "  N/J  - Focus Distance -/+" << std::endl;
    std::cout << "  H    - Cycle DoF debug views" << std::endl;
    std::cout << "\n  PHASE 10 (PARTICLE SYSTEM - MEDIUM):" << std::endl;
    std::cout << "  X    - Toggle Particles ON/OFF" << std::endl;
    std::cout << "  Z    - Cycle Particle Type (Fire/Smoke/Spark/Magic)" << std::endl;
    std::cout << "  F10  - Burst Particles" << std::endl;
    std::cout << "\n  PHASE 11 (CASCADED SHADOW MAPS - HARD):" << std::endl;
    std::cout << "  F11  - Toggle CSM ON/OFF" << std::endl;
    std::cout << "  F12  - Toggle Cascade Visualization" << std::endl;
    std::cout << "  </> - Adjust Split Lambda (linear/logarithmic)" << std::endl;
    std::cout << "\n  SHADOWS:" << std::endl;
    std::cout << "  F1 - Toggle shadows" << std::endl;
    std::cout << "  F2 - Toggle PCF (soft shadows)" << std::endl;
    std::cout << "  F3 - Toggle depth map debug" << std::endl;
    std::cout << "  Arrow Keys - Adjust light" << std::endl;
    std::cout << "===================================" << std::endl;

    std::cout << "Shadow Map Resolution: " << SHADOW_WIDTH << "x" << SHADOW_HEIGHT << std::endl;
    std::cout << "\n=== Phase 6: City Generation + Skybox Atlas ===\n" << std::endl;
    
    std::cout << "\n########################################" << std::endl;
    std::cout << "#  PHASE 6 - CITY + SKYBOX ATLAS      #" << std::endl;
    std::cout << "########################################" << std::endl;
    std::cout << "Features:" << std::endl;
    std::cout << "  - Procedural city generation" << std::endl;
    std::cout << "  - Skybox atlas rendering" << std::endl;
    std::cout << "  - Advanced post-processing" << std::endl;
    std::cout << "  - Shadow mapping with PCF" << std::endl;
    std::cout << "\nPress C to toggle city ON/OFF" << std::endl;
    std::cout << "Press K to toggle skybox mode" << std::endl;
    std::cout << "########################################\n" << std::endl;

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Initialize shadow map AFTER OpenGL context is ready
    shadowMap = new ShadowMap(SHADOW_WIDTH, SHADOW_HEIGHT);
    
    // Load ground texture
    std::cout << "Loading ground texture..." << std::endl;
    groundPlaneTexture = loadGroundTexture("assets/textures/ground.png");
    if (groundPlaneTexture == 0) {
        std::cout << "[WARN] Failed to load ground.png, trying ground.jpg..." << std::endl;
        groundPlaneTexture = loadGroundTexture("assets/textures/ground.jpg");
    }
    if (groundPlaneTexture == 0) {
        std::cout << "[WARN] No ground texture found, creating procedural checkerboard..." << std::endl;
        // Create a simple checkerboard texture
        unsigned char checkerboard[64][64][3];
        for (int y = 0; y < 64; y++) {
            for (int x = 0; x < 64; x++) {
                bool isWhite = ((x / 8) + (y / 8)) % 2 == 0;
                unsigned char color = isWhite ? 200 : 100;
                checkerboard[y][x][0] = color;
                checkerboard[y][x][1] = color;
                checkerboard[y][x][2] = color;
            }
        }
        glGenTextures(1, &groundPlaneTexture);
        glBindTexture(GL_TEXTURE_2D, groundPlaneTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 64, 64, 0, GL_RGB, GL_UNSIGNED_BYTE, checkerboard);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "[OK] Created procedural ground texture" << std::endl;
    } else {
        std::cout << "[OK] Ground texture loaded successfully" << std::endl;
    }

    // Build and compile shader programs
    std::cout << "Loading shaders..." << std::endl;
    
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
        useSkyboxAtlas = false;
    }
    
    std::cout << "[OK] Skybox atlas system initialized (Available: " << (atlasAvailable ? "YES" : "NO - using fallback") << ")\n" << std::endl;

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

    // Phase 8: Initialize Endless Scene
    std::cout << "Initializing Phase 8: Endless Scene..." << std::endl;
    endlessScene = new EndlessSceneManager();
    endlessScene->initialize(model, modelShader);
    std::cout << "[OK] Endless scene initialized\n" << std::endl;

    // Phase 8: Initialize Endless City
    std::cout << "Initializing Phase 8: Endless City..." << std::endl;
    endlessCity = new EndlessCityManager();
    endlessCity->initialize(buildingShader, modelShader);  // Use building shader for buildings, model shader for ground
    std::cout << "[OK] Endless city initialized\n" << std::endl;

    // Phase 7: Initialize Skinned Character
    std::cout << "\n=== Phase 7: GPU Skinning + Animation ===" << std::endl;
    character = new SkinnedCharacter();
    if (character->init("assets/models/bot/bot.gltf", "")) {
        // Position character at ground level (Y=0)
        // The mesh offset in the shader will handle grounding the mesh based on AABB
        character->setWorldTransform(glm::vec3(0.0f, -3.075f, 0.0f), 0.0f);
        character->setScale(2.0f);  // Double the default scale (autoScale handles sizing to ~2 units, so this makes it ~4 units)
        character->setVisible(true);
        character->setForceEmissive(true);  // Cyan debug color for visibility
        std::cout << "[OK] Character initialized successfully" << std::endl;
    } else {
        std::cerr << "[ERROR] Failed to initialize character, continuing without it" << std::endl;
        delete character;
        character = nullptr;
    }
    std::cout << "===================================\n" << std::endl;

    // Phase 10: Initialize Particle System
    std::cout << "\n=== Phase 10: GPU Particle System (Medium) ===" << std::endl;
    particleSystem = new ParticleSystem(5000);
    if (particleSystem->Initialize()) {
        // Configure fire emitter
        EmitterConfig fireConfig;
        fireConfig.position = glm::vec3(5.0f, 0.5f, 0.0f);  // Position near right cube
        fireConfig.direction = glm::vec3(0.0f, 1.0f, 0.0f);
        fireConfig.spread = 0.3f;
        fireConfig.minSpeed = 1.0f;
        fireConfig.maxSpeed = 3.0f;
        fireConfig.minLife = 0.5f;
        fireConfig.maxLife = 2.0f;
        fireConfig.minSize = 0.2f;
        fireConfig.maxSize = 0.6f;
        fireConfig.emitRate = 100.0f;
        fireConfig.startColor = glm::vec4(1.0f, 0.8f, 0.3f, 1.0f);
        fireConfig.endColor = glm::vec4(1.0f, 0.2f, 0.0f, 0.0f);
        fireConfig.gravity = glm::vec3(0.0f, 0.5f, 0.0f);  // Fire rises
        fireConfig.type = ParticleType::FIRE;
        
        particleSystem->SetEmitterConfig(fireConfig);
        particleSystem->SetEmitting(true);
        std::cout << "[OK] Particle system initialized with fire effect" << std::endl;
    } else {
        std::cerr << "[ERROR] Failed to initialize particle system" << std::endl;
        delete particleSystem;
        particleSystem = nullptr;
    }
    std::cout << "==========================================\n" << std::endl;

    // Phase 11: Initialize Cascaded Shadow Maps
    std::cout << "\n=== Phase 11: Cascaded Shadow Maps (Hard) ===" << std::endl;
    csmShadowMap = new CascadedShadowMap(2048);
    if (csmShadowMap->Initialize()) {
        csmShadowMap->SetSplitLambda(csmSplitLambda);
        std::cout << "[OK] Cascaded Shadow Maps initialized" << std::endl;
        
        // Load CSM-enabled model shader (with dedicated CSM vertex shader)
        modelCSMShader = createShaderProgram("shaders/model_csm.vert", "shaders/model_csm.frag");
        if (modelCSMShader != 0) {
            std::cout << "[OK] CSM model shader loaded" << std::endl;
        } else {
            std::cerr << "[WARN] Failed to load CSM model shader, falling back to standard shadows" << std::endl;
            enableCSM = false;
        }
        
        // Load CSM-enabled building shader
        buildingCSMShader = createShaderProgram("shaders/building_csm.vert", "shaders/building_csm.frag");
        if (buildingCSMShader != 0) {
            std::cout << "[OK] CSM building shader loaded" << std::endl;
        } else {
            std::cerr << "[WARN] Failed to load CSM building shader" << std::endl;
        }
    } else {
        std::cerr << "[ERROR] Failed to initialize CSM" << std::endl;
        delete csmShadowMap;
        csmShadowMap = nullptr;
        enableCSM = false;
    }
    std::cout << "=============================================\n" << std::endl;

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
    Camera camera(glm::vec3(0.0f, 5.0f, 15.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
    g_camera = &camera;
    
    // Phase 4: Enable mouse capture by default
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    // Light setup
    glm::vec3 dirLightColor(1.0f, 1.0f, 0.95f);
    glm::vec3 pointLightPos(2.0f, 3.0f, 2.0f);
    glm::vec3 pointLightColor(1.0f, 0.9f, 0.7f);

    std::cout << "=== Rendering Started ===" << std::endl;
    std::cout << "Scene configured with:" << std::endl;
    std::cout << "  * Directional light with shadows" << std::endl;
    std::cout << "  * Point light (no shadows)" << std::endl;
    std::cout << "  * Ground plane for shadow testing" << std::endl;
    std::cout << "  * Multiple cubes casting shadows" << std::endl;
    std::cout << "  * Procedurally generated city" << std::endl;
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

        // Phase 8: Update endless scene
        if (endlessScene && enableEndlessScene && !animationPaused) {
            endlessScene->update(deltaTime, camera.Position);
        }

        // Phase 8: Update endless city
        if (endlessCity && enableEndlessCity && !animationPaused) {
            endlessCity->update(deltaTime, camera.Position);
        }

        // Phase 7: Update character
        if (character && enableCharacter && !animationPaused) {
            character->update(deltaTime);
        }

        // Phase 10: Update particle system
        if (particleSystem && enableParticles && !animationPaused) {
            particleSystem->Update(deltaTime);
        }

        // Input
        processInput(window, camera, deltaTime);
        processDebugKeys(window, skyboxAtlas);
        toggleEndlessScene(window);
        toggleEndlessCity(window);  // NEW: Toggle endless city
        processLightControls(window);
        processCharacterControls(window);  // NEW: Character controls
        processDoFControls(window);  // NEW: DoF controls
        processParticleControls(window);  // NEW: Particle controls
        processCSMControls(window);  // NEW: CSM controls

        // Update FPS
        updateFPS(window);

        // Phase 5: Begin post-processing render (if enabled AND not in debug mode)
        bool usePostProcessing = enablePostProcessing && postProcessor.IsInitialized() && !showDepthMap;
        if (usePostProcessing) {
            postProcessor.BeginRender();
        }

        // === PASS 1: SHADOW MAP (DEPTH PASS) ===
        float near_plane = 0.1f, far_plane = 100.0f;
        
        // Define model matrices (shared between shadow and main passes)
        glm::mat4 groundModel = glm::mat4(1.0f);
        groundModel = glm::scale(groundModel, glm::vec3(10.0f, 1.0f, 10.0f));
        
        glm::mat4 cubeModel = glm::mat4(1.0f);
        cubeModel = glm::translate(cubeModel, glm::vec3(0.0f, 1.5f, 0.0f));
        cubeModel = glm::rotate(cubeModel, cubeRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        
        glm::mat4 cube2Model = glm::mat4(1.0f);
        cube2Model = glm::translate(cube2Model, glm::vec3(-3.0f, 1.0f, -2.0f));
        cube2Model = glm::rotate(cube2Model, cubeRotationAngle * 0.5f, glm::vec3(1.0f, 0.5f, 0.0f));
        cube2Model = glm::scale(cube2Model, glm::vec3(0.8f));
        
        glm::mat4 cube3Model = glm::mat4(1.0f);
        cube3Model = glm::translate(cube3Model, glm::vec3(3.0f, 0.8f, 1.0f));
        cube3Model = glm::rotate(cube3Model, cubeRotationAngle * -0.7f, glm::vec3(0.0f, 1.0f, 1.0f));
        cube3Model = glm::scale(cube3Model, glm::vec3(0.6f));
        
        // Update CSM cascades if enabled
        if (csmShadowMap && enableCSM) {
            glm::mat4 projection = camera.GetProjectionMatrix((float)SCR_WIDTH / (float)SCR_HEIGHT);
            glm::mat4 viewMat = camera.GetViewMatrix();
            csmShadowMap->UpdateCascades(viewMat, projection, lightDirection, near_plane, far_plane);
            
            // CRITICAL: Enable depth test and set proper state for shadow rendering
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glDepthMask(GL_TRUE);
            
            // Render to each cascade
            glCullFace(GL_FRONT);
            glUseProgram(shadowShader);
            
            for (int i = 0; i < NUM_CASCADES; i++) {
                csmShadowMap->BindForWriting(i);
                
                // Ensure depth is cleared and writable
                glClear(GL_DEPTH_BUFFER_BIT);
                
                glm::mat4 cascadeLightSpaceMatrix = csmShadowMap->GetCascade(i).lightSpaceMatrix;
                glUniformMatrix4fv(glGetUniformLocation(shadowShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(cascadeLightSpaceMatrix));
                
                // Render ground plane
                renderGroundPlane(shadowShader, groundModel);
                
                // Render main animated cube (center)
                glUniformMatrix4fv(glGetUniformLocation(shadowShader, "model"), 1, GL_FALSE, glm::value_ptr(cubeModel));
                model->Draw(shadowShader);
                
                // Cube 2 (left)
                glUniformMatrix4fv(glGetUniformLocation(shadowShader, "model"), 1, GL_FALSE, glm::value_ptr(cube2Model));
                model->Draw(shadowShader);
                
                // Cube 3 (right)
                glUniformMatrix4fv(glGetUniformLocation(shadowShader, "model"), 1, GL_FALSE, glm::value_ptr(cube3Model));
                model->Draw(shadowShader);
                
                // Phase 6: Render city buildings in shadow pass
                if (enableCity) {
                    city.RenderShadow(cascadeLightSpaceMatrix, shadowShader);
                }
                
                // Phase 7: Render character in shadow pass
                if (character && enableCharacter) {
                    character->renderDepth(cascadeLightSpaceMatrix);
                }
            }
            csmShadowMap->Unbind();
            glCullFace(GL_BACK);
        } else {
            // Legacy single shadow map
            glm::mat4 lightProjection = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, near_plane, far_plane);
            glm::mat4 lightView = glm::lookAt(
                -lightDirection * 25.0f,
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f)
            );
            glm::mat4 lightSpaceMatrix = lightProjection * lightView;

            // Render scene to shadow map
            shadowMap->BindForWriting();
            glCullFace(GL_FRONT);
            glUseProgram(shadowShader);
            glUniformMatrix4fv(glGetUniformLocation(shadowShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

            // Render ground plane
            renderGroundPlane(shadowShader, groundModel);

            // Render main animated cube (center)
            glUniformMatrix4fv(glGetUniformLocation(shadowShader, "model"), 1, GL_FALSE, glm::value_ptr(cubeModel));
            model->Draw(shadowShader);

            // Cube 2 (left)
            glUniformMatrix4fv(glGetUniformLocation(shadowShader, "model"), 1, GL_FALSE, glm::value_ptr(cube2Model));
            model->Draw(shadowShader);

            // Cube 3 (right)
            glUniformMatrix4fv(glGetUniformLocation(shadowShader, "model"), 1, GL_FALSE, glm::value_ptr(cube3Model));
            model->Draw(shadowShader);

            // Phase 6: Render city buildings in shadow pass
            if (enableCity) {
                city.RenderShadow(lightSpaceMatrix, shadowShader);
            }

            // Phase 7: Render character in shadow pass
            if (character && enableCharacter) {
                character->renderDepth(lightSpaceMatrix);
            }

            shadowMap->Unbind();
            glCullFace(GL_BACK);
        }
        
        // CRITICAL FIX: Always restore framebuffer and viewport AFTER shadow pass
        // This ensures consistent state whether CSM is ON or OFF
        if (usePostProcessing) {
            postProcessor.BeginRender();  // Re-bind the HDR framebuffer
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        
        // Always reset viewport and clear after shadow pass
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        
        // Get light space matrix for legacy compatibility
        glm::mat4 lightSpaceMatrix;
        if (csmShadowMap && enableCSM) {
            lightSpaceMatrix = csmShadowMap->GetCascade(0).lightSpaceMatrix;
        } else {
            glm::mat4 lightProjection = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, near_plane, far_plane);
            glm::mat4 lightView = glm::lookAt(-lightDirection * 25.0f, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            lightSpaceMatrix = lightProjection * lightView;
        }

        // === PASS 2: MAIN RENDER OR DEBUG VIEW ===
        // Note: Framebuffer and viewport already set above after shadow pass

        if (showDepthMap)
        {
            // Debug: Show depth map
            glDisable(GL_DEPTH_TEST);
            glUseProgram(debugDepthShader);
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, shadowMap->GetDepthTexture());
            glUniform1i(glGetUniformLocation(debugDepthShader, "shadowMap"), 0);
            
            renderQuad();
            
            glEnable(GL_DEPTH_TEST);
        }
        else
        {
            // Normal rendering
            glm::mat4 projection = camera.GetProjectionMatrix((float)SCR_WIDTH / (float)SCR_HEIGHT);
            glm::mat4 view = camera.GetViewMatrix();

            // Render skybox first (choose mode)
            if (useSkyboxAtlas && skyboxAtlas->IsInitialized())
            {
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
                glDepthFunc(GL_LEQUAL);
                glUseProgram(skyboxShader);
                glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
                glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "view"), 1, GL_FALSE, glm::value_ptr(skyboxView));
                glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
                skybox->Draw(skyboxShader);
                glDepthFunc(GL_LESS);
            }

            // Render scene with lighting and shadows
            unsigned int activeModelShader = (enableCSM && modelCSMShader != 0) ? modelCSMShader : modelShader;
            glUseProgram(activeModelShader);

            glUniformMatrix4fv(glGetUniformLocation(activeModelShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniformMatrix4fv(glGetUniformLocation(activeModelShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(activeModelShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

            glUniform3fv(glGetUniformLocation(activeModelShader, "dirLightDir"), 1, glm::value_ptr(lightDirection));
            glUniform3fv(glGetUniformLocation(activeModelShader, "dirLightColor"), 1, glm::value_ptr(dirLightColor));
            
            glUniform3fv(glGetUniformLocation(activeModelShader, "pointLightPos"), 1, glm::value_ptr(pointLightPos));
            glUniform3fv(glGetUniformLocation(activeModelShader, "pointLightColor"), 1, glm::value_ptr(pointLightColor));
            glUniform1f(glGetUniformLocation(activeModelShader, "pointLightConstant"), 1.0f);
            glUniform1f(glGetUniformLocation(activeModelShader, "pointLightLinear"), 0.09f);
            glUniform1f(glGetUniformLocation(activeModelShader, "pointLightQuadratic"), 0.032f);

            glUniform3fv(glGetUniformLocation(activeModelShader, "viewPos"), 1, glm::value_ptr(camera.Position));

            // CSM uniforms
            if (enableCSM && csmShadowMap && modelCSMShader != 0) {
                glUniform1i(glGetUniformLocation(activeModelShader, "enableCSM"), GL_TRUE);
                glUniform1i(glGetUniformLocation(activeModelShader, "visualizeCascades"), visualizeCSMCascades ? GL_TRUE : GL_FALSE);
                glUniform1i(glGetUniformLocation(activeModelShader, "numCascades"), NUM_CASCADES);
                
                // IMPORTANT: Pass view matrix for cascade selection
                glUniformMatrix4fv(glGetUniformLocation(activeModelShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
                
                // Upload light space matrices
                glm::mat4 lightMatrices[NUM_CASCADES];
                csmShadowMap->GetLightSpaceMatrices(lightMatrices);
                for (int i = 0; i < NUM_CASCADES; i++) {
                    std::string uniformName = "lightSpaceMatrices[" + std::to_string(i) + "]";
                    glUniformMatrix4fv(glGetUniformLocation(activeModelShader, uniformName.c_str()), 1, GL_FALSE, glm::value_ptr(lightMatrices[i]));
                }
                
                // Upload cascade splits
                float splits[NUM_CASCADES];
                csmShadowMap->GetCascadeSplits(splits);
                for (int i = 0; i < NUM_CASCADES; i++) {
                    std::string uniformName = "cascadeSplits[" + std::to_string(i) + "]";
                    glUniform1f(glGetUniformLocation(activeModelShader, uniformName.c_str()), splits[i]);
                }
                
                // Bind CSM shadow map array to texture unit 2
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D_ARRAY, csmShadowMap->GetDepthTextureArray());
                glUniform1i(glGetUniformLocation(activeModelShader, "csmShadowMap"), 2);
            } else {
                glUniform1i(glGetUniformLocation(activeModelShader, "enableCSM"), GL_FALSE);
                glUniform1i(glGetUniformLocation(activeModelShader, "numCascades"), 0);
            }

            // Bind legacy shadow map to texture unit 1
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, shadowMap->GetDepthTexture());
            glUniform1i(glGetUniformLocation(activeModelShader, "shadowMap"), 1);

            glUniform1i(glGetUniformLocation(activeModelShader, "enableShadows"), enableShadows);
            glUniform1i(glGetUniformLocation(activeModelShader, "uUsePCF"), enablePCF);
            glUniform1i(glGetUniformLocation(activeModelShader, "enableGammaCorrection"), enableGammaCorrection);
            glUniform1f(glGetUniformLocation(activeModelShader, "material.shininess"), 32.0f);
            glUniform1f(glGetUniformLocation(activeModelShader, "bloomThreshold"), bloomThreshold);

            // Reset to texture unit 0 for diffuse textures
            glActiveTexture(GL_TEXTURE0);

            // Render ground plane
            renderGroundPlane(activeModelShader, groundModel);

            // Render main animated cube (center)
            glUniformMatrix4fv(glGetUniformLocation(activeModelShader, "model"), 1, GL_FALSE, glm::value_ptr(cubeModel));
            model->Draw(activeModelShader);

            // Render extra cubes
            glUniformMatrix4fv(glGetUniformLocation(activeModelShader, "model"), 1, GL_FALSE, glm::value_ptr(cube2Model));
            model->Draw(activeModelShader);
            
            glUniformMatrix4fv(glGetUniformLocation(activeModelShader, "model"), 1, GL_FALSE, glm::value_ptr(cube3Model));
            model->Draw(activeModelShader);
            
            // Phase 6: Render city
            if (enableCity)
            {
                // Choose shader based on CSM state
                unsigned int activeBuildingShader = (enableCSM && buildingCSMShader != 0) ? buildingCSMShader : buildingShader;
                glUseProgram(activeBuildingShader);
                
                glUniformMatrix4fv(glGetUniformLocation(activeBuildingShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
                glUniformMatrix4fv(glGetUniformLocation(activeBuildingShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
                glUniformMatrix4fv(glGetUniformLocation(activeBuildingShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
                
                glUniform3fv(glGetUniformLocation(activeBuildingShader, "dirLightDir"), 1, glm::value_ptr(lightDirection));
                glUniform3fv(glGetUniformLocation(activeBuildingShader, "dirLightColor"), 1, glm::value_ptr(dirLightColor));
                glUniform3fv(glGetUniformLocation(activeBuildingShader, "pointLightPos"), 1, glm::value_ptr(pointLightPos));
                glUniform3fv(glGetUniformLocation(activeBuildingShader, "pointLightColor"), 1, glm::value_ptr(pointLightColor));
                glUniform1f(glGetUniformLocation(activeBuildingShader, "pointLightConstant"), 1.0f);
                glUniform1f(glGetUniformLocation(activeBuildingShader, "pointLightLinear"), 0.09f);
                glUniform1f(glGetUniformLocation(activeBuildingShader, "pointLightQuadratic"), 0.032f);
                
                glUniform3fv(glGetUniformLocation(activeBuildingShader, "viewPos"), 1, glm::value_ptr(camera.Position));
                
                // CSM uniforms for building shader
                if (enableCSM && csmShadowMap && buildingCSMShader != 0) {
                    glUniform1i(glGetUniformLocation(activeBuildingShader, "enableCSM"), GL_TRUE);
                    glUniform1i(glGetUniformLocation(activeBuildingShader, "visualizeCascades"), visualizeCSMCascades ? GL_TRUE : GL_FALSE);
                    glUniform1i(glGetUniformLocation(activeBuildingShader, "numCascades"), NUM_CASCADES);
                    
                    // Upload light space matrices
                    glm::mat4 lightMatrices[NUM_CASCADES];
                    csmShadowMap->GetLightSpaceMatrices(lightMatrices);
                    for (int i = 0; i < NUM_CASCADES; i++) {
                        std::string uniformName = "lightSpaceMatrices[" + std::to_string(i) + "]";
                        glUniformMatrix4fv(glGetUniformLocation(activeBuildingShader, uniformName.c_str()), 1, GL_FALSE, glm::value_ptr(lightMatrices[i]));
                    }
                    
                    // Upload cascade splits
                    float splits[NUM_CASCADES];
                    csmShadowMap->GetCascadeSplits(splits);
                    for (int i = 0; i < NUM_CASCADES; i++) {
                        std::string uniformName = "cascadeSplits[" + std::to_string(i) + "]";
                        glUniform1f(glGetUniformLocation(activeBuildingShader, uniformName.c_str()), splits[i]);
                    }
                    
                    // Bind CSM shadow map array to texture unit 2
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D_ARRAY, csmShadowMap->GetDepthTextureArray());
                    glUniform1i(glGetUniformLocation(activeBuildingShader, "csmShadowMap"), 2);
                } else {
                    glUniform1i(glGetUniformLocation(activeBuildingShader, "enableCSM"), GL_FALSE);
                    glUniform1i(glGetUniformLocation(activeBuildingShader, "numCascades"), 0);
                }
                
                // Bind legacy shadow map
                shadowMap->BindForReading(GL_TEXTURE1);
                glUniform1i(glGetUniformLocation(activeBuildingShader, "shadowMap"), 1);
                
                glUniform1i(glGetUniformLocation(activeBuildingShader, "enableShadows"), enableShadows);
                glUniform1i(glGetUniformLocation(activeBuildingShader, "uUsePCF"), enablePCF);
                glUniform1f(glGetUniformLocation(activeBuildingShader, "bloomThreshold"), bloomThreshold);
                
                // Pass the active shader to City::Render so it doesn't override
                city.Render(view, projection, lightSpaceMatrix, camera.Position, activeBuildingShader);
                
                // CRITICAL: Reset OpenGL state after city rendering
                glUseProgram(0);
                glBindVertexArray(0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, 0);
            }

            // Phase 8: Render endless scene
            if (endlessScene && enableEndlessScene)
            {
                glm::mat4 viewProj = projection * view;
                endlessScene->render(viewProj);
                
                // Also render ground for endless scene
                endlessScene->renderGround(view, projection);
            }

            // Phase 8: Render endless city
            if (endlessCity && enableEndlessCity)
            {
                endlessCity->render(view, projection, lightSpaceMatrix, camera.Position);
            }

            // Phase 7: Render character
            if (character && enableCharacter)
            {
                DirectionalLight dirLight;
                dirLight.direction = lightDirection;
                dirLight.color = dirLightColor;
                
                PointLight ptLight;
                ptLight.position = pointLightPos;
                ptLight.color = pointLightColor;
                ptLight.constant = 1.0f;
                ptLight.linear = 0.09f;
                ptLight.quadratic = 0.032f;
                
                ShadowData shadowData;
                shadowData.lightSpaceMatrix = lightSpaceMatrix;
                shadowData.shadowMapTexture = shadowMap->GetDepthTexture();
                
                RenderOptions renderOpts;
                renderOpts.enableShadows = enableShadows;
                renderOpts.enablePCF = enablePCF;
                renderOpts.bloomThreshold = bloomThreshold;
                
                character->render(view, projection, dirLight, ptLight, shadowData, renderOpts);
            }

            // Phase 10: Render particle system (after opaque objects, before post-processing)
            if (particleSystem && enableParticles)
            {
                particleSystem->Render(view, projection, camera.Position);
            }
        }

        // Phase 5: End post-processing render and apply effects
        if (usePostProcessing) {
            postProcessor.EndRender();
            
            // Create DoF settings
            DoFSettings dofSettings;
            dofSettings.enabled = enableDoF;
            dofSettings.focusDistance = dofFocusDistance;
            dofSettings.focusRange = dofFocusRange;
            dofSettings.maxBlur = dofMaxBlur;
            dofSettings.aperture = dofAperture;
            dofSettings.nearPlane = 0.1f;
            dofSettings.farPlane = 100.0f;
            
            postProcessor.Render(exposure, enableBloom, enableGammaCorrection, bloomStrength, debugViewMode, dofSettings);
        }

        // Phase 4: Render HUD overlay
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        float hudY = 580.0f;
        float hudScale = 1.3f;
        glm::vec3 hudColor(0.0f, 1.0f, 0.0f);

        hud.RenderText("FPS: " + std::to_string(currentFPS), 10.0f, hudY, hudScale, hudColor);
        hudY -= 18.0f;
        
        hud.RenderText("Post: " + std::string(enablePostProcessing ? "ON" : "OFF") + " (O)", 10.0f, hudY, hudScale, hudColor);
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
        
        const char* debugModes[] = {"Normal", "HDR Only", "Bright", "Bloom", "DoF Blur", "Depth", "CoC"};
        hud.RenderText(std::string("View: ") + debugModes[debugViewMode] + " (V/H)", 10.0f, hudY, hudScale, hudColor);
        hudY -= 18.0f;
        
        // Phase 6 graphics settings
        hud.RenderText("Shadows: " + std::string(enableShadows ? "ON" : "OFF") + " (F1)", 10.0f, hudY, hudScale, hudColor);
        hudY -= 18.0f;
        
        hud.RenderText("PCF: " + std::string(enablePCF ? "ON" : "OFF") + " (F2)", 10.0f, hudY, hudScale, hudColor);
        hudY -= 18.0f;
        
        hud.RenderText("Gamma: " + std::string(enableGammaCorrection ? "ON" : "OFF") + " (F4)", 10.0f, hudY, hudScale, hudColor);
        hudY -= 18.0f;
        
        hud.RenderText("Skybox: " + std::string(useSkyboxAtlas ? "ATLAS" : "CUBEMAP") + " (K)", 10.0f, hudY, hudScale, hudColor);
        hudY -= 18.0f;
        
        hud.RenderText("City: " + std::string(enableCity ? "ON" : "OFF") + " (C)", 10.0f, hudY, hudScale, hudColor);
        hudY -= 18.0f;
        
        hud.RenderText("Endless: " + std::string(enableEndlessScene ? "ON" : "OFF") + " (E)", 10.0f, hudY, hudScale, hudColor);
        hudY -= 18.0f;
        
        hud.RenderText("City: " + std::string(enableEndlessCity ? "ON" : "OFF") + " (U)", 10.0f, hudY, hudScale, hudColor);
        hudY -= 18.0f;
        
        // Phase 9: DoF info
        hud.RenderText("DoF: " + std::string(enableDoF ? "ON" : "OFF") + " (F9)", 10.0f, hudY, hudScale, hudColor);
        hudY -= 18.0f;
        
        if (enableDoF) {
            char dofBuf[64];
            snprintf(dofBuf, sizeof(dofBuf), "Focus: %.1f Range: %.1f (N/J)", dofFocusDistance, dofFocusRange);
            hud.RenderText(dofBuf, 10.0f, hudY, hudScale * 0.9f, hudColor);
            hudY -= 18.0f;
        }
        
        // Phase 10: Particle system info
        const char* particleTypes[] = {"Fire", "Smoke", "Spark", "Magic"};
        char particleBuf[64];
        snprintf(particleBuf, sizeof(particleBuf), "Particles: %s %s (X)", 
                 enableParticles ? "ON" : "OFF", particleTypes[currentParticleType]);
        hud.RenderText(particleBuf, 10.0f, hudY, hudScale, hudColor);
        hudY -= 18.0f;
        
        if (particleSystem && enableParticles) {
            snprintf(particleBuf, sizeof(particleBuf), "Active: %d/%d (Z=type, F10=burst)", 
                     particleSystem->GetActiveCount(), particleSystem->GetMaxParticles());
            hud.RenderText(particleBuf, 10.0f, hudY, hudScale * 0.9f, hudColor);
            hudY -= 18.0f;
        }
        
        // Phase 11: CSM info
        hud.RenderText("CSM: " + std::string(enableCSM ? "ON" : "OFF") + " (F11)", 10.0f, hudY, hudScale, hudColor);
        hudY -= 18.0f;
        
        if (enableCSM && csmShadowMap) {
            char csmBuf[64];
            snprintf(csmBuf, sizeof(csmBuf), "Cascades: %d Lambda: %.2f (</>) Vis: %s", 
                     NUM_CASCADES, csmSplitLambda, visualizeCSMCascades ? "ON" : "OFF");
            hud.RenderText(csmBuf, 10.0f, hudY, hudScale * 0.9f, hudColor);
            hudY -= 18.0f;
        }
        
        // Camera position display
        char camBuf[64];
        snprintf(camBuf, sizeof(camBuf), "Cam: (%.1f, %.1f, %.1f)", camera.Position.x, camera.Position.y, camera.Position.z);
        hud.RenderText(camBuf, 10.0f, hudY, hudScale * 0.9f, hudColor);
        hudY -= 18.0f;
        
        hud.RenderText("Mouse: " + std::string(mouseCaptured ? "CAPTURED" : "FREE") + " (SPC)", 10.0f, hudY, hudScale * 0.9f, hudColor);
        hudY -= 18.0f;
        
        if (endlessScene && enableEndlessScene) {
            char segmentBuf[64];
            snprintf(segmentBuf, sizeof(segmentBuf), "Segment: %d (Gen: %d)", 
                     endlessScene->getCurrentSegmentIndex(),
                     endlessScene->getTotalSegmentsGenerated());
            hud.RenderText(segmentBuf, 10.0f, hudY, hudScale * 0.9f, hudColor);
            hudY -= 18.0f;
        }
        
        if (endlessCity && enableEndlessCity) {
            auto chunk = endlessCity->getCurrentChunk();
            char cityBuf[64];
            snprintf(cityBuf, sizeof(cityBuf), "CityChunk: (%d,%d) Buildings: %d",
                     chunk.first,
                     chunk.second,
                     endlessCity->getTotalBuildingsGenerated());
            hud.RenderText(cityBuf, 10.0f, hudY, hudScale * 0.9f, hudColor);
            hudY -= 18.0f;
        }

        // Phase 7: Character info
        if (character) {
            hud.RenderText("Character: " + std::string(enableCharacter ? "ON" : "OFF") + " (Y)", 10.0f, hudY, hudScale, hudColor);
            hudY -= 18.0f;
            
            if (enableCharacter) {
                char charBuf[64];
                snprintf(charBuf, sizeof(charBuf), "Anim: %s (%.1fs)", 
                         character->getAnimationName().c_str(),
                         character->getAnimationTime());
                hud.RenderText(charBuf, 10.0f, hudY, hudScale * 0.9f, hudColor);
                hudY -= 18.0f;
                
                snprintf(charBuf, sizeof(charBuf), "Joints: %d", character->getJointCount());
                hud.RenderText(charBuf, 10.0f, hudY, hudScale * 0.9f, hudColor);
                hudY -= 18.0f;
            }
        }

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    delete model;
    if (skybox) delete skybox;
    if (skyboxAtlas) delete skyboxAtlas;
    if (shadowMap) delete shadowMap;
    
    // Phase 8: Cleanup endless scene
    if (endlessScene) {
        endlessScene->cleanup();
        delete endlessScene;
    }
    
    // Phase 8: Cleanup endless city
    if (endlessCity) {
        endlessCity->cleanup();
        delete endlessCity;
    }

    // Phase 7: Cleanup character
    if (character) {
        delete character;
    }

    // Phase 10: Cleanup particle system
    if (particleSystem) {
        particleSystem->Cleanup();
        delete particleSystem;
    }

    // Phase 11: Cleanup CSM
    if (csmShadowMap) {
        csmShadowMap->Cleanup();
        delete csmShadowMap;
    }

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
    if (modelCSMShader) glDeleteProgram(modelCSMShader);
    if (buildingCSMShader) glDeleteProgram(buildingCSMShader);

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

// Process keyboard input - Phase 6 Debug
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

    // V: Cycle Debug Views
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS && !vPressed)
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
}

// E: Toggle Endless Scene
void toggleEndlessScene(GLFWwindow* window)
{
    static bool ePressed = false;
    
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS && !ePressed)
    {
        enableEndlessScene = !enableEndlessScene;
        std::cout << "Endless Scene " << (enableEndlessScene ? "ENABLED" : "DISABLED") << std::endl;
        ePressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_RELEASE)
    {
        ePressed = false;
    }
}

// U: Toggle Endless City
void toggleEndlessCity(GLFWwindow* window)
{
    static bool uPressed = false;
    
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS && !uPressed)
    {
        enableEndlessCity = !enableEndlessCity;
        std::cout << "Endless City " << (enableEndlessCity ? "ENABLED" : "DISABLED") << std::endl;
        uPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_U) == GLFW_RELEASE)
    {
        uPressed = false;
    }
}

// Character controls
void processCharacterControls(GLFWwindow* window)
{
    if (!character) return;
    
    // Y: Toggle character visibility
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS && !yPressed)
    {
        enableCharacter = !enableCharacter;
        std::cout << "Character " << (enableCharacter ? "ENABLED" : "DISABLED") << std::endl;
        yPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_RELEASE)
    {
        yPressed = false;
    }
    
    // I: Toggle skeleton debug
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS && !iPressed)
    {
        bool current = character->isVisible();  // Get current skeleton state indirectly
        character->setDrawSkeleton(!current);
        std::cout << "Character skeleton " << (!current ? "ENABLED" : "DISABLED") << std::endl;
        iPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_I) == GLFW_RELEASE)
    {
        iPressed = false;
    }
    
    // L: Toggle character animation pause (separate from cube animation)
    // (Already handled by main P key, keeping this for completeness)
    
    // M: Toggle force emissive (visibility debug)
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS && !mPressed)
    {
        static bool emissive = false;
        emissive = !emissive;
        character->setForceEmissive(emissive);
        std::cout << "Character force emissive " << (emissive ? "ENABLED" : "DISABLED") << std::endl;
        mPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE)
    {
        mPressed = false;
    }
}

// DoF controls (Phase 9 - Hard Feature)
void processDoFControls(GLFWwindow* window)
{
    // F9: Toggle DoF
    if (glfwGetKey(window, GLFW_KEY_F9) == GLFW_PRESS && !f9Pressed)
    {
        enableDoF = !enableDoF;
        std::cout << "Depth of Field " << (enableDoF ? "ENABLED" : "DISABLED") << std::endl;
        f9Pressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_F9) == GLFW_RELEASE)
    {
        f9Pressed = false;
    }
    
    // N: Decrease focus distance
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS && !nPressed)
    {
        dofFocusDistance -= DOF_FOCUS_STEP;
        if (dofFocusDistance < 1.0f) dofFocusDistance = 1.0f;
        std::cout << "DoF Focus Distance: " << dofFocusDistance << std::endl;
        nPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_N) == GLFW_RELEASE)
    {
        nPressed = false;
    }
    
    // J: Increase focus distance
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS && !jPressed)
    {
        dofFocusDistance += DOF_FOCUS_STEP;
        if (dofFocusDistance > 100.0f) dofFocusDistance = 100.0f;
        std::cout << "DoF Focus Distance: " << dofFocusDistance << std::endl;
        jPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_J) == GLFW_RELEASE)
    {
        jPressed = false;
    }
    
    // H: Cycle debug view modes (includes DoF debug modes)
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS && !hPressed)
    {
        // Cycle: 0 (normal) -> 5 (depth) -> 6 (CoC) -> 0
        if (debugViewMode == 0) {
            debugViewMode = 5;
            std::cout << "Debug View: Depth Visualization" << std::endl;
        } else if (debugViewMode == 5) {
            debugViewMode = 6;
            std::cout << "Debug View: Circle of Confusion" << std::endl;
        } else {
            debugViewMode = 0;
            std::cout << "Debug View: Normal" << std::endl;
        }
        hPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_H) == GLFW_RELEASE)
    {
        hPressed = false;
    }
}

// Particle system controls (Phase 10 - Medium Feature)
void processParticleControls(GLFWwindow* window)
{
    // F10: Burst particles
    if (glfwGetKey(window, GLFW_KEY_F10) == GLFW_PRESS && !f10Pressed)
    {
        if (particleSystem) {
            particleSystem->Burst(200);
            std::cout << "Particle Burst Triggered! (200 particles)" << std::endl;
        }
        f10Pressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_F10) == GLFW_RELEASE)
    {
        f10Pressed = false;
    }
    
    // X: Toggle particles
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS && !xPressed)
    {
        enableParticles = !enableParticles;
        if (particleSystem) {
            particleSystem->SetEmitting(enableParticles);
        }
        std::cout << "Particles " << (enableParticles ? "ENABLED" : "DISABLED") << std::endl;
        xPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_X) == GLFW_RELEASE)
    {
        xPressed = false;
    }
    
    // Z: Change particle type
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS && !zPressed)
    {
        currentParticleType = (currentParticleType + 1) % 4;
        const char* particleTypes[] = {"Fire", "Smoke", "Spark", "Magic"};
        std::cout << "Particle Type: " << particleTypes[currentParticleType] << std::endl;
        
        if (particleSystem) {
            EmitterConfig config = particleSystem->GetEmitterConfig();
            config.type = static_cast<ParticleType>(currentParticleType);
            
            // Adjust config based on type
            switch (currentParticleType) {
                case 0: // Fire
                    config.gravity = glm::vec3(0.0f, 0.5f, 0.0f);
                    config.startColor = glm::vec4(1.0f, 0.8f, 0.3f, 1.0f);
                    config.endColor = glm::vec4(1.0f, 0.2f, 0.0f, 0.0f);
                    config.minSize = 0.2f;
                    config.maxSize = 0.6f;
                    break;
                case 1: // Smoke
                    config.gravity = glm::vec3(0.0f, 0.8f, 0.0f);
                    config.startColor = glm::vec4(0.5f, 0.5f, 0.5f, 0.8f);
                    config.endColor = glm::vec4(0.3f, 0.3f, 0.3f, 0.0f);
                    config.minSize = 0.3f;
                    config.maxSize = 1.0f;
                    break;
                case 2: // Spark
                    config.gravity = glm::vec3(0.0f, -5.0f, 0.0f);
                    config.startColor = glm::vec4(1.0f, 0.9f, 0.5f, 1.0f);
                    config.endColor = glm::vec4(1.0f, 0.5f, 0.1f, 0.0f);
                    config.minSize = 0.05f;
                    config.maxSize = 0.15f;
                    config.minSpeed = 3.0f;
                    config.maxSpeed = 8.0f;
                    break;
                case 3: // Magic
                    config.gravity = glm::vec3(0.0f, 0.0f, 0.0f);
                    config.startColor = glm::vec4(0.5f, 0.0f, 1.0f, 1.0f);
                    config.endColor = glm::vec4(0.0f, 1.0f, 1.0f, 0.0f);
                    config.minSize = 0.1f;
                    config.maxSize = 0.4f;
                    break;
            }
            particleSystem->SetEmitterConfig(config);
        }
        zPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_RELEASE)
    {
        zPressed = false;
    }
}

// CSM controls (Phase 11 - Hard Feature)
void processCSMControls(GLFWwindow* window)
{
    // F11: Toggle CSM
    if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS && !f11Pressed)
    {
        enableCSM = !enableCSM;
        std::cout << "Cascaded Shadow Maps " << (enableCSM ? "ENABLED" : "DISABLED") << std::endl;
        f11Pressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_RELEASE)
    {
        f11Pressed = false;
    }
    
    // F12: Toggle cascade visualization
    if (glfwGetKey(window, GLFW_KEY_F12) == GLFW_PRESS && !f12Pressed)
    {
        visualizeCSMCascades = !visualizeCSMCascades;
        std::cout << "CSM Cascade Visualization " << (visualizeCSMCascades ? "ENABLED" : "DISABLED") << std::endl;
        f12Pressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_F12) == GLFW_RELEASE)
    {
        f12Pressed = false;
    }

    // < (comma): Decrease split lambda (more linear)
    static bool commaPressed = false;
    if (glfwGetKey(window, GLFW_KEY_COMMA) == GLFW_PRESS && !commaPressed)
    {
        csmSplitLambda -= 0.1f;
        if (csmSplitLambda < 0.0f) csmSplitLambda = 0.0f;
        if (csmShadowMap) csmShadowMap->SetSplitLambda(csmSplitLambda);
        std::cout << "CSM Split Lambda: " << csmSplitLambda << " (more linear)" << std::endl;
        commaPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_COMMA) == GLFW_RELEASE)
    {
        commaPressed = false;
    }
    
    // > (period): Increase split lambda (more logarithmic)
    static bool periodPressed = false;
    if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS && !periodPressed)
    {
        csmSplitLambda += 0.1f;
        if (csmSplitLambda > 1.0f) csmSplitLambda = 1.0f;
        if (csmShadowMap) csmShadowMap->SetSplitLambda(csmSplitLambda);
        std::cout << "CSM Split Lambda: " << csmSplitLambda << " (more logarithmic)" << std::endl;
        periodPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_RELEASE)
    {
        periodPressed = false;
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

// Update FPS counter
void updateFPS(GLFWwindow* window)
{
    double currentTime = glfwGetTime();
    frameCount++;
    
    if (currentTime - lastTime >= 1.0)
    {
        currentFPS = frameCount;
        frameCount = 0;
        lastTime = currentTime;
    }
}

// Render fullscreen quad
void renderQuad()
{
    static unsigned int quadVAO = 0;
    static unsigned int quadVBO;
    
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// Render ground plane
void renderGroundPlane(unsigned int shader, const glm::mat4& model)
{
    if (groundPlaneVAO == 0)
    {
        float planeVertices[] = {
            // positions            // normals              // texcoords
             25.0f, 0.0f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
            -25.0f, 0.0f,  25.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
            -25.0f, 0.0f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
            
             25.0f, 0.0f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
            -25.0f, 0.0f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
             25.0f, 0.0f, -25.0f,  0.0f, 1.0f, 0.0f,  25.0f, 25.0f
        };
        
        glGenVertexArrays(1, &groundPlaneVAO);
        glGenBuffers(1, &groundPlaneVBO);
        glBindVertexArray(groundPlaneVAO);
        glBindBuffer(GL_ARRAY_BUFFER, groundPlaneVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindVertexArray(0);
    }
    
    // Bind ground texture if available
    if (groundPlaneTexture != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, groundPlaneTexture);
        glUniform1i(glGetUniformLocation(shader, "texture_diffuse1"), 0);
    }
    
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(groundPlaneVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// Load and compile shaders
std::string loadShaderFromFile(const char* filePath)
{
    std::string shaderCode;
    std::ifstream shaderFile;
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        shaderFile.open(filePath);
        std::stringstream shaderStream;
        shaderStream << shaderFile.rdbuf();
        shaderFile.close();
        shaderCode = shaderStream.str();
    }
    catch (std::ifstream::failure& e) {
        std::cerr << "ERROR: Shader file not successfully read: " << filePath << std::endl;
    }
    return shaderCode;
}

unsigned int compileShader(unsigned int type, const char* source)
{
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR: Shader compilation failed\n" << infoLog << std::endl;
    }
    return shader;
}

unsigned int createShaderProgram(const char* vertexPath, const char* fragmentPath)
{
    std::string vertexCode = loadShaderFromFile(vertexPath);
    std::string fragmentCode = loadShaderFromFile(fragmentPath);
    
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();
    
    unsigned int vertex = compileShader(GL_VERTEX_SHADER, vShaderCode);
    unsigned int fragment = compileShader(GL_FRAGMENT_SHADER, fShaderCode);
    
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    
    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "ERROR: Shader program linking failed\n" << infoLog << std::endl;
    }
    
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    
    return program;
}

// Load ground texture
unsigned int loadGroundTexture(const char* path)
{
    unsigned int textureID = 0;
    glGenTextures(1, &textureID);
    
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
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

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        std::cout << "[OK] Loaded ground texture: " << path << " (" << width << "x" << height << ")" << std::endl;
    }
    else
    {
        std::cout << "[WARN] Failed to load texture: " << path << std::endl;
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }
    
    return textureID;
}

