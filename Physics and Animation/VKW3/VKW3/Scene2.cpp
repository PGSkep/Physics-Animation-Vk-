#include "Scene2.h"

#include "Loader.h"

void Scene2::Load(SceneDataProperties _sceneDataProperties)
{
	for (size_t i = 0; i != _sceneDataProperties.animationsNames.size(); ++i)
	{
		Skeleton skeleton;
		if (Loader::LoadModelASSIMP(nullptr, &skeleton, _sceneDataProperties.animationsNames[i], aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices))
			skeletons.push_back(skeleton);
	}
}
