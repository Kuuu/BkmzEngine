#include "MyApp.h"
#include "DXErrors.h"
#include "d3dx12.h"
#include <DirectXMath.h>
#include <cstddef>

namespace dx = DirectX;

void MyApp::Initialize()
{
	dxApp::Initialize();

	commandAlloc->Reset();
	commandList->Reset(commandAlloc.Get(), nullptr);

	cubes.push_back({ device.Get(), commandList.Get() });
	cubes.back().position = { -1.0f, 0, 3.0f };

	cubes.push_back({ device.Get(), commandList.Get() });
	cubes.back().position = { 1.0f, 0, 3.0f };

	defaultMaterial.inputLayout = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Material::Vertex, Color),
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	
	defaultMaterial.CreatePSO(device.Get(), backBufferFormat, depthStencilFormat, cbvSrvUavDescriptorSize);
	

	commandList->Close();
	ID3D12CommandList *cmds[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(1, cmds);
	FlushCommandQueue();

}

void MyApp::CreateInputLayout()
{
	
}

void MyApp::Update(float deltaTime)
{
	using namespace dx;

	rotationY += deltaTime * 0.5f;

	for (int i = 0; i < 2; i++)
	{

		//XMMATRIX translation = XMMatrixTranslation(0, 0, 3.0f);
		XMMATRIX translation = XMMatrixTranslationFromVector(cubes[i].position);
		XMMATRIX rotationYMatrix = XMMatrixRotationY(rotationY);
		XMMATRIX rotationXMatrix = XMMatrixRotationX(-(XM_PIDIV4) / 1.5f);

		XMMATRIX rotation = rotationYMatrix * rotationXMatrix;

		XMMATRIX world = rotation * translation; // local space to world space
		//XMMATRIX view = XMMatrixIdentity(); // world space to camera space (move everything so camera is at 0,0,0 here)

		XMVECTOR pos = XMVectorSet(0.0f, 0.0f, -2.0f, 1.0f);
		XMVECTOR target = XMVectorZero();
		XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		XMMATRIX view = XMMatrixLookAtLH(pos, target, up);

		/*
		XMMATRIX proj = XMMatrixOrthographicOffCenterLH(
			0.0f, static_cast<float>(width),   // left, right
			static_cast<float>(height), 0.0f,  // bottom, top (flip Y)
			0.0f, 1.0f                         // near, far
		);
		*/

		XMMATRIX perspProj = XMMatrixPerspectiveFovLH(XM_PIDIV4, AspectRatio(), 0.1f, 1000.0f);



		XMMATRIX worldViewProj = world * view * perspProj;

		Material::ObjectConstants objConstants;
		XMStoreFloat4x4(&objConstants.worldViewProj, XMMatrixTranspose(worldViewProj));

		memcpy(defaultMaterial.cbufferMappedData + i * defaultMaterial.cbufferObjectByteSize, &objConstants, sizeof(Material::ObjectConstants));

	}
}

void MyApp::Draw()
{
	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished
	// execution on the GPU.
	DX_CALL(commandAlloc->Reset());

	// A command list can be reset after it has been added to the
	// command queue via ExecuteCommandList. Reusing the command list reuses memory.
	DX_CALL(commandList->Reset(commandAlloc.Get(), defaultMaterial.PSO.Get()));

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
	commandList->SetPipelineState(defaultMaterial.PSO.Get());
	commandList->SetGraphicsRootSignature(defaultMaterial.rootSignature.Get());
	ID3D12DescriptorHeap *heaps[] = { defaultMaterial.cbvHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(heaps), heaps);

	for (int i = 0; i < 2; i++)
	{
		auto hGPU = CD3DX12_GPU_DESCRIPTOR_HANDLE(
			defaultMaterial.cbvHeap->GetGPUDescriptorHandleForHeapStart(),
			i, // i
			cbvSrvUavDescriptorSize
		);

		commandList->SetGraphicsRootDescriptorTable(0, hGPU);

		commandList->IASetVertexBuffers(0, 1, &cubes[i].vbv);
		commandList->IASetIndexBuffer(&cubes[i].ibv);
		commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


		commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
	}

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
