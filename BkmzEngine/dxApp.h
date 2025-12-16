#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include "FrameTimer.h"
#include <DirectXMath.h>
#include <string>

using Microsoft::WRL::ComPtr;

class dxApp
{
public:
	dxApp(HWND hwnd, UINT width, UINT height) : hwnd(hwnd), width(width), height(height) {}

	~dxApp()
	{
		if (device != nullptr)
		{
			FlushCommandQueue();
		}
	}

	virtual void Initialize();
	virtual void Update(float deltaTime) = 0;
	virtual void Draw() = 0;
	float AspectRatio() const;

	ID3D12Resource* CurrentBackBuffer() const
	{
		return swapChainBuffer[currBackBuffer].Get();
	}

	void CalculateFrameStats(float timerTotalTime);

	UINT CalcConstantBufferByteSize(UINT byteSize);

public:
	FrameTimer timer;

private:
	void EnableDebugLayer();
	void CreateDXGIFactory();
	void CreateDevice();
	void CreateFence();
	void GetDescriptorSizes();
	void CreateCommandObjects();
	void CreateSwapChain();
	void CreateDescriptorHeaps();
	void CreateRTV();
	void CreateDSV();
	void SetViewport();
	void ExecuteCommands();

protected:
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;
	void FlushCommandQueue();

	ComPtr<ID3DBlob> LoadShader(const std::wstring &filename);

protected:
	HWND hwnd;

	UINT width;
	UINT height;

	UINT64 currentFence = 0;

	UINT rtvDescriptorSize = 0;
	UINT dsvDescriptorSize = 0;
	UINT cbvSrvUavDescriptorSize = 0;

	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_D16_UNORM;
	static constexpr UINT swapChainBufferCount = 2;
	int currBackBuffer = 0;
	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;

	FLOAT clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

protected:
	ComPtr<IDXGIFactory4> dxgiFactory;
	ComPtr<ID3D12Device> device;
	ComPtr<ID3D12Fence> fence;

	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<ID3D12CommandAllocator> commandAlloc;
	ComPtr<ID3D12GraphicsCommandList> commandList;

	ComPtr<IDXGISwapChain> swapChain;

	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	ComPtr<ID3D12DescriptorHeap> dsvHeap;

	ComPtr<ID3D12Resource> swapChainBuffer[swapChainBufferCount];
	ComPtr<ID3D12Resource> depthStencilBuffer;

private:
	int frameCnt = 0;
	float timeElapsed = 0.0f;
};