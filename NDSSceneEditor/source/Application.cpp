#include "Application.h"

#include "glm/gtc/matrix_transform.hpp"
#include "services/ServiceLocator.h"

#include "ecs/ComponentDataArray.h"
#include "ecs/SystemBase.h"
#include "systems/SysScenegraph.h"

#include "components/CmpTransform.h"
#include "components/CmpModel.h"

namespace nds_se
{
	Application::Application() : 
		m_window(1600, 900, false, "NDS Scene Editor"),
		m_camera(glm::vec3(4, 3, 3), glm::vec3(0), glm::vec3(0, 1, 0))
	{
		// Setup logger UI
		m_logViewUICallbackID = LOGGER->registerCallback(std::bind(&LogView::log, &m_logViewUI, std::placeholders::_1));

		// Setup resource managers
		ServiceLocator::get().provideService<ResourceAllocator<Texture>>(&m_textureManager);
		ServiceLocator::get().provideService<ResourceAllocator<ShaderProgram>>(&m_shaderManager);
		ServiceLocator::get().provideService<ResourceAllocator<Model>>(&m_modelManager);
		ServiceLocator::get().provideService<Renderer>(&m_renderer);
		ServiceLocator::get().provideService<Camera>(&m_camera);
		ServiceLocator::get().provideService<Window>(&m_window);

		// Setup rendering
		m_renderer.setViewProjectionMatrix(m_camera.getViewProjectionMatrix(m_window.getSize()));

		ResourceID<ShaderProgram> shader = m_shaderManager.loadResource("resources/shaders/SimpleShd.glsl.vs", "resources/shaders/SimpleShd.glsl.fs");
		m_renderer.setShaderProgram(shader);

		// Load default texture
		m_textureManager.loadResource("resources/textures/WhiteTex.png", false);

		// Setup entities/systems
		ServiceLocator::get().provideService<EntityManager>(&m_entityManager);
		m_entityManager.registerSystem<SysScenegraph>();
	}

	Application::~Application()
	{
		// Remove logger UI
		LOGGER->unregisterCallback(m_logViewUICallbackID);
	}

	void Application::run()
	{
		// Create systems
		std::unique_ptr<ISystemBase> SysModelRendering(new SystemBase<CmpTransform, CmpModel>("ModelRendering",
			[](Entity entity, CmpTransform& transform, CmpModel& model){
			Renderer* renderer = ServiceLocator::get().getService<Renderer>();
			Camera* camera = ServiceLocator::get().getService<Camera>();
			Window* window = ServiceLocator::get().getService<Window>();
			
			renderer->setViewProjectionMatrix(camera->getViewProjectionMatrix(window->getSize()));
			renderer->drawModel(model.model, transform.matrix);
		}
		));
		m_entityManager.registerSystem(std::move(SysModelRendering));

		// Register missing components
		m_entityManager.registerComponent<std::string>();
		m_entityManager.registerComponent<int>();
		m_entityManager.registerComponent<float>();
		
		ResourceID<Model> model = m_modelManager.loadResource("resources/models/boxroom/Boxroom.obj");
		CmpModel cmpModel;
		cmpModel.model = model;

		Entity entModel = m_entityManager.createEntity();
		m_entityManager.setComponent<std::string>(entModel, "model0");
		m_entityManager.setComponent(entModel, cmpModel);
		m_entityManager.addComponent<CmpTransform>(entModel);
		
		glm::vec3 modelTranslation = { 0.0f, 0.0f, 0.0f };

		float currentTime = (float)glfwGetTime();
		float lastTime = currentTime;

		while (!m_window.shouldClose())
		{
			// Update
			lastTime = currentTime;
			currentTime = (float)glfwGetTime();
			m_camera.updateCameraMovement(m_window, currentTime - lastTime);

			// Begin Render
			m_window.beginRender();
			m_viewportUI.render();

			m_entityManager.update();

			// Render models
			//m_renderer.setViewProjectionMatrix(m_camera.getViewProjectionMatrix(m_window.getSize()));
			//m_renderer.drawModel(model, glm::translate(glm::mat4(1), modelTranslation));

			m_logViewUI.render();
			m_window.endRender();
		}
	}
}