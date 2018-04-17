#ifndef	ENGINE_H
#define ENGINE_H

#include "Renderer.h"
#include "Physics.h"
#include "Scene.h"
#include "Game.h"
#include "Input.h"

class Engine
{
	Timer timer;
	double timeMultiplier;
	double startTime;
	double deltaTime;

	Renderer renderer;
	Physics physics;
	Scene scene;
	Game game;

public:
	Input input;

	void Init();
	void Loop();
	void ShutDown();
};

#endif