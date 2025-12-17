#pragma once
#include "Mesh.h"
#include "DefaultMaterial.h"
#include "d3d12.h"

class Cube : public Mesh<DefaultMaterial::Vertex>
{
public:
	Cube(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, Material* mat)
	{
		std::vector<DefaultMaterial::Vertex> verts = {
		{{-0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},
		{{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},
		{{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 0.0f, 1.0f}},

		{{-0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 1.0f, 1.0f}},
		{{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 1.0f, 1.0f}},
		{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},
		{{0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f, 1.0f}},
		};

		std::vector<std::uint16_t> indexes = {
			2, 3, 6,
			6, 3, 7,
			1, 0, 5,
			5, 0, 4,
			0, 2, 4,
			4, 2, 6,
			3, 1, 7,
			7, 1, 5,
			0, 1, 2,
			2, 1, 3,
			6, 7, 4,
			4, 7, 5
		};

		SetVertices(verts);
		SetIndexes(indexes);
		InitBuffers(device, commandList);

		mat->objectCount += 1;
	}
};