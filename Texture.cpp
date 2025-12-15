#define STB_IMAGE_IMPLEMENTATION
#include "Texture.h"

bool Texture::load(Core* core, const std::string& filename) {
	unsigned char* texels = stbi_load(filename.c_str(), &width, &height, &channels, 0);
	unsigned char* uploadData = texels;
	if (!texels)
	{
		printf("Failed to load texture: %s\n", filename.c_str());
		return false;
	}
	if (channels == 3) {
		channels = 4;
		unsigned char* texelsWithAlpha = new unsigned char[width * height * channels];
		for (int i = 0; i < (width * height); i++) {
			texelsWithAlpha[i * 4] = texels[i * 3];
			texelsWithAlpha[(i * 4) + 1] = texels[(i * 3) + 1];
			texelsWithAlpha[(i * 4) + 2] = texels[(i * 3) + 2];
			texelsWithAlpha[(i * 4) + 3] = 255;
		}
		uploadData = texelsWithAlpha;
		// Initialize texture using width, height, channels, and texelsWithAlpha
	}
	else {
		// Initialize texture using width, height, channels, and texels
	}

	D3D12_HEAP_PROPERTIES heapDesc;
	memset(&heapDesc, 0, sizeof(D3D12_HEAP_PROPERTIES));
	heapDesc.Type = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_DESC textureDesc;
	memset(&textureDesc, 0, sizeof(D3D12_RESOURCE_DESC));
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.MipLevels = 1;
	textureDesc.Format = format;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	core->device->CreateCommittedResource(&heapDesc, D3D12_HEAP_FLAG_NONE, &textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&tex));

	D3D12_RESOURCE_DESC desc = tex->GetDesc();
	unsigned long long size;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	core->device->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, NULL, NULL, &size);
	unsigned int alignedWidth = ((width * channels) + 255) & ~255;
	core->uploadResource(tex, uploadData, alignedWidth * height,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &footprint);

	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = core->srvHeap.getNextCPUHandle();
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	core->device->CreateShaderResourceView(tex, &srvDesc, srvHandle);
	heapOffset = core->srvHeap.used - 1;
	if (uploadData != texels)
		delete[] uploadData;
	stbi_image_free(texels);
}

Texture* TextureManager::loadTexture(Core* core, const std::string& filename)
{
	if (loadedTextures.find(filename) != loadedTextures.end())
		return loadedTextures[filename];

	Texture* tex = new Texture();
	tex->load(core, filename);
	loadedTextures[filename] = tex;

	return tex;
}