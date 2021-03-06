#pragma once

#ifndef UNICODE
#define UNICODE
#endif

// Standard
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <wrl/client.h>
#define NOMINMAX
#include <map>
#include <unordered_map>
#include <vector>
#include <queue>
#include <string>
#include <sstream>
#include <ctime>
#include <memory>
#include <math.h>
#include <fstream>
#include<stack>
#include<iostream>
#include <chrono>

// DirectX 11
#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <Effects.h>
#include <DirectXColors.h>

//PhysX
#include"PxPhysicsAPI.h"

// Singletons
#include "ResourceHandler.h"

#include"ErrorLogger.h"


// Components
#include "Component.h"
#include "TestComponent.h"
#include "InvalidComponent.h"

//Texture loading
#include <WICTextureLoader.h>
#include<DDSTextureLoader.h>
#include"DirectXTex.h"
using namespace DirectX;

// DirectX 11 Linking
#pragma comment (lib, "user32")
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "gdi32")
#pragma comment (lib, "d3dcompiler.lib")

//Defines
#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }
#define SAFE_DELETE(a) if( (a) != NULL ) delete (a); (a) = NULL;

enum class Event
{
	Invalid,
	Pressed,
	Released,
	MouseLPressed,
	MouseLReleased,
	MouseRPressed,
	MouseRReleased,
	MouseMPressed,
	MouseMReleased,
	MouseWheelUp,
	MouseWheelDown,
	MouseMove,
	MouseRAW_MOVE
};


