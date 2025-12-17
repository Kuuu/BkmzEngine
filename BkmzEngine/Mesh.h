#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include <vector>
#include "Utils.h"

template <typename Vertex>
class Mesh
{
public:

	Mesh() { }

	std::vector<Vertex> vertices;
	std::vector<std::uint16_t> indexes;

	D3D12_VERTEX_BUFFER_VIEW vbv;
	D3D12_INDEX_BUFFER_VIEW ibv;

	ComPtr<ID3D12Resource> vertexBufferGPU = nullptr;
	ComPtr<ID3D12Resource> vertexBufferUploader = nullptr;

	ComPtr<ID3D12Resource> indexBufferGPU = nullptr;
	ComPtr<ID3D12Resource> indexBufferUploader = nullptr;

	UINT64 vbByteSize;
	UINT64 ibByteSize;

	//int cbufferIndex = 0;

public:

	void SetVertices(std::vector<Vertex> input)
	{
		vertices = input;
		vbByteSize = vertices.size() * sizeof(Vertex);
	}

	void SetIndexes(std::vector<std::uint16_t> input)
	{
		indexes = input;
		ibByteSize = indexes.size() * sizeof(std::uint16_t);
	}

	int GetIndexCount()
	{
		return indexes.size();
	}

	void InitBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
	{
		vertexBufferGPU = Utils::CreateDefaultBuffer(device, commandList, vertices.data(), vbByteSize, vertexBufferUploader);

		vbv.BufferLocation = vertexBufferGPU->GetGPUVirtualAddress();
		vbv.StrideInBytes = sizeof(Vertex);
		vbv.SizeInBytes = vbByteSize;


		indexBufferGPU = Utils::CreateDefaultBuffer(device, commandList, indexes.data(), ibByteSize, indexBufferUploader);

		ibv.BufferLocation = indexBufferGPU->GetGPUVirtualAddress();
		ibv.Format = DXGI_FORMAT_R16_UINT;
		ibv.SizeInBytes = ibByteSize;
	}

};