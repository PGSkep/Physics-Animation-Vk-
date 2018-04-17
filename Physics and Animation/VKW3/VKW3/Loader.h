#ifndef	LOADER_H
#define LOADER_H

#include <stdio.h>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <map>

#include "VkU.h"

#include "Scene2.h"

namespace Loader
{
	// shader
	static void LoadShader(const char* _filename, size_t& _fileSize, char** _buffer)
	{
		std::ifstream file(_filename, std::ios::ate | std::ios::binary);

		if (file.is_open())
		{
			_fileSize = (size_t)file.tellg();
			file.seekg(0);

			if (file.good())
			{
				*_buffer = new char[_fileSize];
				file.read(*_buffer, _fileSize);
			}
			else
			{
				_fileSize = 0;
			}
		}

		file.close();
	}

	// image
	enum IMAGE_TYPE
	{
		UNKNOWN = 0,
		BGR8,
		BGRA8,
	};
	static void LoadTGA(const char* _filename, uint16_t& _width, uint16_t& _height, uint32_t& _size, IMAGE_TYPE& _imageType, uint8_t*& _data)
	{
		_width = 0;
		_height = 0;
		_size = 0;
		_imageType = UNKNOWN;
		_data = nullptr;

		uint8_t header[12];

		FILE* fTGA;
		fTGA = fopen(_filename, "rb");

		uint8_t cTGAcompare[12] = { 0,0,10,0,0,0,0,0,0,0,0,0 };
		uint8_t uTGAcompare[12] = { 0,0, 2,0,0,0,0,0,0,0,0,0 };

		if (fTGA == NULL)
			return;
		if (fread(&header, sizeof(header), 1, fTGA) == 0)
			return;
		if (memcmp(cTGAcompare, &header, sizeof(header)) == 0)
		{
			// TODO: Load Compressed
		}
		else if (memcmp(uTGAcompare, &header, sizeof(header)) == 0)
		{
			uint8_t header[6];

			if (fread(header, sizeof(header), 1, fTGA) == 0)
				return;

			_width = header[1] * 256 + header[0];
			_height = header[3] * 256 + header[2];
			uint8_t bpp = header[4];

			if (_width == 0 || _height == 0 || (bpp != 24 && bpp != 32))
				return;

			if (bpp == 24)
				_imageType = BGR8;
			else if (bpp == 32)
				_imageType = BGRA8;

			bpp = bpp / 8;
			_size = bpp * _width * _height;

			_data = new uint8_t[_size];
			if (fread(_data, 1, _size, fTGA) != _size)
			{
				_width = 0;
				_height = 0;
				_size = 0;
				_imageType = UNKNOWN;
				bpp = 0;
				delete[] _data;
				_data = nullptr;
			}
		}
		else return;
	}

	// model
	static glm::mat4 aiMatrix4x4ToGlmMat4(aiMatrix4x4 _mat)
	{
		glm::mat4 glmMat4;

		memcpy(&glmMat4[0][0], &_mat[0][0], sizeof(glm::mat4));

		return glmMat4;
	}
	struct Mesh
	{
		VkU::VERTEX_TYPE vertexType;

		uint16_t vertexCount;
		uint64_t vertexSize;
		uint8_t* vertexData;

		uint16_t indexCount;
		uint64_t indexSize;
		uint8_t* indexData;
	};

