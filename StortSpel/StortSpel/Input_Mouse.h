#pragma once
#include "InputStructs.h"

//enum class Event
//{
//	Invalid,
//	Pressed,
//	Released,
//	MouseLPressed,
//	MouseLReleased,
//	MouseRPressed,
//	MouseRReleased,
//	MouseMPressed,
//	MouseMReleased,
//	MouseWheelUp,
//	MouseWheelDown,
//	MouseMove,
//	MouseRAW_MOVE
//};

class MouseEvent
{
private:
	Event eventType;
	MousePos mousePos;
public:
	MouseEvent();
	MouseEvent(const Event evnt, const float x, const float y);
	float getPosX() const;
	float getPosY() const;
	Event getEvent() const;
	MousePos getPos() const;
	bool isValid() const;
};

class Mouse
{

private:
	bool m_LDown;
	bool m_RDown;
	bool m_MDown;
	MousePos m_mousePos;
	std::queue<MouseEvent> m_events;

public:
	Mouse();
	~Mouse();
	MouseEvent readEvent();
	bool empty() const;

	void onMove(const MousePos pos);
	void onRawMove(const MousePos pos);
	void onLeftPress(const MousePos pos);
	void onRightPress(const MousePos pos);
	void onMiddlePress(const MousePos pos);
	void onLeftRelease(const MousePos pos);
	void onRightRelease(const MousePos pos);
	void onMiddleRelease(const MousePos pos);
	void onWheelUp(const MousePos pos);
	void onWheelDown(const MousePos pos);

	bool isLDown() const;
	bool isRDown() const;
	bool isMDown() const;
	float getPosX() const;
	float getPosY() const;
	MousePos getPos() const;
};
