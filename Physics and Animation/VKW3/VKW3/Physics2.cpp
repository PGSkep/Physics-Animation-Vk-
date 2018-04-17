#include "Physics2.h"

#include "Engine2.h"

static bool CollisionProcessedCallback(btManifoldPoint& cp, void* _body0, void* _body1)
{
	if (_body0 == nullptr || _body1 == nullptr)
		return false;

	btCollisionObject* body0 = (btCollisionObject*)_body0;
	btCollisionObject* body1 = (btCollisionObject*)_body1;

	if (body0->getUserPointer() == nullptr || body1->getUserPointer() == nullptr)
		return false;

	//std::cout << "Collision: " << ((Physics2::Rigidbody*)body0->getUserPointer())->objectIndex << " - " << ((Physics2::Rigidbody*)body1->getUserPointer())->objectIndex << "\n";
	return false;
}

/// TEST
static btRigidBody* planeBody;

void Physics2::Init()
{
	collisionConfig = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfig);
	broadphase = new btDbvtBroadphase();
	solver = new btSequentialImpulseConstraintSolver();
	world = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfig);
	world->setGravity(btVector3(0, 0, -10));

	gContactProcessedCallback = CollisionProcessedCallback;

	/// TEST
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

void Physics2::Update()
{
	world->stepSimulation((float)Engine2::deltaTime);
}

void Physics2::ShutDown()
{
	delete world;
	delete solver;
	delete broadphase;
	delete dispatcher;
	delete collisionConfig;

	DeleteBtRigidBody(planeBody);
}

btRigidBody* Physics2::AddSphere(float _x, float _y, float _z, float _radius, float _mass)
{
	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btVector3(_x, _y, _z));

	btSphereShape* shape = new btSphereShape(_radius);
	btDefaultMotionState* motion = new btDefaultMotionState(transform);
	btVector3 inertia(0.0f, 0.0f, 0.0f);
	if (_mass != 0.0f)
		shape->calculateLocalInertia(_mass, inertia);

	btRigidBody::btRigidBodyConstructionInfo info(_mass, motion, shape, inertia);
	btRigidBody* rigidBody = new btRigidBody(info);
	world->addRigidBody(rigidBody);

	rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

	return rigidBody;
}

btRigidBody * Physics2::AddCylinder(float _x, float _y, float _z, float _diameter, float _height, float _mass)
{
	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btVector3(_x, _y, _z));

	btCylinderShape* shape = new btCylinderShape(btVector3(_diameter*0.5f, _diameter*0.5f, _height*0.5f));
	btDefaultMotionState* motion = new btDefaultMotionState(transform);
	btVector3 inertia(0.0f, 0.0f, 0.0f);
	if (_mass != 0.0f)
		shape->calculateLocalInertia(_mass, inertia);

	btRigidBody::btRigidBodyConstructionInfo info(_mass, motion, shape, inertia);
	btRigidBody* rigidBody = new btRigidBody(info);
	world->addRigidBody(rigidBody);

	rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

	return rigidBody;
}

btRigidBody * Physics2::AddCone(float _x, float _y, float _z, float _diameter, float _height, float _mass)
{
	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btVector3(_x, _y, _z));

	btConeShape* shape = new btConeShape(_diameter, _height);
	btDefaultMotionState* motion = new btDefaultMotionState(transform);
	btVector3 inertia(0.0f, 0.0f, 0.0f);
	if (_mass != 0.0f)
		shape->calculateLocalInertia(_mass, inertia);

	btRigidBody::btRigidBodyConstructionInfo info(_mass, motion, shape, inertia);
	btRigidBody* rigidBody = new btRigidBody(info);
	world->addRigidBody(rigidBody);

	rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

	return rigidBody;
}

btRigidBody * Physics2::AddCube(float _x, float _y, float _z, float _width, float _depth, float _height, float _mass)
{
	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btVector3(_x, _y, _z));

	btBoxShape* shape = new btBoxShape(btVector3(_width, _depth, _height));
	btDefaultMotionState* motion = new btDefaultMotionState(transform);
	btVector3 inertia(0.0f, 0.0f, 0.0f);
	if (_mass != 0.0f)
		shape->calculateLocalInertia(_mass, inertia);

	btRigidBody::btRigidBodyConstructionInfo info(_mass, motion, shape, inertia);
	btRigidBody* rigidBody = new btRigidBody(info);
	world->addRigidBody(rigidBody);

	rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

	return rigidBody;
}

btRigidBody * Physics2::AddCapsule(float _x, float _y, float _z, float _radius, float _height, float _mass)
{
	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btVector3(_x, _y, _z));

	btCapsuleShape* shape = new btCapsuleShape(_radius, _height);
	btDefaultMotionState* motion = new btDefaultMotionState(transform);
	btVector3 inertia(0.0f, 0.0f, 0.0f);
	if (_mass != 0.0f)
		shape->calculateLocalInertia(_mass, inertia);

	btRigidBody::btRigidBodyConstructionInfo info(_mass, motion, shape, inertia);
	btRigidBody* rigidBody = new btRigidBody(info);
	world->addRigidBody(rigidBody);

	rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

	return rigidBody;
}
