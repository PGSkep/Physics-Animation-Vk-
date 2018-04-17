#ifndef PHYSICS_H
#define PHYSICS_H

#include "btBulletDynamicsCommon.h"

#include <vector>

#include "Scene.h"

class Physics
{
	btDynamicsWorld* world;
	btDispatcher* dispatcher;
	btCollisionConfiguration* collisionConfig;
	btBroadphaseInterface* broadphase;
	btConstraintSolver* solver;

public:
	struct Rigidbody
	{
		btRigidBody* rigidbody;
		size_t objectIndex;
	};
	struct Node
	{
		Node* next;
		Rigidbody data;
	};
	//std::vector<Rigidbody> rigidbodies;
	Node* rigidbodyList = nullptr;

	void Init();
	void Setup();
	void Update(float _deltaTime, Scene& _objManager);
	void ShutDown();

	void AddSphere(float _x, float _y, float _z, float _radius, float _mass, size_t _transformIndex);
	void AddCylinder(float _x, float _y, float _z, float _diameter, float _height, float _mass, size_t _transformIndex);
	void AddCone(float _x, float _y, float _z, float _diameter, float _height, float _mass, size_t _transformIndex);
	void AddCube(float _x, float _y, float _z, float _width, float _depth, float _height, float _mass, size_t _transformIndex);
	void AddCapsule(float _x, float _y, float _z, float _radius, float _height, float _mass, size_t _transformIndex);
};

#endif