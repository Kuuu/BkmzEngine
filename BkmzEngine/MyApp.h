#pragma once
#include "dxApp.h"
#include "Cube.h"
#include "Material.h"
#include <vector>

class MyApp : public dxApp
{
public:
	MyApp(HWND hwnd, UINT width, UINT height) : dxApp(hwnd, width, height) {}
	
	void Update(float deltaTime) override;
	void Draw() override;
	void Initialize() override;

private:
	void CreateInputLayout();

private:
	static constexpr int vertexCount = 8;
	static constexpr int indexCount = 6*6;
	float rotationY = 0.0f;

	Material defaultMaterial;
	std::vector<Cube> cubes;
};