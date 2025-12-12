#pragma once
#include "dxApp.h"

class MyApp : public dxApp
{
public:
	MyApp(HWND hwnd, UINT width, UINT height) : dxApp(hwnd, width, height) {}
	
	void Draw() override;
};