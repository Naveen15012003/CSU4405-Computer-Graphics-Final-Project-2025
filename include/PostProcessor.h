#pragma once
#include <glad/glad.h>

// DoF parameters structure for cleaner API
struct DoFSettings {
    bool enabled = false;
    float focusDistance = 10.0f;   // Distance to focus plane (world units)
    float focusRange = 5.0f;       // Range of sharp focus
    float maxBlur = 1.0f;          // Maximum blur amount (0-1)
    float aperture = 1.0f;         // Simulated aperture (affects bokeh intensity)
    float nearPlane = 0.1f;        // Camera near plane
    float farPlane = 100.0f;       // Camera far plane
};

class PostProcessor {
public:
    PostProcessor(unsigned int width, unsigned int height);
    ~PostProcessor();

    void Initialize();
    void BeginRender();
    void EndRender();
    void Render(float exposure, bool enableBloom, bool enableGamma);
    void Render(float exposure, bool enableBloom, bool enableGamma, float bloomStrength, int debugMode);
    
    // New: Render with DoF support
    void Render(float exposure, bool enableBloom, bool enableGamma, float bloomStrength, 
                int debugMode, const DoFSettings& dofSettings);
    
    void Cleanup();

    // Resize framebuffers
    void Resize(unsigned int width, unsigned int height);

    // Check if initialized
    bool IsInitialized() const { return initialized; }

    // Public getters for debug visualization
    unsigned int GetHDRTexture() const { return hdrColorBuffer; }
    unsigned int GetBrightTexture() const { return brightColorBuffer; }
    unsigned int GetBloomTexture() const { return bloomColorBuffers[0]; }
    unsigned int GetDepthTexture() const { return hdrDepthTexture; }  // NEW: Access depth

private:
    unsigned int width, height;
    bool initialized;

    // HDR framebuffer
    unsigned int hdrFBO;
    unsigned int hdrColorBuffer;
    unsigned int hdrDepthBuffer;      // Renderbuffer (existing)
    unsigned int hdrDepthTexture;     // NEW: Depth as texture for DoF

    // Bloom framebuffers (ping-pong)
    unsigned int bloomFBO[2];
    unsigned int bloomColorBuffers[2];

    // Bright pass framebuffer
    unsigned int brightFBO;
    unsigned int brightColorBuffer;

    // NEW: DoF framebuffers (ping-pong for blur)
    unsigned int dofFBO[2];
    unsigned int dofColorBuffers[2];

    // Screen quad VAO/VBO
    unsigned int quadVAO, quadVBO;

    // Shaders
    unsigned int postprocessShader;
    unsigned int bloomExtractShader;
    unsigned int blurShader;
    unsigned int dofShader;           // NEW: DoF composite shader
    unsigned int dofBlurShader;       // NEW: DoF blur shader

    void CreateFramebuffers();
    void CreateScreenQuad();
    void LoadShaders();
    void RenderScreenQuad();
    void ApplyBloom();
    void ApplyDoF(const DoFSettings& settings, int debugMode);  // NEW: Apply DoF effect
    bool CheckFramebufferStatus(unsigned int fbo, const char* name);
};
