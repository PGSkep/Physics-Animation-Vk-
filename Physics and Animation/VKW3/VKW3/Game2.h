#ifndef	GAME2_H
#define GAME2_H

#include "Scene2.h"

#define PI 3.14159265358979323846264338327950288f

class Game2
{
public:
	void UpdateModifiers(Scene2& _scene);
	void UpdatePhysics(Scene2& _scene);

	void Update(Scene2& _scene);
};

#endif