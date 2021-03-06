#include"3DPCH.h"
#include "Input_Keyboard.h"


KeyboardEvent::KeyboardEvent()
{
	this->eventType = Event::Invalid;
	this->key = 0u;
}

KeyboardEvent::KeyboardEvent(const Event type, const unsigned char key)
{
	this->eventType = type;
	this->key = key;
}

bool KeyboardEvent::isPressed() const
{
	return this->eventType == Event::Pressed;
}
bool KeyboardEvent::isReleased() const
{
	return this->eventType == Event::Released;
}
bool KeyboardEvent::isValid() const
{
	return this->eventType != Event::Invalid;
}
Event KeyboardEvent::getEvent() const
{
	return this->eventType;
}
unsigned char KeyboardEvent::getKey() const
{
	return this->key;
}


Keyboard::Keyboard()
{
	for (int i = 0; i < 256; i++)
	{
		this->keyStatus[i] = false;
	}
}

void Keyboard::onKeyPressed(const unsigned char key)
{
	this->keyStatus[key] = true;
	this->keyBuffer.push(KeyboardEvent(Event::Pressed, key));
}
void Keyboard::onRelease(const unsigned char key)
{
	this->keyStatus[key] = false;
	this->keyBuffer.push(KeyboardEvent(Event::Released, key));
}
bool Keyboard::isKeyPressed(const unsigned char key) const
{
	return this->keyStatus[key];
}
bool Keyboard::empty() const
{
	return this->keyBuffer.empty();
}
KeyboardEvent Keyboard::readKey()
{
	if (this->keyBuffer.empty())
	{
		return KeyboardEvent();
	}
	else
	{
		KeyboardEvent inputEvnt = this->keyBuffer.front();
		this->keyBuffer.pop();
		return inputEvnt;
	}
}