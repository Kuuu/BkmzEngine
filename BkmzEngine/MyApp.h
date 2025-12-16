#pragma once
#include "dxApp.h"

class MyApp : public dxApp
{
public:
	MyApp(HWND hwnd, UINT width, UINT height) : dxApp(hwnd, width, height) {}
	
	void Update() override;
	void Draw() override;
	void Initialize() override;

private:
	void CreateInputLayout();

private:
	static constexpr int vertexCount = 3;
	static constexpr int indexCount = 3;
	D3D12_VERTEX_BUFFER_VIEW vbv;
	D3D12_INDEX_BUFFER_VIEW ibv;
};