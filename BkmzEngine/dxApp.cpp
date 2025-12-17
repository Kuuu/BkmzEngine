#include "dxApp.h"
#include "DXErrors.h"
#include "d3dx12.h"
#include <d3dcompiler.h>

void dxApp::Initialize()
{
	EnableDebugLayer();
	CreateDXGIFactory();
	CreateDevice();
	CreateFence();
	GetDescriptorSizes();
	CreateCommandObjects();
	CreateSwapChain();
	CreateDescriptorHeaps();
	CreateRTV();
	CreateDSV();
	SetViewport();

	//ExecuteCommands();
	//FlushCommandQueue();
}

D3D12_CPU_DESCRIPTOR_HANDLE dxApp::CurrentBackBufferView() const
{
	// CD3DX12 constructor to offset to the RTV of the current back buffer.
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		rtvHeap->GetCPUDescriptorHandleForHeapStart(), // handle start
		currBackBuffer, // index to offset
		rtvDescriptorSize // byte size of descriptor
	);
}

D3D12_CPU_DESCRIPTOR_HANDLE dxApp::DepthStencilView() const
{
	return dsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void dxApp::FlushCommandQueue()
{
	// Advance the fence value to mark commands up to this fence point.
	currentFence++;

	// Add an instruction to the command queue to set a new fence point.
	// Because we are on the GPU timeline, the new fence point wonft be
	// set until the GPU finishes processing all the commands prior to
	// this Signal().
	DX_CALL(commandQueue->Signal(fence.Get(), currentFence));

	// Wait until the GPU has completed commands up to this fence point.
	if (fence->GetCompletedValue() < currentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, NULL, false, EVENT_ALL_ACCESS);

		// Fire event when GPU hits current fence.
		DX_CALL(fence->SetEventOnCompletion(currentFence, eventHandle));

		// Wait until the GPU hits current fence event is fired.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}


float dxApp::AspectRatio() const
{
	return static_cast<float>(width) / height;
}

void dxApp::CalculateFrameStats(float timerTotalTime)
{
	// Code computes the average frames per second, and also the
	// average time it takes to render one frame. These stats
	// are appended to the window caption bar.
	frameCnt++;

	// Compute averages over one second period.
	if ((timerTotalTime - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		std::wstring fpsStr = std::to_wstring(fps);
		std::wstring mspfStr = std::to_wstring(mspf);
		std::wstring windowText = 
			L" fps: " + fpsStr +
			L" mspf: " + mspfStr;

		SetWindowText(hwnd, windowText.c_str());
		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

void dxApp::EnableDebugLayer()
{
#if defined(DEBUG) || defined(_DEBUG)
	ComPtr<ID3D12Debug> debugController;
	DX_CALL(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
}

void dxApp::CreateDXGIFactory()
{
	DX_CALL(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiFactory)));
}

void dxApp::CreateDevice()
{
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr, // default adapter
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&device));

	// Fallback to WARP device.
	if (FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		DX_CALL(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
		DX_CALL(D3D12CreateDevice(
			warpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&device))
		);
	}
}

void dxApp::CreateFence()
{
	DX_CALL(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
}

void dxApp::GetDescriptorSizes()
{
	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	cbvSrvUavDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void dxApp::CreateCommandObjects()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	DX_CALL(device->CreateCommandQueue(
		&queueDesc,
		IID_PPV_ARGS(&commandQueue))
	);

	DX_CALL(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(commandAlloc.GetAddressOf()))
	);

	DX_CALL(device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAlloc.Get(), // Associated command allocator
		nullptr, // Initial PipelineStateObject
		IID_PPV_ARGS(commandList.GetAddressOf()))
	);

	// Close the command list as it is created in the recording state.
	commandList->Close();
}

void dxApp::CreateSwapChain()
{
	swapChain.Reset();
	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = backBufferFormat;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = swapChainBufferCount;
	sd.OutputWindow = hwnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	DX_CALL(dxgiFactory->CreateSwapChain(
		commandQueue.Get(),
		&sd,
		swapChain.GetAddressOf())
	);
}

void dxApp::CreateDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = swapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;

	DX_CALL(device->CreateDescriptorHeap(
		&rtvHeapDesc,
		IID_PPV_ARGS(rtvHeap.GetAddressOf()))
	);

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;

	DX_CALL(device->CreateDescriptorHeap(
		&dsvHeapDesc,
		IID_PPV_ARGS(dsvHeap.GetAddressOf()))
	);
}

void dxApp::CreateRTV()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(
		rtvHeap->GetCPUDescriptorHandleForHeapStart()
	);

	for (UINT i = 0; i < swapChainBufferCount; i++)
	{
		// Get the ith buffer in the swap chain.
		DX_CALL(swapChain->GetBuffer(
			i, IID_PPV_ARGS(&swapChainBuffer[i]))
		);

		// Create an RTV to it.
		device->CreateRenderTargetView(
			swapChainBuffer[i].Get(), nullptr, rtvHeapHandle
		);
		// Next entry in heap.
		rtvHeapHandle.Offset(1, rtvDescriptorSize);
	}
}

void dxApp::CreateDSV()
{
	// Create the depth/stencil buffer and view.
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = width;
	depthStencilDesc.Height = height;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = depthStencilFormat;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = depthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	DX_CALL(device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(depthStencilBuffer.GetAddressOf()))
	);

	// Create descriptor to mip level 0 of entire resource using the
	// format of the resource.
	device->CreateDepthStencilView(
		depthStencilBuffer.Get(),
		nullptr,
		DepthStencilView()
	);

	
	// Transition the resource from its initial state to be used as a depth buffer.
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		depthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_DEPTH_WRITE
	);

	commandAlloc->Reset();
	commandList->Reset(commandAlloc.Get(), nullptr);

	// Transition
	commandList->ResourceBarrier(1, &barrier);

	commandList->Close();
	ID3D12CommandList *cmds[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(1, cmds);
	FlushCommandQueue();
}

void dxApp::SetViewport()
{
	// Set viewport
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = width;
	scissorRect.bottom = height;
}

void dxApp::Draw()
{
	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished
	// execution on the GPU.
	DX_CALL(commandAlloc->Reset());

	// A command list can be reset after it has been added to the
	// command queue via ExecuteCommandList. Reusing the command list reuses memory.
	DX_CALL(commandList->Reset(commandAlloc.Get(), nullptr));

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

	customDraw();

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

void dxApp::ExecuteCommands()
{
	ID3D12CommandList *cmdsLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
}