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

void dxApp::ExecuteCommands()
{
	ID3D12CommandList *cmdsLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
}

Microsoft::WRL::ComPtr<ID3D12Resource> dxApp::CreateDefaultBuffer(
	const void *initData,
	UINT64 byteSize,
	Microsoft::WRL::ComPtr<ID3D12Resource>&uploadBuffer)
{
	ComPtr<ID3D12Resource> defaultBuffer;

	// Create the actual default buffer resource.
	D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
	DX_CALL(device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(defaultBuffer.GetAddressOf()))
	);

	// In order to copy CPU memory data into our default buffer, we need
	// to create an intermediate upload heap.
	heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
	DX_CALL(device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploadBuffer.GetAddressOf()))
	);

	// Describe the data we want to copy into the default buffer.
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	// Schedule to copy the data to the default buffer resource.
	// At a high level, the helper function UpdateSubresources
	// will copy the CPU memory into the intermediate upload heap.
	// Then, using ID3D12CommandList::CopySubresourceRegion,
	// the intermediate upload heap data will be copied to mBuffer.
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_COPY_DEST);
	commandList->ResourceBarrier(1,
		&barrier
	);

	UpdateSubresources<1>(commandList.Get(),
		defaultBuffer.Get(), uploadBuffer.Get(),
		0, 0, 1, &subResourceData);

	barrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_GENERIC_READ);
	commandList->ResourceBarrier(1,
		&barrier
	);

	// Note: uploadBuffer has to be kept alive after the above function
	// calls because the command list has not been executed yet that
	// performs the actual copy.
	// The caller can Release the uploadBuffer after it knows the copy
	// has been executed.
	return defaultBuffer;
}

UINT dxApp::CalcConstantBufferByteSize(UINT byteSize)
{
	// Constant buffers must be a multiple of the minimum hardware
	// allocation size (usually 256 bytes). So round up to nearest
	// multiple of 256. We do this by adding 255 and then masking off
	// the lower 2 bytes which store all bits < 256.
	// Example: Suppose byteSize = 300.
	// (300 + 255) & ~255
	// 555 & ~255
	// 0x022B & ~0x00ff
	// 0x022B & 0xff00
	// 0x0200
	// 512
	return (byteSize + 255) & ~255;
}

ComPtr<ID3DBlob> dxApp::LoadShader(const std::wstring &filename)
{
	ComPtr<ID3DBlob> blob;
	DX_CALL(D3DReadFileToBlob(filename.c_str(), &blob));
	return blob;
}