	static void IterateAssimpMeshes(Scene2::Skeleton& _skeleton, const aiScene* _scene, std::map<std::string, size_t>& _boneMap)
	{
		for (size_t m = 0; m != _scene->mNumMeshes; ++m)
		{
			const aiMesh* paiMesh = _scene->mMeshes[m];

			for (size_t b = 0; b != paiMesh->mNumBones; ++b)
			{
				size_t boneIndex = _boneMap[paiMesh->mBones[b]->mName.C_Str()];
				_skeleton.joints[boneIndex].boneID = b;
				_skeleton.joints[boneIndex].offset = paiMesh->mBones[b]->mOffsetMatrix;
			}
		}
	}
	static void IterateAssimpAnimation(Scene2::Skeleton& _skeleton, aiNode* _node, aiAnimation* _animation, size_t _parentIndex, std::map<std::string, size_t>& _boneMap)
	{
		Scene2::Skeleton::Joint joint;
		joint.name = _node->mName.C_Str();
		joint.parentIndex = _parentIndex;
		joint.transformation = _node->mTransformation;

		aiNodeAnim* pNodeAnim = nullptr;
		for (unsigned int i = 0; i != _animation->mNumChannels; ++i)
		{
			if (strcmp(joint.name.c_str(), _animation->mChannels[i]->mNodeName.C_Str()) == 0)
			{
				pNodeAnim = _animation->mChannels[i];
				break;
			}
		}

		if (pNodeAnim != nullptr)
		{
			for (size_t i = 0; i != pNodeAnim->mNumPositionKeys; ++i)
			{
				Scene2::Skeleton::Joint::Channel3D channel;
				channel.time = (float)pNodeAnim->mPositionKeys[i].mTime;
				channel.channel.x = pNodeAnim->mPositionKeys[i].mValue.x;
				channel.channel.y = pNodeAnim->mPositionKeys[i].mValue.y;
				channel.channel.z = pNodeAnim->mPositionKeys[i].mValue.z;
				joint.translateChannels.push_back(channel);
			}
			for (size_t i = 0; i != pNodeAnim->mNumRotationKeys; ++i)
			{
				Scene2::Skeleton::Joint::ChannelQuaternion channel;
				channel.time = (float)pNodeAnim->mRotationKeys[i].mTime;
				channel.channel = pNodeAnim->mRotationKeys[i].mValue;
				joint.rotateChannels.push_back(channel);
			}
			for (size_t i = 0; i != pNodeAnim->mNumScalingKeys; ++i)
			{
				Scene2::Skeleton::Joint::Channel3D channel;
				channel.time = (float)pNodeAnim->mScalingKeys[i].mTime;
				channel.channel.x = pNodeAnim->mScalingKeys[i].mValue.x;
				channel.channel.y = pNodeAnim->mScalingKeys[i].mValue.y;
				channel.channel.z = pNodeAnim->mScalingKeys[i].mValue.z;
				joint.scaleChannels.push_back(channel);
			}
		}

		_skeleton.joints.push_back(joint);

		size_t thisIndex = _skeleton.joints.size() - 1;
		_boneMap[joint.name.c_str()] = thisIndex;

		for (size_t i = 0; i != _node->mNumChildren; ++i)
			IterateAssimpAnimation(_skeleton, _node->mChildren[i], _animation, thisIndex, _boneMap);
	}
	static void ParseAssimpSkeleton(Scene2::Skeleton& _skeleton, const aiScene* _scene, aiAnimation* _animation)
	{
		//if (strcmp(_scene->mRootNode->mName.C_Str(), "goblin_max" ) == 0)
		{
			std::map<std::string, size_t> boneMap;
			IterateAssimpAnimation(_skeleton, _scene->mRootNode, _animation, -1, boneMap);

			_skeleton.globalInverseTransform = _scene->mRootNode->mTransformation;
			_skeleton.globalInverseTransform.Inverse();

			IterateAssimpMeshes(_skeleton, _scene, boneMap);

			size_t deformerIndex = 64;
			for (size_t i = 0; i != _skeleton.joints.size(); ++i)
			{
				if (_skeleton.joints[i].boneID == -1)
				{
					_skeleton.joints[i].boneID = --deformerIndex;
				}
			}
		}
	}

