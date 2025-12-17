#pragma once
#include "dxApp.h"
#include "Mesh.h"
#include "DefaultMaterial.h"
#include "GameObject.h"
#include <vector>

class MyApp : public dxApp
{
public:
	MyApp(HWND hwnd, UINT width, UINT height) : dxApp(hwnd, width, height) {}
	
	void Update(float deltaTime) override;

private:
	void CustomDraw();
	void DrawWithMaterial(const std::vector<GameObject> &objects, Material *material);

public:
	void Initialize() override;

private:
	void CreateMaterials();
	void CreateObjects();

private:
	static constexpr int vertexCount = 8;
	static constexpr int indexCount = 6*6;
	float rotationY = 0.0f;

	DefaultMaterial defaultMaterial;
	std::vector<GameObject> gameObjects;
};