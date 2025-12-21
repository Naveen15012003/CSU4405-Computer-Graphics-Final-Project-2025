#pragma once
#include <glad/glad.h>

class PostProcessor {
public:
    PostProcessor(unsigned int width, unsigned int height);
    ~PostProcessor();

    void Initialize();
    void BeginRender();
    void EndRender();
    void Render(float exposure, bool enableBloom, bool enableGamma);
    void Render(float exposure, bool enableBloom, bool enableGamma, float bloomStrength, int debugMode);
    void Cleanup();

    // Resize framebuffers
    void Resize(unsigned int width, unsigned int height);

    // Check if initialized
    bool IsInitialized() const { return initialized; }

    // Public getters for debug visualization
    unsigned int GetHDRTexture() const { return hdrColorBuffer; }
    unsigned int GetBrightTexture() const { return brightColorBuffer; }
    unsigned int GetBloomTexture() const { return bloomColorBuffers[0]; }

private:
    unsigned int width, height;
    bool initialized;

    // HDR framebuffer
    unsigned int hdrFBO;
    unsigned int hdrColorBuffer;
    unsigned int hdrDepthBuffer;

    // Bloom framebuffers (ping-pong)
    unsigned int bloomFBO[2];
    unsigned int bloomColorBuffers[2];

    // Bright pass framebuffer
    unsigned int brightFBO;
    unsigned int brightColorBuffer;

    // Screen quad VAO/VBO
    unsigned int quadVAO, quadVBO;

    // Shaders
    unsigned int postprocessShader;
    unsigned int bloomExtractShader;
    unsigned int blurShader;

    void CreateFramebuffers();
    void CreateScreenQuad();
    void LoadShaders();
    void RenderScreenQuad();
    void ApplyBloom();
    bool CheckFramebufferStatus(unsigned int fbo, const char* name);
};
