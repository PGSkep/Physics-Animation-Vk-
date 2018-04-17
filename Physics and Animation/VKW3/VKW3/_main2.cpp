#include "Engine2.h"

#include<Windows.h>

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetBreakAlloc(-1);

	Engine2 engine;
	engine.Init();
	engine.Load();
	engine.Setup();

	while (!GetAsyncKeyState(VK_ESCAPE))
	{
		engine.Loop();
	}

	engine.ShutDown();
}