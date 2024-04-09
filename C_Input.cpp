#include "C_Input.h"

void C_Input::handleEvent(const SDL_Event& ev)
{
	switch (ev.type)
	{
	case SDL_KEYDOWN:
		buttonHoldStatus[ev.key.keysym.scancode] = true;
		break;
	case SDL_KEYUP:
		buttonHoldStatus[ev.key.keysym.scancode] = false;
		break;
	case SDL_MOUSEBUTTONDOWN:
		mouseButtonHoldStatus[ev.button.button] = true;
		break;	
	case SDL_MOUSEBUTTONUP:
		mouseButtonHoldStatus[ev.button.button] = false;
		break;
	default:
		break;
	}
}

bool C_Input::isButtonHeld(SDL_Scancode k)
{
	if (buttonHoldStatus.find(k) == buttonHoldStatus.end()) return false;
	return buttonHoldStatus[k];
}

bool C_Input::isMouseButtonHeld(int button)
{
	if (mouseButtonHoldStatus.find(button) == mouseButtonHoldStatus.end()) return false;
	return mouseButtonHoldStatus[button];
}
