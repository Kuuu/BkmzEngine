#include "MyApp.h"
#include "DXErrors.h"
#include "d3dx12.h"
#include <DirectXMath.h>
#include <cstddef>
#include "Cube.h"

namespace dx = DirectX;

void MyApp::Initialize()
{
	dxApp::Initialize();

	customDraw = [this]() { this->CustomDraw(); };

	commandAlloc->Reset();
	commandList->Reset(commandAlloc.Get(), nullptr);

	CreateObjects();
	CreateMaterials();
	

	commandList->Close();
	ID3D12CommandList *cmds[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(1, cmds);
	FlushCommandQueue();

}

void MyApp::CreateMaterials()
{
	defaultMaterial.inputLayout = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(DefaultMaterial::Vertex, Color),
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	defaultMaterial.CreatePSO(device.Get(), backBufferFormat, depthStencilFormat, cbvSrvUavDescriptorSize);
}

void MyApp::CreateObjects()
{
	gameObjects.push_back({});
	gameObjects.back().mesh = std::make_unique<Mesh<DefaultMaterial::Vertex>>(
		Cube(device.Get(), commandList.Get(), &defaultMaterial)
	);

	gameObjects.back().transform.position = { -1.0f, 0, 3.0f };
	gameObjects.back().transform.rotation = { -(DirectX::XM_PIDIV4) / 1.5f, 0, 0, 0 };

	gameObjects.push_back({});
	gameObjects.back().mesh = std::make_unique<Mesh<DefaultMaterial::Vertex>>(
		Cube(device.Get(), commandList.Get(), &defaultMaterial)
	);

	gameObjects.back().transform.position = { 1.0f, 0, 3.0f };
	gameObjects.back().transform.rotation = { -(DirectX::XM_PIDIV4) / 1.5f, 0, 0, 0 };

	gameObjects.push_back({});
	gameObjects.back().mesh = std::make_unique<Mesh<DefaultMaterial::Vertex>>(
		Cube(device.Get(), commandList.Get(), &defaultMaterial)
	);

	gameObjects.back().transform.position = { 0.0f, 1.0f, 3.0f };
	gameObjects.back().transform.rotation = { -(DirectX::XM_PIDIV4) / 1.5f, 0, 0, 0 };
}

void MyApp::Update(float deltaTime)
{
	using namespace dx;

	XMVECTOR pos = XMVectorSet(0.0f, 0.0f, -2.0f, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);

	for (int i = 0; i < gameObjects.size(); i++)
	{
		gameObjects[i].transform.rotation += { 0, deltaTime * 0.5f, 0, 0 };

		//XMMATRIX translation = XMMatrixTranslation(0, 0, 3.0f);
		XMMATRIX translation = XMMatrixTranslationFromVector(gameObjects[i].transform.position);
		XMMATRIX rotationYMatrix = XMMatrixRotationY(XMVectorGetY(gameObjects[i].transform.rotation));
		XMMATRIX rotationXMatrix = XMMatrixRotationX(XMVectorGetX(gameObjects[i].transform.rotation));

		XMMATRIX rotation = rotationYMatrix * rotationXMatrix;

		XMMATRIX world = rotation * translation; // local space to world space
		//XMMATRIX view = XMMatrixIdentity(); // world space to camera space (move everything so camera is at 0,0,0 here)

		XMMATRIX perspProj = XMMatrixPerspectiveFovLH(XM_PIDIV4, AspectRatio(), 0.1f, 1000.0f);

		XMMATRIX worldViewProj = world * view * perspProj;

		DefaultMaterial::ObjectConstants objConstants;
		XMStoreFloat4x4(&objConstants.worldViewProj, XMMatrixTranspose(worldViewProj));

		memcpy(defaultMaterial.cbufferMappedData + i * defaultMaterial.cbufferObjectByteSize, &objConstants, sizeof(DefaultMaterial::ObjectConstants));

	}
}

void MyApp::CustomDraw()
{
	DrawWithMaterial(gameObjects, &defaultMaterial);
}

void MyApp::DrawWithMaterial(const std::vector<GameObject> &objects, Material *material)
{
	commandList->SetPipelineState(material->PSO.Get());
	commandList->SetGraphicsRootSignature(material->rootSignature.Get());
	ID3D12DescriptorHeap *heaps[] = { material->cbvHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(heaps), heaps);

	for (int i = 0; i < objects.size(); i++)
	{
		auto hGPU = CD3DX12_GPU_DESCRIPTOR_HANDLE(
			defaultMaterial.cbvHeap->GetGPUDescriptorHandleForHeapStart(),
			i, // i
			cbvSrvUavDescriptorSize
		);

		commandList->SetGraphicsRootDescriptorTable(0, hGPU);

		commandList->IASetVertexBuffers(0, 1, &objects[i].mesh->vbv);
		commandList->IASetIndexBuffer(&objects[i].mesh->ibv);
		commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


		commandList->DrawIndexedInstanced(objects[i].mesh->GetIndexCount(), 1, 0, 0, 0);
	}
}
