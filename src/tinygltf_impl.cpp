// Separate implementation file for tinygltf to avoid STB conflicts
// STB_IMAGE is already implemented in Texture.cpp, so we don't redefine it

#define TINYGLTF_IMPLEMENTATION
// DO NOT define STB implementations - they're already in Texture.cpp
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION

#include <tiny_gltf.h>
#include <stb_image.h>

// Provide implementations for image loading/writing that tinygltf expects
namespace tinygltf {

bool LoadImageData(Image *image, const int image_idx, std::string *err,
                   std::string *warn, int req_width, int req_height,
                   const unsigned char *bytes, int size, void *user_data) {
    // Use stbi from Texture.cpp to load image data
    int w, h, comp;
    unsigned char *data = stbi_load_from_memory(bytes, size, &w, &h, &comp, 0);
    if (!data) {
        if (err) {
            (*err) += "Unknown image format or failed to decode image.\n";
        }
        return false;
    }

    if (req_width > 0) {
        if (req_width != w) {
            stbi_image_free(data);
            if (err) {
                (*err) += "Image width mismatch.\n";
            }
            return false;
        }
    }

    if (req_height > 0) {
        if (req_height != h) {
            stbi_image_free(data);
            if (err) {
                (*err) += "Image height mismatch.\n";
            }
            return false;
        }
    }

    image->width = w;
    image->height = h;
    image->component = comp;
    image->image.resize(static_cast<size_t>(w * h * comp));
    std::copy(data, data + w * h * comp, image->image.begin());
    stbi_image_free(data);

    return true;
}

bool WriteImageData(const std::string *basepath, const std::string *filename,
                    const Image *image, bool embedImages,
                    const FsCallbacks *fs_cb, const URICallbacks *uri_cb,
                    std::string *out_uri, void *user_data) {
    // We don't need to write images for loading, so just return false
    return false;
}

}  // namespace tinygltf
