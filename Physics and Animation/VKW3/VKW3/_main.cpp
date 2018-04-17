#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include <Windows.h>

#include "Engine.h"

#include "Input.h"

int main2()
{
	Engine engine;

	engine.Init();

	engine.input.Update();
	while (!engine.input.CheckKeyDown(Input::INPUT_KEYS::KEY_ESC))
	{
		engine.Loop();
	}

	engine.ShutDown();

	return 0;
}
