#pragma once
#include <SDL/SDL.h>
#include <unordered_map>

class C_Input
{
public:
	C_Input() = default;
	void handleEvent(const SDL_Event& ev);
	bool isButtonHeld(SDL_Scancode k);
	bool isMouseButtonHeld(int button);
private:
	std::unordered_map<SDL_Scancode, bool> buttonHoldStatus;
	std::unordered_map<int, bool> mouseButtonHoldStatus;
};