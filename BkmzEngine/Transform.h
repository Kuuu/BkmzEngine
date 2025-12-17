#pragma once
#include "d3d12.h"
#include <DirectXMath.h>

class Transform
{
public:
	DirectX::XMVECTOR position = { 0 };
	DirectX::XMVECTOR rotation = { 0 };
	DirectX::XMVECTOR scale = { 0 };
};