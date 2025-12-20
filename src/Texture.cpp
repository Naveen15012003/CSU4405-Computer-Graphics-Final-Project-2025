#include "Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

Texture::Texture()
    : ID(0), Type(""), Path(""), isCubemap(false)
{
}

std::string Texture::GetProjectRoot()
{
    // Use __FILE__ to find source location, walk up to find assets/
    fs::path sourcePath = fs::path(__FILE__).parent_path();  // src/
    fs::path projectRoot = sourcePath.parent_path();          // project root
    
    // Verify assets/ exists
    fs::path assetsPath = projectRoot / "assets";
    if (fs::exists(assetsPath))
    {
        std::cout << "  [GetProjectRoot] Found project root: " << projectRoot.string() << std::endl;
        return projectRoot.string();
    }
    
    // If not found, try going up more levels (in case we're in bin/Debug)
    for (int i = 0; i < 4; i++)
    {
        projectRoot = projectRoot.parent_path();
        assetsPath = projectRoot / "assets";
        if (fs::exists(assetsPath))
        {
            std::cout << "  [GetProjectRoot] Found project root (after " << (i+2) << " levels up): " << projectRoot.string() << std::endl;
            return projectRoot.string();
        }
    }
    
    // Fallback: return current directory
    std::cerr << "  [GetProjectRoot] WARNING: Could not find project root with assets/ folder" << std::endl;
    return fs::current_path().string();
}

std::string Texture::ResolvePath(const std::string& relativePath, const std::vector<std::string>& searchPaths)
{
    // Try each search path
    for (const auto& searchPath : searchPaths)
    {
        fs::path fullPath = fs::path(searchPath) / relativePath;
        
        if (fs::exists(fullPath))
        {
            std::cout << "  [ResolvePath] ? Resolved: " << fullPath.string() << std::endl;
            return fullPath.string();
        }
    }
    
    // If not found, print what we tried
    std::cerr << "  [ResolvePath] ? Path not found. Tried:" << std::endl;
    for (const auto& searchPath : searchPaths)
    {
        fs::path fullPath = fs::path(searchPath) / relativePath;
        std::cerr << "    " << fullPath.string() << std::endl;
    }
    
    return relativePath;
}

bool Texture::loadTextureFile(const std::string& path, int& width, int& height, int& nrChannels, unsigned char** data, bool flipVertically)
{
    // Get project root for absolute path
    static std::string projectRoot = GetProjectRoot();
    
    // Build absolute path first
    fs::path absolutePath = fs::path(projectRoot) / path;
    
    // Define search paths (absolute first, then relative fallbacks)
    std::vector<std::string> searchPaths = {
        projectRoot,                 // Absolute from project root
        "",                         // Current working directory
        "../",                      // Parent directory
        "../../",                   // Grandparent directory
        "../../../"                 // Great-grandparent directory
    };
    
    // Try to resolve the path
    std::string resolvedPath = ResolvePath(path, searchPaths);
    
    // Set flip mode
    stbi_set_flip_vertically_on_load(flipVertically);
    
    // Attempt to load
    *data = stbi_load(resolvedPath.c_str(), &width, &height, &nrChannels, 0);
    
    if (*data)
    {
        Path = resolvedPath;
        return true;
    }
    
    return false;
}

bool Texture::LoadFromFile(const std::string& path, bool flipVertically)
{
    Type = "diffuse";  // Default type

    // Generate texture
    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_2D, ID);

    // Set texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Load image with path resolution
    int width, height, nrChannels;
    unsigned char* data = nullptr;
    
    if (loadTextureFile(path, width, height, nrChannels, &data, flipVertically))
    {
        GLenum format = GL_RGB;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
        std::cout << "  [Texture] ? Loaded: " << Path << " (" << width << "x" << height << ", " << nrChannels << " channels)" << std::endl;
        return true;
    }
    else
    {
        std::cerr << "  [Texture] ? Failed to load: " << path << std::endl;
        std::cerr << "    STB Error: " << stbi_failure_reason() << std::endl;
        if (data) stbi_image_free(data);
        return false;
    }
}

