#include "PostProcessor.h"
#include <iostream>
#include <fstream>
#include <sstream>

// Helper function to load shader
static std::string LoadShaderSource(const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Failed to open shader: " << path << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Helper function to compile shader
static unsigned int CompileShader(const char* vertexPath, const char* fragmentPath) {
    std::string vertexCode = LoadShaderSource(vertexPath);
    std::string fragmentCode = LoadShaderSource(fragmentPath);

    if (vertexCode.empty() || fragmentCode.empty()) {
        std::cerr << "[ERROR] Shader source empty" << std::endl;
        return 0;
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);

    int success;
    char infoLog[512];
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cerr << "[ERROR] Vertex shader compilation failed: " << vertexPath << "\n" << infoLog << std::endl;
        return 0;
    }

    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cerr << "[ERROR] Fragment shader compilation failed: " << fragmentPath << "\n" << infoLog << std::endl;
        glDeleteShader(vertex);
        return 0;
    }

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "[ERROR] Shader program linking failed\n" << infoLog << std::endl;
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return 0;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return program;
}

PostProcessor::PostProcessor(unsigned int width, unsigned int height)
    : width(width), height(height), initialized(false),
      hdrFBO(0), hdrColorBuffer(0), hdrDepthBuffer(0), hdrDepthTexture(0),
      brightFBO(0), brightColorBuffer(0),
      quadVAO(0), quadVBO(0),
      postprocessShader(0), bloomExtractShader(0), blurShader(0),
      dofShader(0), dofBlurShader(0)
{
    bloomFBO[0] = bloomFBO[1] = 0;
    bloomColorBuffers[0] = bloomColorBuffers[1] = 0;
    dofFBO[0] = dofFBO[1] = 0;
    dofColorBuffers[0] = dofColorBuffers[1] = 0;
}

PostProcessor::~PostProcessor() {
    Cleanup();
}

void PostProcessor::Initialize() {
    std::cout << "\n=== POST-PROCESSOR INITIALIZATION ===" << std::endl;
    std::cout << "Resolution: " << width << "x" << height << std::endl;

    CreateFramebuffers();
    CreateScreenQuad();
    LoadShaders();

    if (postprocessShader != 0) {
        initialized = true;
        std::cout << "[OK] Post-processor initialized successfully\n" << std::endl;
    } else {
        std::cerr << "[ERROR] Post-processor initialization failed\n" << std::endl;
    }
}

