#include "Game2.h"

#include <Windows.h> /// REMOVE ME

void ControlSkeleton(Scene2::AnimationModifier& _animationModifier, float _mouseXForce, float _mouseYForce, size_t shoulderRIndex, size_t armRIndex, size_t foreArmRIndex)
{
	glm::quat quat;

	float shoulderX = (0.2f + -_mouseXForce * 0.2f) * PI;
	float shoulderY = (-1.5f + _mouseYForce * 0.1f) * PI;
	float shoulderZ = _mouseYForce * 0.1f * PI;

	quat = glm::rotate(quat, shoulderX, glm::vec3(1.0f, 0.0f, 0.0f));
	quat = glm::rotate(quat, shoulderY, glm::vec3(0.0f, 1.0f, 0.0f));
	quat = glm::rotate(quat, shoulderZ, glm::vec3(0.0f, 0.0f, 1.0f));

	_animationModifier.modifiers[shoulderRIndex].rotX = quat.x;
	_animationModifier.modifiers[shoulderRIndex].rotY = quat.y;
	_animationModifier.modifiers[shoulderRIndex].rotZ = quat.z;
	_animationModifier.modifiers[shoulderRIndex].rotW = quat.w;

	quat = glm::quat();

	float armX = (0.1f + _mouseXForce * 0.2f) * PI;
	float armY = (-0.1f + _mouseYForce * 0.5f) * PI;
	float armZ = (0.5f + -_mouseXForce * 0.5f) * PI;

	quat = glm::rotate(quat, armZ, glm::vec3(0.0f, 0.0f, 1.0f));
	quat = glm::rotate(quat, armY, glm::vec3(0.0f, 1.0f, 0.0f));
	quat = glm::rotate(quat, armX, glm::vec3(1.0f, 0.0f, 0.0f));

	_animationModifier.modifiers[armRIndex].rotX = quat.x;
	_animationModifier.modifiers[armRIndex].rotY = quat.y;
	_animationModifier.modifiers[armRIndex].rotZ = quat.z;
	_animationModifier.modifiers[armRIndex].rotW = quat.w;

	quat = glm::quat();

	float foreArmX = (0.0f + _mouseXForce * 0.0f) * PI;
	float foreArmY = (0.0f + _mouseYForce * 0.0f) * PI;
	float foreArmZ = (0.6f + -_mouseXForce * 0.4f) * PI;

	quat = glm::rotate(quat, foreArmZ, glm::vec3(0.0f, 0.0f, 1.0f));
	quat = glm::rotate(quat, foreArmY, glm::vec3(0.0f, 1.0f, 0.0f));
	quat = glm::rotate(quat, foreArmX, glm::vec3(1.0f, 0.0f, 0.0f));

	_animationModifier.modifiers[foreArmRIndex].rotX = quat.x;
	_animationModifier.modifiers[foreArmRIndex].rotY = quat.y;
	_animationModifier.modifiers[foreArmRIndex].rotZ = quat.z;
	_animationModifier.modifiers[foreArmRIndex].rotW = quat.w;
}


void Game2::UpdateModifiers(Scene2& _scene)
{
	POINT mousePos;
	GetCursorPos(&mousePos);

	ControlSkeleton(_scene.animationModifiers[0],
		mousePos.x / 1980.0f, mousePos.y / 1090.0f,
		5, 6, 7);
}

void Game2::UpdatePhysics(Scene2& _scene)
{
}

void Game2::Update(Scene2& _scene)
{
	UpdateModifiers(_scene);
	UpdatePhysics(_scene);

	for (size_t i = 0; i != _scene.objects.size(); ++i)
	{
		for (size_t j = 0; j != _scene.objects[i]->behaviors.size(); ++j)
		{
			_scene.objects[i]->behaviors[j](_scene.objects[i]);
		}
	}
}