	static bool LoadModelASSIMP(Mesh* _mesh, Scene2::Skeleton* _skeleton, const char* _fileName, int _aiProcess_Flags)
	{
		// force mesh to be triangulated
		{
			if ((_aiProcess_Flags & aiProcess_Triangulate) != aiProcess_Triangulate)
				_aiProcess_Flags = _aiProcess_Flags | aiProcess_Triangulate;
		}

		// Import file
		Assimp::Importer Importer;
		const aiScene* pScene;
		{
			pScene = Importer.ReadFile(_fileName, _aiProcess_Flags);
			if (pScene == nullptr)
				return false;
		}

		// Parse skeleton and get data
		if (pScene->mRootNode != nullptr && _skeleton != nullptr)
		{
			if (pScene->mNumAnimations > 0)
				ParseAssimpSkeleton(*_skeleton, pScene, pScene->mAnimations[0]);
			else
				ParseAssimpSkeleton(*_skeleton, pScene, nullptr);

			pScene->mNumMeshes;
		}

		if (_mesh != nullptr)
		{
			// Get the meshes properties
			struct meshProperties
			{
				uint16_t vertexCount;
				uint16_t indexCount;
				uint16_t jointCount;
				uint16_t uvCount;

				uint32_t firstIndex;

				bool hasNormals;
				bool hasTangentsAndBitangents;
			};
			uint32_t indexSum = 0;
			std::vector<meshProperties> props(pScene->mNumMeshes);
			{
				for (auto i = 0; i != props.size(); ++i)
				{
					// gets a mesh
					const aiMesh* paiMesh = pScene->mMeshes[i];

					// evaluates a mesh
					props[i].vertexCount = paiMesh->mNumVertices;
					props[i].indexCount = paiMesh->mNumFaces * 3;
					props[i].jointCount = paiMesh->mNumBones;

					props[i].firstIndex = indexSum;
					indexSum += props[i].indexCount;

					if (paiMesh->HasTextureCoords(0) == true)
						props[i].uvCount = *paiMesh->mNumUVComponents;
					else
						props[i].uvCount = 0;
					props[i].hasNormals = paiMesh->HasNormals();
					props[i].hasTangentsAndBitangents = paiMesh->HasTangentsAndBitangents();
				}
			}

			// Get vertex type
			VkU::VERTEX_TYPE vertexType;
			meshProperties sumProperties;
			{
				sumProperties.firstIndex = 0;
				sumProperties.vertexCount = 0;
				sumProperties.indexCount = 0;
				sumProperties.uvCount = 0;
				sumProperties.jointCount = 0;
				sumProperties.hasNormals = false;
				sumProperties.hasTangentsAndBitangents = false;

				for (auto i = 0; i != props.size(); ++i)
				{
					sumProperties.vertexCount += props[i].vertexCount;
					sumProperties.indexCount += props[i].indexCount;
					sumProperties.uvCount += props[i].uvCount;
					sumProperties.jointCount += props[i].jointCount;
					if (props[i].hasNormals == true)
						sumProperties.hasNormals = true;
					if (props[i].hasTangentsAndBitangents)
						sumProperties.hasTangentsAndBitangents = true;
				}

				if (sumProperties.vertexCount == 0 || sumProperties.indexCount == 0)
					vertexType = VkU::VERTEX_TYPE::UNKNOWN;

				else if (sumProperties.uvCount == 0 && sumProperties.jointCount == 0 && sumProperties.hasNormals == false && sumProperties.hasTangentsAndBitangents == false)
					vertexType = VkU::VERTEX_TYPE::P3;
				else if (sumProperties.uvCount > 0 && sumProperties.jointCount == 0 && sumProperties.hasNormals == false && sumProperties.hasTangentsAndBitangents == false)
					vertexType = VkU::VERTEX_TYPE::P3U;
				else if (sumProperties.uvCount == 0 && sumProperties.jointCount == 0 && sumProperties.hasNormals == true && sumProperties.hasTangentsAndBitangents == false)
					vertexType = VkU::VERTEX_TYPE::P3N;
				else if (sumProperties.uvCount > 0 && sumProperties.jointCount == 0 && sumProperties.hasNormals == true && sumProperties.hasTangentsAndBitangents == false)
					vertexType = VkU::VERTEX_TYPE::P3UN;
				else if (sumProperties.uvCount > 0 && sumProperties.jointCount == 0 && sumProperties.hasNormals == true && sumProperties.hasTangentsAndBitangents == true)
					vertexType = VkU::VERTEX_TYPE::P3UNTB;
				else if (sumProperties.uvCount == 0 && sumProperties.jointCount > 0 && sumProperties.hasNormals == false && sumProperties.hasTangentsAndBitangents == false)
					vertexType = VkU::VERTEX_TYPE::P3S4;
				else if (sumProperties.uvCount > 0 && sumProperties.jointCount > 0 && sumProperties.hasNormals == false && sumProperties.hasTangentsAndBitangents == false)
					vertexType = VkU::VERTEX_TYPE::P3US4;
				else if (sumProperties.uvCount == 0 && sumProperties.jointCount > 0 && sumProperties.hasNormals == true && sumProperties.hasTangentsAndBitangents == false)
					vertexType = VkU::VERTEX_TYPE::P3NS4;
				else if (sumProperties.uvCount > 0 && sumProperties.jointCount > 0 && sumProperties.hasNormals == true && sumProperties.hasTangentsAndBitangents == false)
					vertexType = VkU::VERTEX_TYPE::P3UNS4;
				else if (sumProperties.uvCount > 0 && sumProperties.jointCount > 0 && sumProperties.hasNormals == true && sumProperties.hasTangentsAndBitangents == true)
					vertexType = VkU::VERTEX_TYPE::P3UNTBS4;

				else
					vertexType = VkU::VERTEX_TYPE::UNKNOWN;
			}

			// Get vertex size
			uint64_t vertexSize = 0;
			{
				vertexSize += sizeof(float) * 3 * sumProperties.vertexCount; // position

				if (sumProperties.uvCount > 0)
					vertexSize += sizeof(float) * 2 * sumProperties.vertexCount; // uv

				if (sumProperties.hasNormals == true)
					vertexSize += sizeof(float) * 3 * sumProperties.vertexCount; // normal

				if (sumProperties.hasTangentsAndBitangents == true)
					vertexSize += sizeof(float) * 6 * sumProperties.vertexCount; // tangent / bitangent

				if (sumProperties.jointCount > 0)
				{
					vertexSize += sizeof(float) * 4 * sumProperties.vertexCount; // weight
					vertexSize += sizeof(uint16_t) * 4 * sumProperties.vertexCount; // id
				}
			}

			// Get bones data // Get bone vertex data
			std::map<std::string, uint32_t> boneMapping;
			struct vertexBoneData
			{
				std::array<uint16_t, 4> IDs;
				std::array<float, 4> weights;

				void add(uint16_t _ID, float _weight)
				{
					for (uint16_t i = 0; i != 4; ++i)
					{
						if (weights[i] == 0.0f)
						{
							IDs[i] = _ID;
							weights[i] = _weight;
							return;
						}
					}
				}
			};
			std::vector<vertexBoneData> vertBoneData(props[0].vertexCount);
			{
				for (auto i = 0; i != props.size(); ++i)
				{
					const aiMesh* paiMesh = pScene->mMeshes[i];

					uint32_t numBones = 0;

					for (uint32_t j = 0; j < paiMesh->mNumBones; ++j)
					{
						uint32_t index = 0;

						std::string name(paiMesh->mBones[j]->mName.data);

						if (boneMapping.find(name) == boneMapping.end())
						{
							// Bone not present, add new one
							index = numBones;
							numBones++;
							boneMapping[name] = index;
							paiMesh->mNumAnimMeshes;
						}
						else
						{
							index = boneMapping[name];
						}

						for (uint32_t k = 0; k < paiMesh->mBones[j]->mNumWeights; k++)
						{
							uint32_t vertexID = props[i].firstIndex + paiMesh->mBones[j]->mWeights[k].mVertexId;
							vertBoneData[vertexID].add(index, paiMesh->mBones[j]->mWeights[k].mWeight);
						}
					}
				}
			}

			// Get vertex data
			uint8_t* vertexData = new uint8_t[vertexSize];
			ZeroMemory(vertexData, vertexSize);
			{
				size_t dataPos = 0;

				uint8_t emptyf2[sizeof(float) * 2];
				ZeroMemory(emptyf2, sizeof(float) * 2);
				uint8_t emptyf9[sizeof(float) * 9];
				ZeroMemory(emptyf9, sizeof(float) * 9);
				uint8_t empty16u4f4[sizeof(float) * 4 + sizeof(uint16_t) * 4];
				ZeroMemory(empty16u4f4, sizeof(float) * 4 + sizeof(uint16_t) * 4);

				for (auto i = 0; i != props.size(); ++i) /// TODO: Check is memcmp s can be merged
				{
					const aiMesh* paiMesh = pScene->mMeshes[i];

					for (auto j = 0; j != paiMesh->mNumVertices; ++j)
					{
						memcpy(&vertexData[dataPos], &paiMesh->mVertices[j].x, sizeof(float));
						dataPos += sizeof(float);
						memcpy(&vertexData[dataPos], &paiMesh->mVertices[j].y, sizeof(float));
						dataPos += sizeof(float);
						memcpy(&vertexData[dataPos], &paiMesh->mVertices[j].z, sizeof(float));
						dataPos += sizeof(float);

						if (sumProperties.uvCount > 0)
						{
							if (props[i].uvCount > 0)
							{
								memcpy(&vertexData[dataPos], &paiMesh->mTextureCoords[0][j].x, sizeof(float));
								dataPos += sizeof(float);
								memcpy(&vertexData[dataPos], &paiMesh->mTextureCoords[0][j].y, sizeof(float));
								dataPos += sizeof(float);
							}
							else
							{
								memcpy(&vertexData[dataPos], &emptyf2, sizeof(float) * 2);
								dataPos += sizeof(float) * 2;
							}
						}

						if (sumProperties.hasNormals && sumProperties.hasTangentsAndBitangents)
						{
							if (props[i].hasNormals && props[i].hasTangentsAndBitangents)
							{
								memcpy(&vertexData[dataPos], &paiMesh->mNormals[j].x, sizeof(float));
								dataPos += sizeof(float);
								memcpy(&vertexData[dataPos], &paiMesh->mNormals[j].y, sizeof(float));
								dataPos += sizeof(float);
								memcpy(&vertexData[dataPos], &paiMesh->mNormals[j].z, sizeof(float));
								dataPos += sizeof(float);

								memcpy(&vertexData[dataPos], &paiMesh->mTangents[j].x, sizeof(float));
								dataPos += sizeof(float);
								memcpy(&vertexData[dataPos], &paiMesh->mTangents[j].y, sizeof(float));
								dataPos += sizeof(float);
								memcpy(&vertexData[dataPos], &paiMesh->mTangents[j].z, sizeof(float));
								dataPos += sizeof(float);

								memcpy(&vertexData[dataPos], &paiMesh->mBitangents[j].x, sizeof(float));
								dataPos += sizeof(float);
								memcpy(&vertexData[dataPos], &paiMesh->mBitangents[j].y, sizeof(float));
								dataPos += sizeof(float);
								memcpy(&vertexData[dataPos], &paiMesh->mBitangents[j].z, sizeof(float));
								dataPos += sizeof(float);
							}
							else
							{
								memcpy(&vertexData[dataPos], &emptyf9, sizeof(float) * 9);
								dataPos += sizeof(float) * 9;
							}
						}

						if (sumProperties.jointCount > 0)
						{
							if (props[i].jointCount > 0)
							{
								memcpy(&vertexData[dataPos], &vertBoneData[j].weights[0], sizeof(float));
								dataPos += sizeof(float);
								memcpy(&vertexData[dataPos], &vertBoneData[j].weights[1], sizeof(float));
								dataPos += sizeof(float);
								memcpy(&vertexData[dataPos], &vertBoneData[j].weights[2], sizeof(float));
								dataPos += sizeof(float);
								memcpy(&vertexData[dataPos], &vertBoneData[j].weights[3], sizeof(float));
								dataPos += sizeof(float);

								memcpy(&vertexData[dataPos], &vertBoneData[j].IDs[0], sizeof(uint16_t));
								dataPos += sizeof(uint16_t);
								memcpy(&vertexData[dataPos], &vertBoneData[j].IDs[1], sizeof(uint16_t));
								dataPos += sizeof(uint16_t);
								memcpy(&vertexData[dataPos], &vertBoneData[j].IDs[2], sizeof(uint16_t));
								dataPos += sizeof(uint16_t);
								memcpy(&vertexData[dataPos], &vertBoneData[j].IDs[3], sizeof(uint16_t));
								dataPos += sizeof(uint16_t);
							}
							else
							{
								memcpy(&vertexData[dataPos], &empty16u4f4, sizeof(float) * 4 + sizeof(uint16_t) * 4);
								dataPos += sizeof(float) * 4 + sizeof(uint16_t) * 4;
							}
						}
					}
				}
			}

			// Get indices count
			uint32_t indiceCount = 0;
			{
				for (uint32_t i = 0; i != props.size(); ++i)
				{
					indiceCount += props[i].indexCount;
				}
			}

			// Get indices
			uint64_t indiceSize = indiceCount * sizeof(uint16_t);
			uint8_t* indiceData = new uint8_t[indiceSize];
			ZeroMemory(indiceData, indiceCount * sizeof(uint16_t));
			{
				size_t dataPos = 0;

				for (size_t i = 0; i != props.size(); ++i)
				{
					const aiMesh* paiMesh = pScene->mMeshes[i];

					for (unsigned int j = 0; j < paiMesh->mNumFaces; ++j)
					{
						const aiFace& Face = paiMesh->mFaces[j];
						if (Face.mNumIndices != 3)
							continue;

						uint16_t val = props[i].firstIndex + Face.mIndices[0];
						memcpy(&indiceData[dataPos], &val, sizeof(uint16_t));
						dataPos += sizeof(uint16_t);
						val = props[i].firstIndex + Face.mIndices[1];
						memcpy(&indiceData[dataPos], &val, sizeof(uint16_t));
						dataPos += sizeof(uint16_t);
						val = props[i].firstIndex + Face.mIndices[2];
						memcpy(&indiceData[dataPos], &val, sizeof(uint16_t));
						dataPos += sizeof(uint16_t);
					}
				}
			}

			// Get vertex Count
			uint32_t vertexCount = 0;
			{
				for (size_t i = 0; i != props.size(); ++i)
					vertexCount += props[i].vertexCount;
			}

			_mesh->vertexType = vertexType;

			_mesh->vertexCount = vertexCount;
			_mesh->vertexSize = vertexSize;
			_mesh->vertexData = vertexData;

			_mesh->indexCount = indiceCount;
			_mesh->indexSize = indiceSize;
			_mesh->indexData = indiceData;
		}
		return true;
	}
}

#endif