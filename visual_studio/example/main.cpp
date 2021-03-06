// project.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "SceneSelectorScene.h"
#include "SampleSceneTessIco.h"
#include "ModelExplorerScene.h"

int main()
{
	// Start with a window of the given size and provide a lambda which returns the first scene to be displayed:
	e186::Engine::StartWindowedWithRootScene(1600, 900, []()
	{
		// Create the root scene, which is a scene that offers to load other scenes
		auto root_scene = std::make_unique<e186::SceneSelectorScene>();
		//   Prepare an instance of the "e186::SampleSceneTessIco" scene
		root_scene->AddScene<e186::SampleSceneTessIco>();
		//   Prepare the creation of an instance of the "e186::ModelExplorerScene" scene which loads the given 3D model file and renders it at the given scale
		root_scene->AddSceneGenFunc("Model Explorer: Sponza", []() {
			//                                                 3D model to load                            scale for the 3D model						
			return std::make_unique<e186::ModelExplorerScene>("assets/models/sponza/sponza_structure.obj", glm::scale(glm::vec3{.1f, .1f, .1f}));
		});
		// Start with the root scene
		return root_scene;
	});
}
