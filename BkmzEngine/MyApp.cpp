#include "MyApp.h"
#include "DXErrors.h"
#include "d3dx12.h"

void MyApp::Draw()
{
	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished
	// execution on the GPU.
	DX_CALL(commandAlloc->Reset());

	// A command list can be reset after it has been added to the
	// command queue via ExecuteCommandList. Reusing the command list reuses memory.
	DX_CALL(commandList->Reset(commandAlloc.Get(), nullptr));

	// Indicate a state transition on the resource usage.
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	commandList->ResourceBarrier(
		1, &barrier
	);

	// Set the viewport and scissor rect. This needs to be reset
	// whenever the command list is reset.
	commandList->RSSetViewports(1, &viewport);

	// Clear the back buffer and depth buffer.
	commandList->ClearRenderTargetView(
		CurrentBackBufferView(),
		clearColor, 0, nullptr);

	commandList->ClearDepthStencilView(
		DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH |
		D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	auto rtvHandle = CurrentBackBufferView();
	auto dsvHandle = DepthStencilView();

	commandList->OMSetRenderTargets(1, &rtvHandle,
		true, &dsvHandle);

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
