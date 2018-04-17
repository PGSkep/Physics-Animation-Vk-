#ifndef	GAME_H
#define GAME_H

#include <array>
#include <vector>

#include "Scene.h"
#include "Input.h"

class Game
{
public:
	void Update(Scene& _scene, Input& _input);
};

#endif