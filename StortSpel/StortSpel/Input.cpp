#include"3DPCH.h"
#include"Input.h"

Input::Input()
{
	m_cursorEnabled = false;
}

LRESULT Input::handleMessages(HWND hwnd, UINT& uMsg, WPARAM& wParam, LPARAM& lParam)
{
	switch (uMsg)
	{
	case WM_KEYDOWN:
	{
		bool wasPressed = lParam & 0x40000000;
		unsigned char key = static_cast<unsigned char>(wParam);
		if (!wasPressed)
		{
			m_Keyboard.onKeyPressed(key);
		}
		if (key == VK_ESCAPE)
			uMsg = WM_DESTROY;

		return 0;
	}
	case WM_KEYUP:
	{
		unsigned char key = static_cast<unsigned char>(wParam);
		m_Keyboard.onRelease(key);
		return 0;
	}
	case WM_LBUTTONDOWN:
	{
		if (!m_cursorEnabled)
		{
			while (ShowCursor(FALSE) >= 0);
			RECT windowRect;
			GetClientRect(hwnd, &windowRect);
			MapWindowPoints(hwnd, nullptr, reinterpret_cast<POINT*>(&windowRect), 2);
			ClipCursor(&windowRect);
		}

		MousePos mPos;
		mPos.x = LOWORD(lParam);
		mPos.y = HIWORD(lParam);

		m_Mouse.onLeftPress(mPos);
		return 0;
	}
	case WM_LBUTTONUP:
	{
		MousePos mPos;
		mPos.x = LOWORD(lParam);
		mPos.y = HIWORD(lParam);

		m_Mouse.onLeftRelease(mPos);
		return 0;
	}
	case WM_RBUTTONDOWN:
	{
		MousePos mPos;
		mPos.x = LOWORD(lParam);
		mPos.y = HIWORD(lParam);

		m_Mouse.onRightPress(mPos);
		return 0;
	}
	case WM_RBUTTONUP:
	{
		MousePos mPos;
		mPos.x = LOWORD(lParam);
		mPos.y = HIWORD(lParam);

		m_Mouse.onRightRelease(mPos);
		return 0;
	}
	case WM_MBUTTONDOWN:
	{
		MousePos mPos;
		mPos.x = LOWORD(lParam);
		mPos.y = HIWORD(lParam);

		m_Mouse.onMiddlePress(mPos);
		return 0;
	}
	case WM_MBUTTONUP:
	{
		MousePos mPos;
		mPos.x = LOWORD(lParam);
		mPos.y = HIWORD(lParam);

		m_Mouse.onMiddleRelease(mPos);
		return 0;
	}
	case WM_MOUSEMOVE:
	{
		MousePos mPos;
		mPos.x = LOWORD(lParam);
		mPos.y = HIWORD(lParam);

		m_Mouse.onMove(mPos);
		return 0;
	}
	case WM_MOUSEWHEEL:
	{
		MousePos mPos;
		mPos.x = LOWORD(lParam);
		mPos.y = HIWORD(lParam);

		if (GET_WHEEL_DELTA_WPARAM(wParam) > 0)
		{
			m_Mouse.onWheelUp(mPos);
		}
		else if (GET_WHEEL_DELTA_WPARAM(wParam) < 0)
		{
			m_Mouse.onWheelDown(mPos);
		}
		return 0;
	}
	case WM_INPUT:
	{
		UINT size;
		GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));
		if (size > 0)
		{
			std::unique_ptr<BYTE[]> data = std::make_unique<BYTE[]>(size);
			if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, data.get(), &size, sizeof(RAWINPUTHEADER)) == size)
			{
				RAWINPUT* rInput = reinterpret_cast<RAWINPUT*>(data.get());
				if (rInput->header.dwType == RIM_TYPEMOUSE)
				{
					MousePos mPos;
					mPos.x = rInput->data.mouse.lLastX;
					mPos.y = rInput->data.mouse.lLastY;

					m_Mouse.onRawMove(mPos);
				}
			}
		}

		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	case WM_ACTIVATE:
	{
		if (!m_cursorEnabled)
		{
			if (wParam & WM_ACTIVATE || wParam & WA_CLICKACTIVE)
			{
				while (ShowCursor(FALSE) >= 0);
				RECT windowRect;
				GetClientRect(hwnd, &windowRect);
				MapWindowPoints(hwnd, nullptr, reinterpret_cast<POINT*>(&windowRect), 2);
				ClipCursor(&windowRect);
			}
			else
			{
				while (ShowCursor(TRUE) < 0);
				ClipCursor(nullptr);
			}
		}
	}

	}

	return 0;
}

void Input::readBuffers()
{
	if (!m_Keyboard.empty())
	{
		KeyboardEvent readEvent = m_Keyboard.readKey();
	}

	if (!m_Mouse.empty())
	{
		MouseEvent mouseEvnt = m_Mouse.readEvent();
	}
}

MouseEvent Input::getMouseEvent()
{
	return m_Mouse.readEvent();
}

Keyboard* Input::getKeyboard()
{
	return &m_Keyboard;
}

Mouse* Input::getMouse()
{
	return &m_Mouse;
}