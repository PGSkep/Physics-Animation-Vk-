#ifndef	SCENE2_H
#define SCENE2_H

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <array>
#include <vector>

#include "Physics2.h"

class Scene2
{
public:
	struct Skeleton
	{
		struct Joint
		{
			std::string name;
			size_t parentIndex;

			struct Channel3D
			{
				float time;
				aiVector3D channel;
			};
			struct ChannelQuaternion
			{
				float time;
				aiQuaternion channel;
			};
			std::vector<Channel3D> translateChannels;
			std::vector<ChannelQuaternion> rotateChannels;
			std::vector<Channel3D> scaleChannels;

			aiMatrix4x4 offset;
			aiMatrix4x4 transformation;

			size_t boneID = -1;
		};
		std::vector<Joint> joints;
		aiMatrix4x4 globalInverseTransform;
	};
	struct AnimationModifier
	{
		struct JointModifier
		{
			float weight;
			float rotX, rotY, rotZ, rotW;

			static inline JointModifier GetJointModifier(float _weight, float _rotX, float _rotY, float _rotZ, float _rotW)
			{
				return{ _weight, _rotX, _rotY, _rotZ, _rotW };
			};
		};

		std::array<JointModifier, 64> modifiers;

		void SetIdentity()
		{
			for (size_t i = 0; i != modifiers.size(); ++i)
			{
				modifiers[i].rotX = 1.0f;
				modifiers[i].rotY = 1.0f;
				modifiers[i].rotZ = 1.0f;
				modifiers[i].rotW = 1.0f;
			}
		}
	};

	struct Model
	{
		size_t vertexBufferIndex;
		size_t indexBufferIndex;

		size_t pipelineIndex;
		size_t descriptorSetIndex;

		static inline Model GetModel(size_t _vertexBufferIndex, size_t _indexBufferIndex, size_t _pipelineIndex, size_t _descriptorSetIndex)
		{
			return{ _vertexBufferIndex, _indexBufferIndex, _pipelineIndex, _descriptorSetIndex };
		}
	};

	struct Object
	{
		glm::mat4 transform;

		bool visible;
		size_t modelID;
		size_t skeletonID;
		size_t animationID;
		size_t animationModifierID;
		float animationStart;
		float animationEnd;
		float animationTime;
		float animationSpeedMultiplier;
		btRigidBody* rigidBody;

		size_t parent;
		size_t parentJointIndex;
		std::vector<size_t> children;

		std::vector<void(*)(void*)> behaviors;

		static inline Object* GetObjectA(glm::mat4 _transform, bool _visible, size_t _modelID, size_t _skeletonID, size_t _animationID, size_t _animationModifierID, float _animationStart, float _animationEnd, float _animationTime, float _animationSpeedMultiplier, btRigidBody* _rigidBody, size_t _parent, size_t _parentJointIndex, std::vector<size_t> _children, std::vector<void(*)(void*)> _behaviors)
		{
			Object* result = new Object;

			result->transform = _transform;

			result->visible = _visible;
			result->modelID = _modelID;
			result->skeletonID = _skeletonID;
			result->animationID = _animationID;
			result->animationModifierID = _animationModifierID;
			result->animationStart = _animationStart;
			result->animationEnd = _animationEnd;
			result->animationTime = _animationTime;
			result->animationSpeedMultiplier = _animationSpeedMultiplier;
			result->rigidBody = _rigidBody;
			if(_rigidBody != nullptr)
				_rigidBody->setUserPointer(result);

			result->parent = _parent;
			result->parentJointIndex = _parentJointIndex;
			result->children = _children;

			result->behaviors = _behaviors;

			return result;
		}

		glm::mat4 GetGlobalTRansform(Scene2& _scene)
		{
			if (parent != -1) /// TODO: use parentJointIndex
				return transform * _scene.objects[parent]->GetGlobalTRansform(_scene);
			else
				return transform;
		}
	};

	std::vector<Model> models;
	std::vector<Skeleton> skeletons;
	std::vector<AnimationModifier> animationModifiers;
	std::vector<Object*> objects;

	struct SceneDataProperties
	{
		std::vector<const char*> animationsNames;
	};
	void Load(SceneDataProperties _sceneDataProperties);

	void Cleanup()
	{
		for (size_t i = 0; i != objects.size(); ++i)
		{
			if(objects[i]->rigidBody != nullptr)
				Physics2::DeleteBtRigidBody(objects[i]->rigidBody);
			delete objects[i];
		}

		objects.clear();
	};
};

#endif