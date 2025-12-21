#pragma once

#include <d3d12.h>
#include <vector>
#include "Maths.h"
#include "Core.h"
#include "AABB.h"

struct STATIC_VERTEX
{
	Vec3 pos;
	Vec3 normal;
	Vec3 tangent;
	float tu;
	float tv;
};

struct ANIMATED_VERTEX
{
	Vec3 pos;
	Vec3 normal;
	Vec3 tangent;
	float tu;
	float tv;
	unsigned int bonesIDs[4];
	float boneWeights[4];
};

class VertexLayoutCache
{
public:
	static const D3D12_INPUT_LAYOUT_DESC& getStaticLayout()
	{
		static const D3D12_INPUT_ELEMENT_DESC inputLayoutStatic[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};
		static const D3D12_INPUT_LAYOUT_DESC desc = {inputLayoutStatic, 4};
		return desc;
	}
	static const D3D12_INPUT_LAYOUT_DESC& getStaticInstancedLayout()
	{
		static const D3D12_INPUT_ELEMENT_DESC inputLayoutStaticInstanced[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
			{ "WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
			{ "WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
			{ "WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		};
		static const D3D12_INPUT_LAYOUT_DESC desc = { inputLayoutStaticInstanced, 8 };
		return desc;
	}
	static const D3D12_INPUT_LAYOUT_DESC& getAnimatedLayout()
	{
		static const D3D12_INPUT_ELEMENT_DESC inputLayoutAnimated[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BONEIDS", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BONEWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};
		static const D3D12_INPUT_LAYOUT_DESC desc = {inputLayoutAnimated, 6};
		return desc;
	}
	static const D3D12_INPUT_LAYOUT_DESC& getlineLayout()
	{
		static const D3D12_INPUT_ELEMENT_DESC inputLayoutline[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};
		static const D3D12_INPUT_LAYOUT_DESC desc = { inputLayoutline, 2 };
		return desc;
	}
};
struct InstanceData
{
	Matrix world;
};
class Mesh
{
public:
	ID3D12Resource* vertexBuffer;
	ID3D12Resource* indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vbView;
	D3D12_INDEX_BUFFER_VIEW ibView;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	unsigned int numMeshIndices;

	ID3D12Resource* instanceBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW instanceVBView;
	int instanceCount = 0;

	bool dynamic = false;
	AABB localAABB;
	void initInstanceBuffer(Core* core, const std::vector<InstanceData>& instances)
	{
		instanceCount = (int)instances.size();

		UINT bufferSize = sizeof(InstanceData) * instanceCount;

		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = bufferSize;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		core->device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&instanceBuffer)
		);

		void* mapped;
		instanceBuffer->Map(0, nullptr, &mapped);
		memcpy(mapped, instances.data(), bufferSize);
		instanceBuffer->Unmap(0, nullptr);

		instanceVBView.BufferLocation = instanceBuffer->GetGPUVirtualAddress();
		instanceVBView.StrideInBytes = sizeof(InstanceData);
		instanceVBView.SizeInBytes = bufferSize;
	}

	void init(Core* core, void* vertices, int vertexSizeInBytes, int numVertices, unsigned int* indices, int numIndices)
	{
		D3D12_HEAP_PROPERTIES heapprops;
		memset(&heapprops, 0, sizeof(D3D12_HEAP_PROPERTIES));
		heapprops.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapprops.CreationNodeMask = 1;
		heapprops.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC vbDesc;
		memset(&vbDesc, 0, sizeof(D3D12_RESOURCE_DESC));
		vbDesc.Width = numVertices * vertexSizeInBytes;
		vbDesc.Height = 1;
		vbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		vbDesc.DepthOrArraySize = 1;
		vbDesc.MipLevels = 1;
		vbDesc.SampleDesc.Count = 1;
		vbDesc.SampleDesc.Quality = 0;
		vbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		HRESULT hr;
		hr = core->device->CreateCommittedResource(&heapprops, D3D12_HEAP_FLAG_NONE, &vbDesc, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(&vertexBuffer));

		core->uploadResource(vertexBuffer, vertices, numVertices * vertexSizeInBytes, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		D3D12_RESOURCE_DESC ibDesc;
		memset(&ibDesc, 0, sizeof(D3D12_RESOURCE_DESC));
		ibDesc.Width = numIndices * sizeof(unsigned int);
		ibDesc.Height = 1;
		ibDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		ibDesc.DepthOrArraySize = 1;
		ibDesc.MipLevels = 1;
		ibDesc.SampleDesc.Count = 1;
		ibDesc.SampleDesc.Quality = 0;
		ibDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		hr = core->device->CreateCommittedResource(&heapprops, D3D12_HEAP_FLAG_NONE, &ibDesc, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(&indexBuffer));

		core->uploadResource(indexBuffer, indices, numIndices * sizeof(unsigned int), D3D12_RESOURCE_STATE_INDEX_BUFFER);

		vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vbView.StrideInBytes = vertexSizeInBytes;
		vbView.SizeInBytes = numVertices * vertexSizeInBytes;

		ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
		ibView.Format = DXGI_FORMAT_R32_UINT;
		ibView.SizeInBytes = numIndices * sizeof(unsigned int);

		numMeshIndices = numIndices;
	}
	void init(Core* core, std::vector<STATIC_VERTEX> vertices, std::vector<unsigned int> indices)
	{
		init(core, &vertices[0], sizeof(STATIC_VERTEX), vertices.size(), &indices[0], indices.size());
		inputLayoutDesc = VertexLayoutCache::getStaticLayout();
		localAABB.min = vertices[0].pos;
		localAABB.max = vertices[0].pos;

		for (const auto& v : vertices)
		{
			localAABB.min.x = std::min(localAABB.min.x, v.pos.x);
			localAABB.min.y = std::min(localAABB.min.y, v.pos.y);
			localAABB.min.z = std::min(localAABB.min.z, v.pos.z);

			localAABB.max.x = std::max(localAABB.max.x, v.pos.x);
			localAABB.max.y = std::max(localAABB.max.y, v.pos.y);
			localAABB.max.z = std::max(localAABB.max.z, v.pos.z);
		}
	}
	void init(Core* core, std::vector<ANIMATED_VERTEX> vertices, std::vector<unsigned int> indices)
	{
		init(core, &vertices[0], sizeof(ANIMATED_VERTEX), vertices.size(), &indices[0], indices.size());
		inputLayoutDesc = VertexLayoutCache::getAnimatedLayout();
		localAABB.min = vertices[0].pos;
		localAABB.max = vertices[0].pos;

		for (const auto& v : vertices)
		{
			localAABB.min.x = std::min(localAABB.min.x, v.pos.x);
			localAABB.min.y = std::min(localAABB.min.y, v.pos.y);
			localAABB.min.z = std::min(localAABB.min.z, v.pos.z);

			localAABB.max.x = std::max(localAABB.max.x, v.pos.x);
			localAABB.max.y = std::max(localAABB.max.y, v.pos.y);
			localAABB.max.z = std::max(localAABB.max.z, v.pos.z);
		}
	}
	void initDynamic(Core* core, int maxVertices, int vertexStride)
	{
		dynamic = true;
		numMeshIndices = maxVertices;

		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = maxVertices * vertexStride;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		core->device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&vertexBuffer)
		);

		vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vbView.StrideInBytes = vertexStride;
		vbView.SizeInBytes = maxVertices * vertexStride;
	}
	template<typename T>
	void updateDynamic(Core* core, const std::vector<T>& data)
	{
		numMeshIndices = (int)data.size();

		void* mapped = nullptr;
		vertexBuffer->Map(0, nullptr, &mapped);
		memcpy(mapped, data.data(), data.size() * sizeof(T));
		vertexBuffer->Unmap(0, nullptr);
	}
	void draw(Core* core)
	{
		core->getCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		core->getCommandList()->IASetVertexBuffers(0, 1, &vbView);
		core->getCommandList()->IASetIndexBuffer(&ibView);
		core->getCommandList()->DrawIndexedInstanced(numMeshIndices, 1, 0, 0, 0);
	}
	void drawInstanced(Core* core)
	{
		D3D12_VERTEX_BUFFER_VIEW views[2] = {
		vbView,
		instanceVBView
		};
		//char buf[256];
		//sprintf_s(buf,"DrawInstanced: numMeshIndices=%u instanceCount=%u\n",numMeshIndices, instanceCount);
		//OutputDebugStringA(buf);
		core->getCommandList()->IASetVertexBuffers(0, 2, views);
		core->getCommandList()->IASetIndexBuffer(&ibView);
		core->getCommandList()->IASetPrimitiveTopology(
			D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
		);

		core->getCommandList()->DrawIndexedInstanced(
			numMeshIndices,
			instanceCount,
			0, 0, 0
		);
	}
	void drawLines(Core* core)
	{
		core->getCommandList()->IASetVertexBuffers(0, 1, &vbView);
		core->getCommandList()->DrawInstanced(numMeshIndices, 1, 0, 0);
	}

	void cleanUp()
	{
		indexBuffer->Release();
		vertexBuffer->Release();
	}
	~Mesh()
	{
		cleanUp();
	}
};