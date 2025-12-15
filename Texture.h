#pragma once
#include "stb_image.h"
#include "core.h"
#include <string>
#include <map>
class Texture {
public:
    ID3D12Resource* tex;
    int heapOffset;
    DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    int width = 0;
    int height = 0;
    int channels = 0;
    bool load(Core* core, const std::string& filename);
};
class TextureManager
{
public:
    std::map<std::string, Texture*> loadedTextures;

    Texture* loadTexture(Core* core, const std::string& filename);

    int find(const std::string& filename) {
        if (loadedTextures.count(filename))
            return loadedTextures[filename]->heapOffset;
        return -1; // Ã»ÕÒµ½
    }
    void freeTextures(Core* core) {
        for (auto& pair : loadedTextures) {
            pair.second->tex->Release();
            delete pair.second;
        }
    }
};