#pragma once
#include <SDL/SDL.h>
#include <unordered_map>
#include <unordered_set>

class C_Input
{
public:
	C_Input();
	void beginNewFrame();

	void handleEvent(const SDL_Event& ev); //Take an event and process it. This should be called on every frame. Any events not pretaining to input are ignored, so it's safe to pass any events here.

	bool isButtonHeld(SDL_Scancode k);
	bool wasButtonPressedOnThisFrame(SDL_Scancode k); //usable for one-time detections. Will only return true on frame the button was pressed on, holding it will not return true repeatedly.
	bool wasCharPressedOnThisFrame(char c);

	bool isMouseButtonHeld(int button);
private:
	std::unordered_map<char, SDL_Scancode> charToScancodeMap;
	std::unordered_map<SDL_Scancode, char> scancodeToCharMap;

	std::unordered_set<SDL_Scancode> lockedKeys;

	std::unordered_map<SDL_Scancode, bool> buttonHoldStatus;
	std::unordered_map<SDL_Scancode, bool> buttonPressStatus;
	std::unordered_map<int, bool> mouseButtonHoldStatus;
};