#pragma once

#include "GUIElement.h"
#include "GUIText.h"

class GUIHandler
{
private:
	GUIHandler();

	// Device
	ID3D11Device* m_device;
	ID3D11DeviceContext* m_dContext;

	// Render States
	std::unique_ptr< CommonStates > m_states;

	// Screen Dimensions
	int m_screenWidth;
	int m_screenHeight;

	// SpriteBatch
	SpriteBatch* m_spriteBatch;

	// GUI Elements
	std::vector< GUIElement* > m_elements;
	std::unordered_map< std::wstring, SpriteFont* > m_fonts;
	const std::wstring m_FONTS_PATH = L"../res/fonts/";

public:
	static GUIHandler& get();
	~GUIHandler();

	void initialize(ID3D11Device* device, ID3D11DeviceContext* dContext);

	int addGUIText(std::string textString, std::wstring fontName, GUITextStyle style = GUITextStyle());
	void changeGUIText(int index, std::string newTextString);
	
	void removeElement(int index);

	void render();
};