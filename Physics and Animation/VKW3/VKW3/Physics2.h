#ifndef	PHYSICS2_H
#define PHYSICS2_H

#include "btBulletDynamicsCommon.h"

class Physics2
{
	btDynamicsWorld* world;
	btDispatcher* dispatcher;
	btCollisionConfiguration* collisionConfig;
	btBroadphaseInterface* broadphase;
	btConstraintSolver* solver;

public:
	void Init();
	void Update();
	void ShutDown();

	static inline void DeleteBtRigidBody(btRigidBody* _btRigidBody)
	{
		delete _btRigidBody->getMotionState();
		delete _btRigidBody->getCollisionShape();
		delete _btRigidBody;

		_btRigidBody = nullptr;
	}

	btRigidBody* AddSphere(float _x, float _y, float _z, float _radius, float _mass);
	btRigidBody* AddCylinder(float _x, float _y, float _z, float _diameter, float _height, float _mass);
	btRigidBody* AddCone(float _x, float _y, float _z, float _diameter, float _height, float _mass);
	btRigidBody* AddCube(float _x, float _y, float _z, float _width, float _depth, float _height, float _mass);
	btRigidBody* AddCapsule(float _x, float _y, float _z, float _radius, float _height, float _mass);
};

#endif