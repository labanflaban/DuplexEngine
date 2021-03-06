#pragma once
#include"Input.h"
#include"GraphicEngine.h"
#include"Physics.h"

class ApplicationLayer
{
private:
	ApplicationLayer();
	GraphicEngine* m_graphicEnginePtr;
	Physics m_physics;
	HWND m_window;


	float m_time;

	int width, height;

	void createWin32Window(const HINSTANCE hInstance, const wchar_t* windowTitle, HWND& _d3d11Window);
public:
	static ApplicationLayer& getInstance()
	{
		static ApplicationLayer instance;
		return instance;
	}
	ApplicationLayer(ApplicationLayer const&) = delete;
	void operator=(ApplicationLayer const&) = delete;
	~ApplicationLayer();
	bool initializeApplication(const HINSTANCE& hInstance, const LPWSTR& lpCmdLine, HWND hWnd, const int& showCmd);
	void applicationLoop();

	GraphicEngine* getGraphicsEngine() { return m_graphicEnginePtr; }

	const HWND& getWindow() { return m_window; }
	Input m_input;

};