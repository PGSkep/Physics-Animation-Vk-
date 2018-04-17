#ifndef	BEHAVIORS_H
#define BEHAVIORS_H

#include "Scene2.h"

namespace Behaviors
{
	void SetTransformToRigidbody(void* _data)
	{
		Scene2::Object* object = (Scene2::Object*)_data;

		btTransform transform;
		object->rigidBody->getMotionState()->getWorldTransform(transform);

		transform.getOpenGLMatrix(&object->transform[0][0]);
	}

	void UpdateAnimationTime(void* _data)
	{
		Scene2::Object* object = (Scene2::Object*)_data;

		object->animationTime += (float)(Engine2::deltaTime * Engine2::timeMultiplier * object->animationSpeedMultiplier);

		if (object->animationTime > object->animationEnd)
			object->animationTime -= object->animationEnd - object->animationStart;
	}
};

#endif