#include "Scene.h"

#include "Loader.h"

void Scene::Setup(SceneProperties _sceneProperties)
{
	models = _sceneProperties.models;

	for (size_t i = 0; i != _sceneProperties.animationsNames.size(); ++i)
	{
		Scene::Skeleton skeleton;
		//if (Loader::LoadModelASSIMP(nullptr, &skeleton, _sceneProperties.animationsNames[i], aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices))
			skeletons.push_back(skeleton);
	}

	rootObjects = _sceneProperties.rootObjects;
}