bool Texture::LoadCubemap(const std::string faces[6])
{
    isCubemap = true;

    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ID);

    const char* faceNames[6] = { "right", "left", "top", "bottom", "front", "back" };
    const char* extensions[3] = { ".jpg", ".png", ".jpeg" };
    bool allLoaded = true;
    int width = 0, height = 0, nrChannels = 0;

    std::cout << "\n=== LOADING SKYBOX CUBEMAP ===" << std::endl;
    std::cout << "Expected files in assets/skybox/:" << std::endl;
    std::cout << "  right.jpg/png, left.jpg/png, top.jpg/png," << std::endl;
    std::cout << "  bottom.jpg/png, front.jpg/png, back.jpg/png" << std::endl;
    std::cout << "Supported extensions: .jpg, .png, .jpeg\n" << std::endl;
    
    for (unsigned int i = 0; i < 6; i++)
    {
        std::cout << "Loading cubemap face [" << (i + 1) << "/6]: " << faceNames[i] << std::endl;
        
        // Extract base path without extension
        std::string basePath = faces[i];
        size_t lastDot = basePath.find_last_of(".");
        if (lastDot != std::string::npos)
        {
            basePath = basePath.substr(0, lastDot);
        }
        
        unsigned char* data = nullptr;
        bool loaded = false;
        std::string successfulPath;
        
        // Try each extension
        for (int extIdx = 0; extIdx < 3 && !loaded; extIdx++)
        {
            std::string testPath = basePath + extensions[extIdx];
            if (loadTextureFile(testPath, width, height, nrChannels, &data, false))
            {
                loaded = true;
                successfulPath = testPath;
                break;
            }
        }
        
        if (loaded && data)
        {
            GLenum format = GL_RGB;
            if (nrChannels == 1)
                format = GL_RED;
            else if (nrChannels == 3)
                format = GL_RGB;
            else if (nrChannels == 4)
                format = GL_RGBA;

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
            std::cout << "  ? Face loaded: " << faceNames[i] << " (" << width << "x" << height << ", " << nrChannels << " channels)" << std::endl;
        }
        else
        {
            std::cerr << "  ? FAILED to load face: " << faceNames[i] << std::endl;
            std::cerr << "    Tried: " << basePath << ".jpg/.png/.jpeg" << std::endl;
            if (data) stbi_image_free(data);
            allLoaded = false;
        }
    }

    if (allLoaded)
    {
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        std::cout << "\n??? SKYBOX LOADED SUCCESSFULLY ???\n" << std::endl;
        return true;
    }
    else
    {
        std::cerr << "\n??? SKYBOX LOADING FAILED ???" << std::endl;
        std::cerr << "Some cubemap faces could not be loaded." << std::endl;
        std::cerr << "\nRequired naming convention:" << std::endl;
        std::cerr << "  Place 6 square images in assets/skybox/ named:" << std::endl;
        std::cerr << "  right.jpg, left.jpg, top.jpg, bottom.jpg, front.jpg, back.jpg" << std::endl;
        std::cerr << "  (or .png, .jpeg extensions)" << std::endl;
        std::cerr << "  All images must be same resolution and square.\n" << std::endl;
        return false;
    }
}

