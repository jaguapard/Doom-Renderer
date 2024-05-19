#include "GameStateBase.h"

GameStateSwitch GameStateBase::run()
{
    try
    {
        while (true)
        {
            this->beginNewFrame();
            this->handleInput();
            this->update();

            this->draw();
            this->endFrame();
        }
    }
    //yes, using exceptions for flow contol is very bad. 
    //But it is very nice that a switch can happen everywhere without needing any flags and checking them everywhere. 
    //Outside users don't know about this exception anyway, and other exceptions are not caught here, they will
    //propagate further down the call stack
    catch (const GameStateSwitch& sw) 
    {
        return sw;
    }
}
