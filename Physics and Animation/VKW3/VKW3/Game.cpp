#include "Game.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <iostream>

#include "AnimationController.h"

void Game::Update(Scene& _scene, Input& _input)
{
	AnimationController::ControlSkeleton(_scene.animationModifiers[0],
		_input.mousePosition[0] / 1980.0f, _input.mousePosition[1] / 1090.0f,
		5, 6, 7);
}
