#pragma once
#include "Material.h"
#include "d3d12.h"
#include <DirectXMath.h>

class DefaultMaterial : public Material
{
public:

	struct Vertex
	{
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT4 Color;
	};

	struct ObjectConstants
	{
		DirectX::XMFLOAT4X4 worldViewProj;
	};

	void CreatePSO(ID3D12Device *device, DXGI_FORMAT backBufferFormat, 
		DXGI_FORMAT depthStencilFormat, UINT cbvSrvUavDescriptorSize) override
	{
		Material::CreatePSO(device, backBufferFormat, depthStencilFormat, cbvSrvUavDescriptorSize, sizeof(ObjectConstants));
	}
};