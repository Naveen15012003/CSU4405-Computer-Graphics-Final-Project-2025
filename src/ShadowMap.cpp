#include "ShadowMap.h"

ShadowMap::ShadowMap(unsigned int width, unsigned int height)
    : m_Width(width), m_Height(height), m_FBO(0), m_DepthTexture(0), m_PreviousFBO(0)
{
    Init();
}

ShadowMap::~ShadowMap()
{
    if (m_DepthTexture != 0)
    {
        glDeleteTextures(1, &m_DepthTexture);
        m_DepthTexture = 0;
    }
    
    if (m_FBO != 0)
    {
        glDeleteFramebuffers(1, &m_FBO);
        m_FBO = 0;
    }
}

void ShadowMap::Init()
{
    // Create framebuffer
    glGenFramebuffers(1, &m_FBO);
    
    // Create depth texture
    glGenTextures(1, &m_DepthTexture);
    glBindTexture(GL_TEXTURE_2D, m_DepthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_Width, m_Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    
    // Texture parameters for shadow sampling
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    
    // Border color (white = no shadow outside frustum)
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    
    // Attach depth texture to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthTexture, 0);
    
    // No color attachment
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    
    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "ERROR: Shadow map framebuffer is not complete!" << std::endl;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    std::cout << "Shadow map created: " << m_Width << "x" << m_Height << std::endl;
}

void ShadowMap::BindForWriting()
{
    // Save current framebuffer
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &m_PreviousFBO);
    
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    glViewport(0, 0, m_Width, m_Height);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::BindForReading(GLenum textureUnit)
{
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D, m_DepthTexture);
}

void ShadowMap::Unbind()
{
    // Restore previous framebuffer (HDR FBO or screen FBO)
    glBindFramebuffer(GL_FRAMEBUFFER, m_PreviousFBO);
}
