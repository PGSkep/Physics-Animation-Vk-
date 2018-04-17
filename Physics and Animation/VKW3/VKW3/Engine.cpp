#include "Engine.h"

void Engine::Init()
{
	timer.SetResolution(Timer::RESOLUTION::RESOLUTION_NANOSECONDS);
	timer.Play();

	input.activeKeys = {
		Input::INPUT_KEYS::KEY_ESC,
		Input::INPUT_KEYS::KEY_R,
		Input::INPUT_KEYS::KEY_T,
		Input::INPUT_KEYS::KEY_Y,
		Input::INPUT_KEYS::KEY_F,
		Input::INPUT_KEYS::KEY_G,
		Input::INPUT_KEYS::KEY_H,
		Input::INPUT_KEYS::KEY_V,
		Input::INPUT_KEYS::KEY_B,
		Input::INPUT_KEYS::KEY_N,
		Input::INPUT_KEYS::KEY_SPACE,
	};

	Renderer::RendererInitProperties rendererInitProperties;
	{
		rendererInitProperties.appName = "appName";
		rendererInitProperties.appVersion = 0;
		rendererInitProperties.engineName = "engineName";
		rendererInitProperties.engineVersion = 0;
#ifdef LOG_RENDERER
		rendererInitProperties.enableDebug = true;
#else
		rendererInitProperties.enableDebug = false;
#endif
		rendererInitProperties.enableSwapchain = true;
		rendererInitProperties.debugReportCallback = nullptr;
		rendererInitProperties.debugFlags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT;// | VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
		rendererInitProperties.preferedDepthFormats = nullptr;
		rendererInitProperties.windowWidth = 1900;
		rendererInitProperties.windowHeight = 900;
		rendererInitProperties.windowTitle = "Title";
		rendererInitProperties.windowName = "Name";
		rendererInitProperties.queues = { VkU::Queue::GetQueue(VK_TRUE, VK_QUEUE_GRAPHICS_BIT, 1.0f, 1) };
		rendererInitProperties.useDepthBuffer = true;
		rendererInitProperties.targetSwapchainImageCount = 3;
	}
	renderer.Init(rendererInitProperties);
	physics.Init();

	Renderer::DataPackProperties dataPackProperties;
	{
		dataPackProperties.shaderModulesNames = {
			{ "Shaders/Animated/" "vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "main" },
			{ "Shaders/Animated/" "frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main" },

			{ "Shaders/" "vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "main" },
			{ "Shaders/" "frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main" } };
		dataPackProperties.textureNames = { "Textures/Test.tga" };
		dataPackProperties.modelNames = {
			{ "Models/icoSphere.dae" },	// 0
			{ "Models/cylinder.dae" },	// 1
			{ "Models/cube.dae" },		// 2
			{ "Models/plane.dae" },		// 3
			{ "Models/cylinder90rx.dae" },	// 4
			{ "Models/cone270rx.dae" },	// 5
			{ "Models/capsule.dae" },	// 6
			
			{ "Models/goblin.dae" },	// 7
			//{ "Models/Viking/Viking_WalkLight.fbx" },	// 7

		};

		dataPackProperties.maxModelMatrixCount = 50;
		dataPackProperties.maxAnimationCount = 2;
	}
	renderer.LoadScene(dataPackProperties);

	Renderer::SetupProperties setupProperties;
	{
		setupProperties.shaders = {
			{ 0, 1 },
			{ 2, 3 }
		};
		setupProperties.pipelines.resize(2);
		setupProperties.pipelines[0].shaderStateIndex = 0;
		setupProperties.pipelines[0].vertexInputStateIndex = 0;
		setupProperties.pipelines[0].inputAssemblyStateIndex = 0;
		setupProperties.pipelines[0].viewportStateIndex = 0;
		setupProperties.pipelines[0].rasterizationStateIndex = 0;
		setupProperties.pipelines[0].multisampleStateIndex = 0;
		setupProperties.pipelines[0].depthStencilStateIndex = 0;
		setupProperties.pipelines[0].colorBlendStateIndex = 0;

		setupProperties.pipelines[1].shaderStateIndex = 1;
		setupProperties.pipelines[1].vertexInputStateIndex = 1;
		setupProperties.pipelines[1].inputAssemblyStateIndex = 0;
		setupProperties.pipelines[1].viewportStateIndex = 0;
		setupProperties.pipelines[1].rasterizationStateIndex = 1;
		setupProperties.pipelines[1].multisampleStateIndex = 0;
		setupProperties.pipelines[1].depthStencilStateIndex = 0;
		setupProperties.pipelines[1].colorBlendStateIndex = 0;
	}
	renderer.Setup(setupProperties);

	scene.animationModifiers = {
		{}
	};
	scene.animationModifiers[0].modifiers[5].weight = 1.0f;
	scene.animationModifiers[0].modifiers[6].weight = 1.0f;
	scene.animationModifiers[0].modifiers[7].weight = 1.0f;

	Scene::SceneProperties sceneProperties;
	sceneProperties.rootObjects = {
		Scene::Object::GetObjectA(true, 0, 0, 0,{}), // anim
		Scene::Object::GetObjectA(true, 1, -1, -1,{}), // sphere
		Scene::Object::GetObjectA(true, 2, -1, -1,{}), // plane
		Scene::Object::GetObjectA(true, 3, -1, -1,{}), // cylinder
		Scene::Object::GetObjectA(true, 4, -1, -1,{}), // cone
		Scene::Object::GetObjectA(true, 5, -1, -1,{}), // cube
		Scene::Object::GetObjectA(true, 6, -1, -1,{}), // capsule
		Scene::Object::GetObjectA(true, 6, -1, -1,{}), // capsule sword
	};
	sceneProperties.models = {
		Scene::Model::GetModel(7, 7, 0, 0, 0), // anim
		Scene::Model::GetModel(0, 0, 1, 0, -1), // sphere
		Scene::Model::GetModel(3, 3, 1, 0, -1), // plane
		Scene::Model::GetModel(4, 4, 1, 0, -1), // cylinder
		Scene::Model::GetModel(5, 5, 1, 0, -1), // cone
		Scene::Model::GetModel(2, 2, 1, 0, -1), // cube
		Scene::Model::GetModel(6, 6, 1, 0, -1), // capsule
	};
	sceneProperties .animationsNames = {
		{ "Models/goblin.dae" },
	};
	scene.Setup(sceneProperties);

	physics.Setup();
	physics.AddSphere	(0.0f, 0.0f, 20.0f, 1.0f, 1.0f, 1);
	physics.AddCylinder	(0.0f, 0.1f, 15.0f, 1.0f, 1.0f, 1.0f, 3);
	physics.AddCone		(0.0f, 0.5f, 25.0f, 1.0f, 2.0f, 1.0f, 4);
	physics.AddCube		(0.0f, -0.6f, 5.0f, 1.0f, 1.0f, 1.0f, 1.0f, 5);
	physics.AddCapsule	(0.0f, 0.0f, 10.0f, 0.5f, 1.0f, 1.0f, 6);
	physics.AddCapsule	(0.0f, 0.0f, 10.0f, 0.5f, 1.0f, 1.0f, 7);

	startTime = timer.GetTime();
	deltaTime = timer.GetTime() - startTime;

	timeMultiplier = 1.0f;
}

void Engine::Loop()
{
	startTime = timer.GetTime();

	input.Update();
	physics.Update((float)(deltaTime * timeMultiplier), scene);

	game.Update(scene, input);

	renderer.Update(scene);
	renderer.Render();

	deltaTime = timer.GetTime() - startTime;
}

void Engine::ShutDown()
{
	physics.ShutDown();
	renderer.ShutDown();
}
