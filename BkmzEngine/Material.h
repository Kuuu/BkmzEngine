#pragma once
#include <d3d12.h>
#include "d3dcompiler.h"
#include <wrl.h>
#include "Utils.h"

class Material
{
public:

	~Material()
	{
		if (cbufferUploader && cbufferMappedData)
		{
			cbufferUploader->Unmap(0, nullptr);
			cbufferMappedData = nullptr;
		}
	}

	struct Vertex
	{
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT4 Color;
	};

	struct ObjectConstants
	{
		DirectX::XMFLOAT4X4 worldViewProj;
	};

	Microsoft::WRL::ComPtr<ID3D12PipelineState> PSO;
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;

	ComPtr<ID3D12DescriptorHeap> cbvHeap;
	ComPtr<ID3D12Resource> cbufferUploader = nullptr;
	ComPtr<ID3D12RootSignature> rootSignature;

	UINT cbufferObjectByteSize = 0;
	UINT8 *cbufferMappedData = nullptr;

	const int objectCount = 2;

	ComPtr<ID3DBlob> LoadShader(const std::wstring &filename)
	{
		ComPtr<ID3DBlob> blob;
		DX_CALL(D3DReadFileToBlob(filename.c_str(), &blob));
		return blob;
	}


	void CreatePSO(ID3D12Device* device, DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthStencilFormat, UINT cbvSrvUavDescriptorSize)
	{
		cbufferObjectByteSize = Utils::CalcConstantBufferByteSize(sizeof(ObjectConstants));

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		auto cbDesc = CD3DX12_RESOURCE_DESC::Buffer(cbufferObjectByteSize * objectCount);

		DX_CALL(device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&cbDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&cbufferUploader)
		));

		DX_CALL(cbufferUploader->Map(0, nullptr, reinterpret_cast<void **>(&cbufferMappedData)));

		D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
		cbvHeapDesc.NumDescriptors = objectCount;
		cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		DX_CALL(device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&cbvHeap)));

		// Address to start of the buffer (0th constant buffer).
		for (int i = 0; i < objectCount; i++)
		{
			D3D12_GPU_VIRTUAL_ADDRESS cbAddress = cbufferUploader->GetGPUVirtualAddress() + i * cbufferObjectByteSize;

			auto hCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(
				cbvHeap->GetCPUDescriptorHandleForHeapStart(),
				i,
				cbvSrvUavDescriptorSize
			);


			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
			cbvDesc.BufferLocation = cbAddress;
			cbvDesc.SizeInBytes = cbufferObjectByteSize;

			device->CreateConstantBufferView(&cbvDesc, hCPU);
		}

		// Root parameter can be a table, root descriptor or root constants.
		CD3DX12_ROOT_PARAMETER slotRootParameter[1];

		// Create a single descriptor table of CBVs.
		CD3DX12_DESCRIPTOR_RANGE cbvTable;

		cbvTable.Init(
			D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
			1, // Number of descriptors in table
			0
		);// base shader register arguments are bound to for this root parameter
		slotRootParameter[0].InitAsDescriptorTable(
			1, // Number of ranges
			&cbvTable // Pointer to array of ranges
		);


		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0,
			nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		// create a root signature with a single slot which points to a
		// descriptor range consisting of a single constant buffer.
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;

		DX_CALL(D3D12SerializeRootSignature(&rootSigDesc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(),
			errorBlob.GetAddressOf()));

		DX_CALL(device->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(&rootSignature))
		);

		auto vsBytecode = LoadShader(L"VertexShader.cso");
		auto psBytecode = LoadShader(L"PixelShader.cso");

		CD3DX12_RASTERIZER_DESC rsDesc(D3D12_DEFAULT);

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
		ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

		psoDesc.InputLayout = { inputLayout.data(), (UINT)inputLayout.size() };
		psoDesc.pRootSignature = rootSignature.Get();
		psoDesc.VS = {
			reinterpret_cast<BYTE *>(vsBytecode->GetBufferPointer()),
			vsBytecode->GetBufferSize()
		};
		psoDesc.PS = {
			reinterpret_cast<BYTE *>(psBytecode->GetBufferPointer()),
			psBytecode->GetBufferSize()
		};

		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = backBufferFormat;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;
		psoDesc.DSVFormat = depthStencilFormat;
		ComPtr<ID3D12PipelineState> mPSO;
		DX_CALL(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&PSO)));
	}
};