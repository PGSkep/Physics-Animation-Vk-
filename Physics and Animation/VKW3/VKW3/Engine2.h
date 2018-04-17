#ifndef	ENGINE2_H
#define ENGINE2_H

#include "Timer.h"

#include "Renderer2.h"
#include "Scene2.h"
#include "Game2.h"
#include "Physics2.h"

class Engine2
{
	Timer timer;

	Renderer2 renderer;
	Scene2 scene;
	Game2 game;
	Physics2 physics;

	double startTime;

public:
	static double deltaTime;
	static double timeMultiplier;

	void Init();
	void Load();
	void Setup();
	void Loop();
	void ShutDown();
};

#endif