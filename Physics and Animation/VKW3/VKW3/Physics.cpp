#include "Physics.h"

#include <iostream>

static bool CollisionProcessedCallback(btManifoldPoint& cp, void* _body0, void* _body1)
{
	if (_body0 == nullptr || _body1 == nullptr)
		return false;
	
	btCollisionObject* body0 = (btCollisionObject*)_body0;
	btCollisionObject* body1 = (btCollisionObject*)_body1;

	if (body0->getUserPointer() == nullptr || body1->getUserPointer() == nullptr)
		return false;

	std::cout << "Collision: " << ((Physics::Rigidbody*)body0->getUserPointer())->objectIndex << " - " << ((Physics::Rigidbody*)body1->getUserPointer())->objectIndex << "\n";
	return false;
}

static btRigidBody* planeBody;

void Physics::Init()
{
	collisionConfig = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfig);
	broadphase = new btDbvtBroadphase();
	solver = new btSequentialImpulseConstraintSolver();
	world = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfig);
	world->setGravity(btVector3(0, 0, -10));

	gContactProcessedCallback = CollisionProcessedCallback;
}
void Physics::Setup()
{
	// creation of the ground plane at 0,0,0
	{
		btTransform transform;
		transform.setIdentity();
		transform.setOrigin(btVector3(0, 0, 0));

		btStaticPlaneShape* planeShape = new btStaticPlaneShape(btVector3(0.0f, 0.0f, 1.0f), 0.0f);
		btDefaultMotionState* planeMotion = new btDefaultMotionState(transform);
		btRigidBody::btRigidBodyConstructionInfo info(0.0f, planeMotion, planeShape);
		planeBody = new btRigidBody(info);

		planeBody->setCollisionFlags(planeBody->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

		world->addRigidBody(planeBody);
	}
}
void Physics::Update(float _deltaTime, Scene& _scene)
{
	world->stepSimulation(_deltaTime);

	btTransform transform;

	Node* currentNode = rigidbodyList;
	while (currentNode != nullptr)
	{
		currentNode->data.rigidbody->getMotionState()->getWorldTransform(transform);
		transform.getOpenGLMatrix(_scene.rootObjects[currentNode->data.objectIndex].matrix.mat);

		currentNode = currentNode->next;
	}

	//for (size_t i = 0; i != rigidbodies.size(); ++i)
	//{
	//	rigidbodies[i].rigidbody->getMotionState()->getWorldTransform(transform);
	//	transform.getOpenGLMatrix(_objManager.objects[rigidbodies[i].objectIndex].matrix.mat);
	//}
}
void Physics::ShutDown()
{
	delete world;
	delete solver;
	delete broadphase;
	delete dispatcher;
	delete collisionConfig;

	Node* currentNode = rigidbodyList;
	Node* tempNode;
	while (currentNode != nullptr)
	{
		tempNode = currentNode;
		currentNode = currentNode->next;
		delete tempNode;
	}
}

void Physics::AddSphere(float _x, float _y, float _z, float _radius, float _mass, size_t _transformIndex)
{
	Rigidbody rigidBody;

	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btVector3(_x, _y, _z));

	btSphereShape* shape = new btSphereShape(_radius);

	btDefaultMotionState* motion = new btDefaultMotionState(transform);

	btVector3 inertia(0.0f, 0.0f, 0.0f);
	if (_mass != 0.0f)
		shape->calculateLocalInertia(_mass, inertia);

	btRigidBody::btRigidBodyConstructionInfo info(_mass, motion, shape, inertia);
	rigidBody.rigidbody = new btRigidBody(info);
	rigidBody.objectIndex = _transformIndex;

	Node* newNode = new Node;
	newNode->data = rigidBody;
	newNode->next = rigidbodyList;
	rigidbodyList = newNode;

	world->addRigidBody(rigidBody.rigidbody);

	rigidBody.rigidbody->setCollisionFlags(rigidBody.rigidbody->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
	rigidBody.rigidbody->setUserPointer(&newNode->data);// So collision callback has a pointer to the anything that the rigidbody is connected to.
}
void Physics::AddCylinder(float _x, float _y, float _z, float _diameter, float _height, float _mass, size_t _transformIndex)
{
	Rigidbody rigidBody;

	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btVector3(_x, _y, _z));

	btCylinderShape* shape = new btCylinderShape(btVector3(_diameter*0.5f, _diameter*0.5f, _height*0.5f));

	btDefaultMotionState* motion = new btDefaultMotionState(transform);

	btVector3 inertia(0.0f, 0.0f, 0.0f);
	if (_mass != 0.0f)
		shape->calculateLocalInertia(_mass, inertia);

	btRigidBody::btRigidBodyConstructionInfo info(_mass, motion, shape, inertia);
	rigidBody.rigidbody = new btRigidBody(info);
	rigidBody.objectIndex = _transformIndex;

	Node* newNode = new Node;
	newNode->data = rigidBody;
	newNode->next = rigidbodyList;
	rigidbodyList = newNode;

	world->addRigidBody(rigidBody.rigidbody);

	rigidBody.rigidbody->setCollisionFlags(rigidBody.rigidbody->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
	rigidBody.rigidbody->setUserPointer(&newNode->data);// So collision callback has a pointer to the anything that the rigidbody is connected to.
}
void Physics::AddCone(float _x, float _y, float _z, float _diameter, float _height, float _mass, size_t _transformIndex)
{
	Rigidbody rigidBody;

	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btVector3(_x, _y, _z));

	btConeShape* shape = new btConeShape(_diameter, _height);

	btDefaultMotionState* motion = new btDefaultMotionState(transform);

	btVector3 inertia(0.0f, 0.0f, 0.0f);
	if (_mass != 0.0f)
		shape->calculateLocalInertia(_mass, inertia);

	btRigidBody::btRigidBodyConstructionInfo info(_mass, motion, shape, inertia);
	rigidBody.rigidbody = new btRigidBody(info);
	rigidBody.objectIndex = _transformIndex;

	Node* newNode = new Node;
	newNode->data = rigidBody;
	newNode->next = rigidbodyList;
	rigidbodyList = newNode;

	world->addRigidBody(rigidBody.rigidbody);

	rigidBody.rigidbody->setCollisionFlags(rigidBody.rigidbody->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
	rigidBody.rigidbody->setUserPointer(&newNode->data);// So collision callback has a pointer to the anything that the rigidbody is connected to.
}
void Physics::AddCube(float _x, float _y, float _z, float _width, float _depth, float _height, float _mass, size_t _transformIndex)
{
	Rigidbody rigidBody;

	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btVector3(_x, _y, _z));

	btBoxShape* shape = new btBoxShape(btVector3(_width, _depth, _height));

	btDefaultMotionState* motion = new btDefaultMotionState(transform);

	btVector3 inertia(0.0f, 0.0f, 0.0f);
	if (_mass != 0.0f)
		shape->calculateLocalInertia(_mass, inertia);

	btRigidBody::btRigidBodyConstructionInfo info(_mass, motion, shape, inertia);
	rigidBody.rigidbody = new btRigidBody(info);
	rigidBody.objectIndex = _transformIndex;

	Node* newNode = new Node;
	newNode->data = rigidBody;
	newNode->next = rigidbodyList;
	rigidbodyList = newNode;

	world->addRigidBody(rigidBody.rigidbody);

	rigidBody.rigidbody->setCollisionFlags(rigidBody.rigidbody->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
	rigidBody.rigidbody->setUserPointer(&newNode->data);// So collision callback has a pointer to the anything that the rigidbody is connected to.
}
void Physics::AddCapsule(float _x, float _y, float _z, float _radius, float _height, float _mass, size_t _transformIndex)
{
	Rigidbody rigidBody;

	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btVector3(_x, _y, _z));

	btCapsuleShape* shape = new btCapsuleShape(_radius, _height);

	btDefaultMotionState* motion = new btDefaultMotionState(transform);

	btVector3 inertia(0.0f, 0.0f, 0.0f);
	if (_mass != 0.0f)
		shape->calculateLocalInertia(_mass, inertia);

	btRigidBody::btRigidBodyConstructionInfo info(_mass, motion, shape, inertia);
	rigidBody.rigidbody = new btRigidBody(info);
	rigidBody.objectIndex = _transformIndex;

	Node* newNode = new Node;
	newNode->data = rigidBody;
	newNode->next = rigidbodyList;
	rigidbodyList = newNode;

	world->addRigidBody(rigidBody.rigidbody);

	rigidBody.rigidbody->setCollisionFlags(rigidBody.rigidbody->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK | btCollisionObject::CF_NO_CONTACT_RESPONSE);
	rigidBody.rigidbody->setUserPointer(&newNode->data);// So collision callback has a pointer to the anything that the rigidbody is connected to.
}
