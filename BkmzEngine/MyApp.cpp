#include "MyApp.h"
#include "DXErrors.h"
#include "d3dx12.h"
#include <DirectXMath.h>
#include <cstddef>

namespace dx = DirectX;

struct Vertex
{
	dx::XMFLOAT3 Position;
	dx::XMFLOAT4 Color;
};

struct ObjectConstants
{
	dx::XMFLOAT4X4 worldViewProj = dxApp::Identity4x4();
};

void MyApp::Initialize()
{
	dxApp::Initialize();

	commandAlloc->Reset();
	commandList->Reset(commandAlloc.Get(), nullptr);

	/*
	Vertex verts[vertexCount] = {
		{{0.2f, 0.2f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
		{{0.5f, 0.7f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
		{{0.8f, 0.2f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
	};
	*/
	

	
	Vertex verts[vertexCount] = {
		{{200.0f, 500.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
		{{500.0f, 200.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
		{{700.0f, 500.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
	};
	
	

	const UINT64 vbByteSize = vertexCount * sizeof(Vertex);

	std::uint16_t indexes[indexCount] = {0, 1, 2};

	const UINT64 ibByteSize = indexCount * sizeof(std::uint16_t);


	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, Color),
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	vertexBufferGPU = CreateDefaultBuffer(verts, vbByteSize, vertexBufferUploader);

	vbv.BufferLocation = vertexBufferGPU->GetGPUVirtualAddress();
	vbv.StrideInBytes = sizeof(Vertex);
	vbv.SizeInBytes = vbByteSize;


	indexBufferGPU = CreateDefaultBuffer(indexes, ibByteSize, indexBufferUploader);

	ibv.BufferLocation = indexBufferGPU->GetGPUVirtualAddress();
	ibv.Format = DXGI_FORMAT_R16_UINT;
	ibv.SizeInBytes = ibByteSize;

	cbufferObjectByteSize = CalcConstantBufferByteSize(sizeof(ObjectConstants));

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	auto cbDesc = CD3DX12_RESOURCE_DESC::Buffer(cbufferObjectByteSize * 1);

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
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	DX_CALL(device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&cbvHeap)));

	// Address to start of the buffer (0th constant buffer).
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = cbufferUploader->GetGPUVirtualAddress();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = cbufferObjectByteSize;

	device->CreateConstantBufferView(&cbvDesc, cbvHeap->GetCPUDescriptorHandleForHeapStart());

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

	commandList->Close();
	ID3D12CommandList *cmds[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(1, cmds);
	FlushCommandQueue();

}

void MyApp::CreateInputLayout()
{
	
}

void MyApp::Update()
{
	using namespace dx;

	XMMATRIX world = XMMatrixIdentity();
	XMMATRIX view = XMMatrixIdentity();

	XMMATRIX proj = XMMatrixOrthographicOffCenterLH(
		0.0f, static_cast<float>(width),   // left, right
		static_cast<float>(height), 0.0f,  // bottom, top (flip Y)
		0.0f, 1.0f                         // near, far
	);


	
	XMMATRIX worldViewProj = world * view * proj;

	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.worldViewProj, XMMatrixTranspose(worldViewProj));

	memcpy(cbufferMappedData, &objConstants, sizeof(ObjectConstants));
}

void MyApp::Draw()
{
	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished
	// execution on the GPU.
	DX_CALL(commandAlloc->Reset());

	// A command list can be reset after it has been added to the
	// command queue via ExecuteCommandList. Reusing the command list reuses memory.
	DX_CALL(commandList->Reset(commandAlloc.Get(), PSO.Get()));

	// Set the viewport and scissor rect. This needs to be reset
	// whenever the command list is reset.
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);

	// Indicate a state transition on the resource usage.
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	commandList->ResourceBarrier(
		1, &barrier
	);

	// Specify the buffers we are going to render to.
	auto rtvHandle = CurrentBackBufferView();
	auto dsvHandle = DepthStencilView();

	commandList->OMSetRenderTargets(1, &rtvHandle,
		true, &dsvHandle);

	// Clear the back buffer and depth buffer.
	commandList->ClearRenderTargetView(
		CurrentBackBufferView(),
		clearColor, 0, nullptr);

	commandList->ClearDepthStencilView(
		DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH |
		D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Draw Here
	commandList->SetPipelineState(PSO.Get());
	commandList->SetGraphicsRootSignature(rootSignature.Get());
	ID3D12DescriptorHeap *heaps[] = { cbvHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(heaps), heaps);
	commandList->SetGraphicsRootDescriptorTable(
		0,
		cbvHeap->GetGPUDescriptorHandleForHeapStart()
	);
	commandList->IASetVertexBuffers(0, 1, &vbv);
	commandList->IASetIndexBuffer(&ibv);
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);

	// Indicate a state transition on the resource usage.
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);

	commandList->ResourceBarrier(
		1, &barrier);

	// Done recording commands.
	DX_CALL(commandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList *cmdsLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// swap the back and front buffers
	DX_CALL(swapChain->Present(0, 0));
	currBackBuffer = (currBackBuffer + 1) % swapChainBufferCount;

	// Wait until frame commands are complete. This waiting is
	// inefficient and is done for simplicity. Later we will show how to
	// organize our rendering code so we do not have to wait per frame.
	FlushCommandQueue();

}