std::vector<std::string> Texture::ListFilesInDirectory(const std::string& directoryPath)
{
    std::vector<std::string> files;
    
    try
    {
        if (fs::exists(directoryPath) && fs::is_directory(directoryPath))
        {
            for (const auto& entry : fs::directory_iterator(directoryPath))
            {
                if (entry.is_regular_file())
                {
                    files.push_back(entry.path().filename().string());
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "  [ListFilesInDirectory] Error: " << e.what() << std::endl;
    }
    
    return files;
}

bool Texture::LoadSkyboxAuto(const std::string& skyboxDirectory, Texture& cubemapTexture)
{
    std::cout << "\n=== AUTO-DETECTING SKYBOX NAMING CONVENTION ===" << std::endl;
    
    // Get project root and build absolute path
    std::string projectRoot = GetProjectRoot();
    fs::path skyboxPath = fs::path(projectRoot) / skyboxDirectory;
    
    std::cout << "Skybox directory: " << skyboxPath.string() << std::endl;
    std::cout << "Directory exists: " << (fs::exists(skyboxPath) ? "YES" : "NO") << std::endl;
    
    // List all files in skybox directory
    std::vector<std::string> filesFound = ListFilesInDirectory(skyboxPath.string());
    
    if (filesFound.empty())
    {
        std::cerr << "\n? No files found in skybox directory!" << std::endl;
        std::cerr << "Expected directory: " << skyboxPath.string() << std::endl;
        return false;
    }
    
    std::cout << "\nFiles found in skybox directory (" << filesFound.size() << "):" << std::endl;
    for (const auto& file : filesFound)
    {
        std::cout << "  - " << file << std::endl;
    }
    
    // Define naming convention sets
    struct NamingSet
    {
        std::string name;
        std::vector<std::string> baseNames;  // right, left, top, bottom, front, back order
    };
    
    std::vector<NamingSet> namingSets = {
        { "Standard", { "right", "left", "top", "bottom", "front", "back" } },
        { "Unity/PX-NX", { "px", "nx", "py", "ny", "pz", "nz" } },
        { "Detailed", { "posx", "negx", "posy", "negy", "posz", "negz" } }
    };
    
    const char* extensions[] = { ".jpg", ".png", ".jpeg", ".JPG", ".PNG", ".JPEG" };
    
    // Try each naming convention
    for (const auto& namingSet : namingSets)
    {
        std::cout << "\nTrying naming convention: " << namingSet.name << std::endl;
        
        bool allFound = true;
        std::string faces[6];
        
        for (size_t i = 0; i < 6; i++)
        {
            bool faceFound = false;
            
            // Try each extension
            for (const char* ext : extensions)
            {
                std::string filename = namingSet.baseNames[i] + ext;
                fs::path facePath = skyboxPath / filename;
                
                if (fs::exists(facePath))
                {
                    faces[i] = (fs::path(skyboxDirectory) / filename).string();
                    std::cout << "  ? Found: " << filename << std::endl;
                    faceFound = true;
                    break;
                }
            }
            
            if (!faceFound)
            {
                std::cout << "  ? Missing: " << namingSet.baseNames[i] << ".*" << std::endl;
                allFound = false;
                break;
            }
        }
        
        if (allFound)
        {
            std::cout << "\n??? Detected naming convention: " << namingSet.name << " ???" << std::endl;
            std::cout << "Loading skybox..." << std::endl;
            return cubemapTexture.LoadCubemap(faces);
        }
    }
    
    // If we get here, no complete set was found
    std::cerr << "\n??? NO COMPLETE SKYBOX SET FOUND ???" << std::endl;
    std::cerr << "\nSupported naming conventions:" << std::endl;
    std::cerr << "  Set A (Standard): right, left, top, bottom, front, back" << std::endl;
    std::cerr << "  Set B (Unity): px, nx, py, ny, pz, nz" << std::endl;
    std::cerr << "  Set C (Detailed): posx, negx, posy, negy, posz, negz" << std::endl;
    std::cerr << "\nSupported extensions: .jpg, .png, .jpeg (case-insensitive)" << std::endl;
    std::cerr << "\nPlace 6 matching files in: " << skyboxPath.string() << std::endl;
    
    return false;
}

void Texture::Bind(unsigned int slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    if (isCubemap)
        glBindTexture(GL_TEXTURE_CUBE_MAP, ID);
    else
        glBindTexture(GL_TEXTURE_2D, ID);
}

void Texture::Unbind() const
{
    if (isCubemap)
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    else
        glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::Delete()
{
    if (ID != 0)
    {
        glDeleteTextures(1, &ID);
        ID = 0;
    }
}