void PostProcessor::CreateFramebuffers() {
    // HDR Framebuffer with MRT (Multiple Render Targets)
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

    // COLOR_ATTACHMENT0: Main HDR scene color
    glGenTextures(1, &hdrColorBuffer);
    glBindTexture(GL_TEXTURE_2D, hdrColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrColorBuffer, 0);

    // COLOR_ATTACHMENT1: Bright color for bloom
    glGenTextures(1, &brightColorBuffer);
    glBindTexture(GL_TEXTURE_2D, brightColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, brightColorBuffer, 0);

    // Enable MRT - render to both color attachments
    GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, drawBuffers);
    std::cout << "[HDR FBO] MRT enabled: COLOR_ATTACHMENT0 + COLOR_ATTACHMENT1" << std::endl;

    // NEW: Depth texture (for DoF) instead of renderbuffer
    glGenTextures(1, &hdrDepthTexture);
    glBindTexture(GL_TEXTURE_2D, hdrDepthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, hdrDepthTexture, 0);
    std::cout << "[HDR FBO] Depth texture attached for DoF" << std::endl;

    if (!CheckFramebufferStatus(hdrFBO, "HDR")) return;

    // Bloom ping-pong framebuffers (half resolution for performance)
    unsigned int bloomWidth = width / 2;
    unsigned int bloomHeight = height / 2;

    // We'll use brightColorBuffer as source, so we only need ping-pong buffers
    for (int i = 0; i < 2; i++) {
        glGenFramebuffers(1, &bloomFBO[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO[i]);

        glGenTextures(1, &bloomColorBuffers[i]);
        glBindTexture(GL_TEXTURE_2D, bloomColorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, bloomWidth, bloomHeight, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomColorBuffers[i], 0);

        if (!CheckFramebufferStatus(bloomFBO[i], i == 0 ? "Bloom Ping" : "Bloom Pong")) return;
    }

    // NEW: DoF ping-pong framebuffers (full resolution for quality)
    std::cout << "[DoF] Creating DoF framebuffers..." << std::endl;
    for (int i = 0; i < 2; i++) {
        glGenFramebuffers(1, &dofFBO[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, dofFBO[i]);

        glGenTextures(1, &dofColorBuffers[i]);
        glBindTexture(GL_TEXTURE_2D, dofColorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dofColorBuffers[i], 0);

        if (!CheckFramebufferStatus(dofFBO[i], i == 0 ? "DoF Ping" : "DoF Pong")) return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    std::cout << "[POST-PROCESSOR] All framebuffers created (MRT + DoF support)" << std::endl;
}

void PostProcessor::CreateScreenQuad() {
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void PostProcessor::LoadShaders() {
    std::cout << "Loading post-process shaders..." << std::endl;

    postprocessShader = CompileShader("shaders/postprocess.vert", "shaders/postprocess.frag");
    if (postprocessShader) {
        std::cout << "[OK] Post-process shader loaded" << std::endl;
    }

    bloomExtractShader = CompileShader("shaders/postprocess.vert", "shaders/bloom_extract.frag");
    if (bloomExtractShader) {
        std::cout << "[OK] Bloom extract shader loaded" << std::endl;
    }

    blurShader = CompileShader("shaders/postprocess.vert", "shaders/blur.frag");
    if (blurShader) {
        std::cout << "[OK] Blur shader loaded" << std::endl;
    }

    // NEW: Load DoF shaders
    dofShader = CompileShader("shaders/postprocess.vert", "shaders/dof.frag");
    if (dofShader) {
        std::cout << "[OK] DoF composite shader loaded" << std::endl;
    } else {
        std::cerr << "[WARN] DoF shader failed to load - DoF will be disabled" << std::endl;
    }

    dofBlurShader = CompileShader("shaders/postprocess.vert", "shaders/dof_blur.frag");
    if (dofBlurShader) {
        std::cout << "[OK] DoF blur shader loaded" << std::endl;
    } else {
        std::cerr << "[WARN] DoF blur shader failed to load - DoF will be disabled" << std::endl;
    }
}

void PostProcessor::BeginRender() {
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

    // DEBUG: Confirm HDR FBO is bound
    GLint currentFBO;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFBO);
    static bool debugOnce = false;
    if (!debugOnce) {
        std::cout << "[DEBUG] BeginRender() bound FBO: " << currentFBO << " (expected: " << hdrFBO << ")" << std::endl;
        debugOnce = true;
    }

    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);  // CRITICAL: Re-enable depth test for scene rendering
}

void PostProcessor::EndRender() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcessor::ApplyBloom() {
    // brightColorBuffer (COLOR_ATTACHMENT1) already contains bright pixels from MRT
    // We skip the extraction step and go straight to blur
    
    // Ping-pong blur (10 iterations = 5 horizontal + 5 vertical)
    bool horizontal = true;
    int iterations = 10;

    glUseProgram(blurShader);
    for (int i = 0; i < iterations; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO[horizontal]);
        glViewport(0, 0, width / 2, height / 2);
        glUniform1i(glGetUniformLocation(blurShader, "horizontal"), horizontal);

        glActiveTexture(GL_TEXTURE0);
        // First iteration uses brightColorBuffer, subsequent iterations ping-pong
        glBindTexture(GL_TEXTURE_2D, i == 0 ? brightColorBuffer : bloomColorBuffers[!horizontal]);
        glUniform1i(glGetUniformLocation(blurShader, "image"), 0);

        RenderScreenQuad();
        horizontal = !horizontal;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcessor::ApplyDoF(const DoFSettings& settings, int debugMode) {
    if (!dofBlurShader || !dofShader) {
        return;  // DoF shaders not available
    }

    // Step 1: Create blurred version of the scene using ping-pong blur
    bool horizontal = true;
    int blurIterations = 6;  // 3 horizontal + 3 vertical passes
    float blurScale = settings.maxBlur * 2.0f;  // Scale blur based on max blur setting

    glUseProgram(dofBlurShader);
    for (int i = 0; i < blurIterations; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, dofFBO[horizontal]);
        glViewport(0, 0, width, height);
        glUniform1i(glGetUniformLocation(dofBlurShader, "horizontal"), horizontal);
        glUniform1f(glGetUniformLocation(dofBlurShader, "blurScale"), blurScale);

        glActiveTexture(GL_TEXTURE0);
        // First iteration uses hdrColorBuffer, subsequent iterations ping-pong
        glBindTexture(GL_TEXTURE_2D, i == 0 ? hdrColorBuffer : dofColorBuffers[!horizontal]);
        glUniform1i(glGetUniformLocation(dofBlurShader, "image"), 0);

        RenderScreenQuad();
        horizontal = !horizontal;
    }

    // Step 2: Composite sharp + blurred using depth-based CoC
    // The result goes to dofColorBuffers[0] (we'll read from this in final pass)
    glBindFramebuffer(GL_FRAMEBUFFER, dofFBO[0]);
    glViewport(0, 0, width, height);
    
    glUseProgram(dofShader);

    // Bind scene color (sharp)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrColorBuffer);
    glUniform1i(glGetUniformLocation(dofShader, "sceneColor"), 0);

    // Bind depth texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, hdrDepthTexture);
    glUniform1i(glGetUniformLocation(dofShader, "sceneDepth"), 1);

    // Bind blurred scene
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, dofColorBuffers[1]);  // Last ping-pong result
    glUniform1i(glGetUniformLocation(dofShader, "blurredScene"), 2);

    // Set DoF parameters
    glUniform1f(glGetUniformLocation(dofShader, "focusDistance"), settings.focusDistance);
    glUniform1f(glGetUniformLocation(dofShader, "focusRange"), settings.focusRange);
    glUniform1f(glGetUniformLocation(dofShader, "maxBlur"), settings.maxBlur);
    glUniform1f(glGetUniformLocation(dofShader, "aperture"), settings.aperture);
    glUniform1i(glGetUniformLocation(dofShader, "enableDoF"), settings.enabled ? 1 : 0);
    glUniform1i(glGetUniformLocation(dofShader, "debugMode"), debugMode);
    glUniform1f(glGetUniformLocation(dofShader, "nearPlane"), settings.nearPlane);
    glUniform1f(glGetUniformLocation(dofShader, "farPlane"), settings.farPlane);

    RenderScreenQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcessor::Render(float exposure, bool enableBloom, bool enableGamma) {
    DoFSettings defaultDoF;
    defaultDoF.enabled = false;
    Render(exposure, enableBloom, enableGamma, 0.4f, 0, defaultDoF);
}

void PostProcessor::Render(float exposure, bool enableBloom, bool enableGamma, float bloomStrength, int debugMode) {
    DoFSettings defaultDoF;
    defaultDoF.enabled = false;
    Render(exposure, enableBloom, enableGamma, bloomStrength, debugMode, defaultDoF);
}

void PostProcessor::Render(float exposure, bool enableBloom, bool enableGamma, float bloomStrength, 
                           int debugMode, const DoFSettings& dofSettings) {
    // Apply bloom if enabled (only in normal mode, not DoF debug modes)
    if (enableBloom && debugMode == 0) {
        ApplyBloom();
    }

    // Apply DoF if enabled (includes debug modes 5 and 6)
    bool applyDoF = dofSettings.enabled || debugMode == 5 || debugMode == 6;
    if (applyDoF && dofShader && dofBlurShader) {
        ApplyDoF(dofSettings, debugMode);
    }

    // === FINAL POST-PROCESS TO SCREEN ===
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    
    // Clear to prevent leftover fragments
    glClear(GL_COLOR_BUFFER_BIT);
    
    // CRITICAL: Set OpenGL state for fullscreen quad
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    // Debug output (once)
    static bool debugOnce = false;
    if (!debugOnce) {
        std::cout << "[POST-PROCESS] HDR texture ID: " << hdrColorBuffer << std::endl;
        std::cout << "[POST-PROCESS] Bright texture ID: " << brightColorBuffer << std::endl;
        std::cout << "[POST-PROCESS] Bloom texture ID: " << bloomColorBuffers[0] << std::endl;
        std::cout << "[POST-PROCESS] Depth texture ID: " << hdrDepthTexture << std::endl;
        std::cout << "[POST-PROCESS] DoF texture ID: " << dofColorBuffers[0] << std::endl;
        debugOnce = true;
    }

    glUseProgram(postprocessShader);

    // Bind the appropriate texture for each debug mode
    glActiveTexture(GL_TEXTURE0);
    if (debugMode == 5 || debugMode == 6) {
        // DoF debug modes: use DoF output (which contains debug visualization)
        glBindTexture(GL_TEXTURE_2D, dofColorBuffers[0]);
    } else if (applyDoF && debugMode == 0) {
        // Normal mode with DoF: use DoF composited result
        glBindTexture(GL_TEXTURE_2D, dofColorBuffers[0]);
    } else if (debugMode == 0 || debugMode == 1) {
        // Normal mode or HDR only: use HDR buffer
        glBindTexture(GL_TEXTURE_2D, hdrColorBuffer);
    } else if (debugMode == 2) {
        // Bright pass: use bright color buffer
        glBindTexture(GL_TEXTURE_2D, brightColorBuffer);
    } else if (debugMode == 3) {
        // Bloom blur: use blurred bloom
        glBindTexture(GL_TEXTURE_2D, bloomColorBuffers[0]);
    } else if (debugMode == 4) {
        // DoF blur only (blurred scene without compositing)
        glBindTexture(GL_TEXTURE_2D, dofColorBuffers[1]);
    }
    glUniform1i(glGetUniformLocation(postprocessShader, "hdrBuffer"), 0);

    // Bloom texture (only used in normal mode)
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bloomColorBuffers[0]);
    glUniform1i(glGetUniformLocation(postprocessShader, "bloomBlur"), 1);

    // Set uniforms
    glUniform1f(glGetUniformLocation(postprocessShader, "exposure"), exposure);
    glUniform1i(glGetUniformLocation(postprocessShader, "enableBloom"), (enableBloom && debugMode == 0 && !applyDoF) ? 1 : 0);
    glUniform1i(glGetUniformLocation(postprocessShader, "enableGamma"), enableGamma ? 1 : 0);
    glUniform1f(glGetUniformLocation(postprocessShader, "bloomStrength"), bloomStrength);
    
    // For DoF debug modes, we bypass tone mapping to show raw values
    int effectiveDebugMode = (debugMode == 5 || debugMode == 6) ? debugMode : debugMode;
    glUniform1i(glGetUniformLocation(postprocessShader, "debugMode"), effectiveDebugMode);

    // Render fullscreen quad
    RenderScreenQuad();
    
    // Restore OpenGL state
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void PostProcessor::RenderScreenQuad() {
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

bool PostProcessor::CheckFramebufferStatus(unsigned int fbo, const char* name) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[ERROR] " << name << " framebuffer incomplete: " << status << std::endl;
        return false;
    }
    std::cout << "[OK] " << name << " framebuffer complete" << std::endl;
    return true;
}

