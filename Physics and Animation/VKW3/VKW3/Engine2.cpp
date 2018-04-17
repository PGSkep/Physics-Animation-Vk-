#include "Engine2.h"

#include "Behaviors.h"

double Engine2::deltaTime;
double Engine2::timeMultiplier;

void Engine2::Init()
{
	// Renderer
	{
		timer.SetResolution(Timer::RESOLUTION::RESOLUTION_NANOSECONDS);
		timer.Play();

		Renderer2::RendererInitProperties rendererInitProperties;
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
		rendererInitProperties.maxAnimationCount = 2;
		rendererInitProperties.maxModelMatrixCount = 50;

		renderer.Init(rendererInitProperties);
	}

	physics.Init();
}

void Engine2::Load()
{
	// Renderer
	{
		Renderer2::RendererDataProperties rendererDataProperties;
		rendererDataProperties.shaderModulesNames = {
			{ "Shaders/Animated/" "vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "main" },
			{ "Shaders/Animated/" "frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main" },

			{ "Shaders/" "vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "main" },
			{ "Shaders/" "frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main" } };
		rendererDataProperties.textureNames = { "Textures/Test.tga" };
		rendererDataProperties.modelNames = {
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

		renderer.Load(rendererDataProperties);
	}

	// Scene
	{
		Scene2::SceneDataProperties sceneDataProperties;
		sceneDataProperties.animationsNames = {
			{ "Models/goblin.dae" },
		};
		scene.Load(sceneDataProperties);

		scene.animationModifiers = { {} };
		scene.animationModifiers[0].SetIdentity();
		scene.animationModifiers[0].modifiers[5].weight = 1.0f;
		scene.animationModifiers[0].modifiers[6].weight = 1.0f;
		scene.animationModifiers[0].modifiers[7].weight = 1.0f;

		scene.models = 
		{
			Scene2::Model::GetModel(7, 7, 0, 0),
			Scene2::Model::GetModel(0, 0, 1, 0),
			Scene2::Model::GetModel(3, 3, 1, 0),
			Scene2::Model::GetModel(4, 4, 1, 0),
			Scene2::Model::GetModel(5, 5, 1, 0),
			Scene2::Model::GetModel(2, 2, 1, 0),
			Scene2::Model::GetModel(6, 6, 1, 0),
		};
		scene.objects = 
		{
			Scene2::Object::GetObjectA(glm::scale(glm::translate(glm::mat4(), glm::vec3(10.0f, 0.0f, 0.0f)), glm::vec3(0.002f, 0.002f, 0.002f))	, true, 0,  0,  0,  0, 0.0f, 3.333f	, 0.0f, 1.0f, nullptr,									-1, -1,{},{ Behaviors::UpdateAnimationTime }),
			Scene2::Object::GetObjectA(glm::scale(glm::mat4(), glm::vec3(500.0f, 500.0f, 500.0f))												, true, 1, -1, -1, -1, 0.0f, 0.0f	, 0.0f, 0.0f, physics.AddSphere(3, 2, 10, 1, 1),		0, 25,{},{  }),
			Scene2::Object::GetObjectA(glm::scale(glm::mat4(), glm::vec3(10.0f, 10.0f, 10.0f))													, true, 2, -1, -1, -1, 0.0f, 0.0f	, 0.0f, 0.0f, nullptr,									-1, -1,{},{}),
			Scene2::Object::GetObjectA(glm::mat4()																								, true, 3, -1, -1, -1, 0.0f, 0.0f	, 0.0f, 0.0f, physics.AddCylinder(-2, 2, 2, 1, 1, 1),	-1, -1,{},{ Behaviors::SetTransformToRigidbody }),
			Scene2::Object::GetObjectA(glm::mat4()																								, true, 4, -1, -1, -1, 0.0f, 0.0f	, 0.0f, 0.0f, physics.AddCone(2, -2, 2, 1, 1, 1),		-1, -1,{},{ Behaviors::SetTransformToRigidbody }),
			Scene2::Object::GetObjectA(glm::mat4()																								, true, 5, -1, -1, -1, 0.0f, 0.0f	, 0.0f, 0.0f, physics.AddCube(2, 2, -2, 1, 1, 1, 1),	-1, -1,{},{ Behaviors::SetTransformToRigidbody }),
			Scene2::Object::GetObjectA(glm::mat4()																								, true, 6, -1, -1, -1, 0.0f, 0.0f	, 0.0f, 0.0f, physics.AddCapsule(0, 2, 2, 1, 1, 1),		-1, -1,{},{ Behaviors::SetTransformToRigidbody }),
		};
	}
}

void Engine2::Setup()
{
	// Renderer
	{
		Renderer2::RendererSetupProperties rendererSetupProperties;
		rendererSetupProperties.shaders = {
				{ 0, 1 },
				{ 2, 3 }
		};
		rendererSetupProperties.pipelines.resize(2);
		rendererSetupProperties.pipelines[0].shaderStateIndex = 0;
		rendererSetupProperties.pipelines[0].vertexInputStateIndex = 0;
		rendererSetupProperties.pipelines[0].inputAssemblyStateIndex = 0;
		rendererSetupProperties.pipelines[0].viewportStateIndex = 0;
		rendererSetupProperties.pipelines[0].rasterizationStateIndex = 0;
		rendererSetupProperties.pipelines[0].multisampleStateIndex = 0;
		rendererSetupProperties.pipelines[0].depthStencilStateIndex = 0;
		rendererSetupProperties.pipelines[0].colorBlendStateIndex = 0;

		rendererSetupProperties.pipelines[1].shaderStateIndex = 1;
		rendererSetupProperties.pipelines[1].vertexInputStateIndex = 1;
		rendererSetupProperties.pipelines[1].inputAssemblyStateIndex = 0;
		rendererSetupProperties.pipelines[1].viewportStateIndex = 0;
		rendererSetupProperties.pipelines[1].rasterizationStateIndex = 1;
		rendererSetupProperties.pipelines[1].multisampleStateIndex = 0;
		rendererSetupProperties.pipelines[1].depthStencilStateIndex = 0;
		rendererSetupProperties.pipelines[1].colorBlendStateIndex = 0;

		renderer.Setup(rendererSetupProperties);
	}

	startTime = timer.GetTime();
	deltaTime = timer.GetTime() - startTime;

	timeMultiplier = 0.1f;
}

void Engine2::Loop()
{
	startTime = timer.GetTime();

	game.Update(scene);
	renderer.Render(scene);
	physics.Update();

	deltaTime = timer.GetTime() - startTime;
}

void Engine2::ShutDown()
{
	renderer.ShutDown();
	physics.ShutDown();
	scene.Cleanup();
}
