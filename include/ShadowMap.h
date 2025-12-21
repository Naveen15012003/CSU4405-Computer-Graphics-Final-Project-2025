#pragma once

#include <glad/glad.h>
#include <iostream>

class ShadowMap
{
public:
    // Constructor
    ShadowMap(unsigned int width = 2048, unsigned int height = 2048);
    
    // Destructor
    ~ShadowMap();

    // Bind for writing (depth pass)
    void BindForWriting();
    
    // Bind for reading (main pass)
    void BindForReading(GLenum textureUnit);
    
    // Unbind (restore previous framebuffer)
    void Unbind();

    // Getters
    unsigned int GetWidth() const { return m_Width; }
    unsigned int GetHeight() const { return m_Height; }
    unsigned int GetDepthTexture() const { return m_DepthTexture; }
    
private:
    unsigned int m_FBO;
    unsigned int m_DepthTexture;
    unsigned int m_Width;
    unsigned int m_Height;
    GLint m_PreviousFBO;  // Store previous FBO to restore after shadow pass
    
    void Init();
};
