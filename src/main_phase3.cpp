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