void PostProcessor::Resize(unsigned int newWidth, unsigned int newHeight) {
    width = newWidth;
    height = newHeight;
    Cleanup();
    Initialize();
}

void PostProcessor::Cleanup() {
    if (hdrFBO) glDeleteFramebuffers(1, &hdrFBO);
    if (hdrColorBuffer) glDeleteTextures(1, &hdrColorBuffer);
    if (hdrDepthBuffer) glDeleteRenderbuffers(1, &hdrDepthBuffer);
    if (hdrDepthTexture) glDeleteTextures(1, &hdrDepthTexture);

    if (brightFBO) glDeleteFramebuffers(1, &brightFBO);
    if (brightColorBuffer) glDeleteTextures(1, &brightColorBuffer);

    for (int i = 0; i < 2; i++) {
        if (bloomFBO[i]) glDeleteFramebuffers(1, &bloomFBO[i]);
        if (bloomColorBuffers[i]) glDeleteTextures(1, &bloomColorBuffers[i]);
        if (dofFBO[i]) glDeleteFramebuffers(1, &dofFBO[i]);
        if (dofColorBuffers[i]) glDeleteTextures(1, &dofColorBuffers[i]);
    }

    if (quadVAO) glDeleteVertexArrays(1, &quadVAO);
    if (quadVBO) glDeleteBuffers(1, &quadVBO);

    if (postprocessShader) glDeleteProgram(postprocessShader);
    if (bloomExtractShader) glDeleteProgram(bloomExtractShader);
    if (blurShader) glDeleteProgram(blurShader);
    if (dofShader) glDeleteProgram(dofShader);
    if (dofBlurShader) glDeleteProgram(dofBlurShader);

    initialized = false;
}
