#pragma once
#include "Transform.h"
#include "Mesh.h"
#include <memory>
#include "DefaultMaterial.h"

class GameObject
{
public:
	Transform transform;
	std::unique_ptr<Mesh<DefaultMaterial::Vertex>> mesh;
};