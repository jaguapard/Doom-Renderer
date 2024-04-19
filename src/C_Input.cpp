#include "C_Input.h"
#include <iostream>

C_Input::C_Input()
{
	charToScancodeMap['0'] = SDL_SCANCODE_0;
	for (char c = '1'; c <= '9'; ++c)
	{
		charToScancodeMap[c] = SDL_Scancode(SDL_SCANCODE_1 + c - '1');
	}	
	for (char c = 'A'; c <= 'Z'; ++c)
	{
		charToScancodeMap[c] = SDL_Scancode(SDL_SCANCODE_A + c - 'A');
	}

	for (const auto& it : charToScancodeMap) scancodeToCharMap[it.second] = it.first;
}

void C_Input::beginNewFrame()
{
	for (auto& it : buttonPressStatus) if (it.second) lockedKeys.insert(it.first); //if some buttons were pressed on previous frame, remember them to filter keydown events properly
	buttonPressStatus.clear(); //forget all key presses. This must be done to ensure that each key press results in only one frame where button press is registered
}

void C_Input::handleEvent(const SDL_Event& ev)
{
	switch (ev.type)
	{
	case SDL_KEYDOWN:
	{
		SDL_Scancode scancode = ev.key.keysym.scancode;
		buttonHoldStatus[scancode] = true;
		if (lockedKeys.find(scancode) == lockedKeys.end()) buttonPressStatus[scancode] = true;
		break;
	}		
	case SDL_KEYUP:
	{
		SDL_Scancode scancode = ev.key.keysym.scancode;
		buttonHoldStatus[scancode] = false;
		lockedKeys.erase(scancode);
		break;
	}
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
	return buttonHoldStatus[k]; //if k does not exists in the map, it will be created as false, so this works
}

bool C_Input::wasButtonPressedOnThisFrame(SDL_Scancode k)
{
	return buttonPressStatus[k];
}

bool C_Input::wasCharPressedOnThisFrame(char c)
{
	SDL_Scancode s = charToScancodeMap[c];
	return buttonPressStatus[s];
}

bool C_Input::isMouseButtonHeld(int button)
{
	return mouseButtonHoldStatus[button];
}
