#ifndef	SCENE_H
#define SCENE_H

#include <array>
#include <vector>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

class Scene
{
public:
	struct Mat4x4
	{
		float mat[16];
	
		static inline Mat4x4 GetIdentity()
		{
			Mat4x4 matrix;
			matrix.mat[0] = 1.0f; matrix.mat[1] = 0.0f; matrix.mat[2] = 0.0f; matrix.mat[3] = 0.0f;
			matrix.mat[4] = 0.0f; matrix.mat[5] = 1.0f; matrix.mat[6] = 0.0f; matrix.mat[7] = 0.0f;
			matrix.mat[8] = 0.0f; matrix.mat[9] = 0.0f; matrix.mat[10] = 1.0f; matrix.mat[11] = 0.0f;
			matrix.mat[12] = 0.0f; matrix.mat[13] = 0.0f; matrix.mat[14] = 0.0f; matrix.mat[15] = 1.0f;
			return matrix;
		}
	};
	struct Model
	{
		size_t vertexBufferIndex;
		size_t indexBufferIndex;
	
		size_t pipelineIndex;
		size_t descriptorSetIndex;
	
		size_t animationIndex;
		size_t animationModifierIndex;
	
		static inline Model GetModel(uint32_t _vertexBufferIndex, uint32_t _indexBufferIndex, uint32_t _pipelineIndex, uint32_t _descriptorSetIndex, uint32_t _animationIndex)
		{
			return{ _vertexBufferIndex, _indexBufferIndex, _pipelineIndex, _descriptorSetIndex, _animationIndex };
		}
	};
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
	};
	
	struct Object
	{
		bool visible;
		Mat4x4 matrix;
		size_t modelID;
		size_t skeletonID;
		size_t animationModifierID;
		std::vector<Object> childs;
	
		static inline Object GetObjectA(bool _visible, size_t _modelID, size_t _skeletonID, size_t _animationModifierID, std::vector<Object> _childs)
		{
			return{ _visible, Mat4x4::GetIdentity(), _modelID, _skeletonID, _animationModifierID, _childs };
		}
		static inline Object GetObjectA(bool _visible, Mat4x4 _matrix, size_t _modelID, size_t _skeletonID, size_t _animationModifierID, std::vector<Object> _childs)
		{
			return{ _visible, _matrix, _modelID, _skeletonID, _animationModifierID, _childs };
		}
	};
	
	struct SceneProperties
	{
		std::vector<Model> models;
		std::vector<const char*> animationsNames;
		std::vector<Object> rootObjects;
	};
	
	std::vector<Model> models;
	std::vector<Skeleton> skeletons;
	std::vector<AnimationModifier> animationModifiers;
	
	std::vector<Object> rootObjects;
	
	void Setup(SceneProperties _sceneProperties);
};

#endif