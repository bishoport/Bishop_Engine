#include "TestLayer.h"
#include "Glad/glad.h"
#include <glfw/include/GLFW/glfw3.h>

#include "GLCore/Core/Input.h"
#include "GLCore/Core/KeyCodes.h"
#include <imgui_internal.h>
#include <ImGuizmo.h>

using namespace ECS;
using namespace GLCore;
using namespace GLCore::Utils;
using namespace GLCore::Loaders;


float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
	// positions   // texCoords
	-1.0f,  1.0f,  0.0f, 1.0f,
	-1.0f, -1.0f,  0.0f, 0.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,

	-1.0f,  1.0f,  0.0f, 1.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,
	 1.0f,  1.0f,  1.0f, 1.0f
};


TestLayer::TestLayer() : m_PerspectiveCameraController(16.0f / 9.0f) {}
TestLayer::~TestLayer(){}

void TestLayer::OnAttach()
{	

	//--ImGUI Configuration--------------------
	std::ifstream ifile("imgui.ini");
	ini_file_exists = ifile.is_open();
	ifile.close();

	if(ini_file_exists)
		ImGui::LoadIniSettingsFromDisk("imgui.ini");

	GLuint folderTexture = GLCore::Loaders::LoadIconTexture("assets/icons/folder_icon.png");
	GLuint modelTexture  = GLCore::Loaders::LoadIconTexture("assets/icons/model_icon.png");
	GLuint imageTexture  = GLCore::Loaders::LoadIconTexture("assets/icons/picture_icon.png");

	assetsPanel.setIcons(folderTexture, modelTexture, imageTexture);

	assetsPanel.SetDelegate([this](const std::string& filePath, const std::string& fileName) {
		this->getModelPathFromAssets(filePath, fileName);
		});
	//------------------------------------------------------------------------

	/*glEnable(GL_DEPTH_TEST);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_CLAMP);*/

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL); // set depth function to less than AND equal for skybox depth trick.

	//--OTHERS--------------------------------
	EnableGLDebugging();
	//m_CurrentDirectory = s_AssetPath;
	//------------------------------------------------------------------------

	

	//--CAMARA
	m_PerspectiveCameraController.GetCamera().SetPosition(glm::vec3(-1.492, 6.195f, 25.196f));
	m_PerspectiveCameraController.GetCamera().SetRotation(glm::vec2(3.2f, -451.199f));
	//----------------------------------------------------------------------------------------------------------------------------



	//--CARGA DE SHADERS
	directLight_shadow_mapping_depth_shader = new GLCore::Shader("assets/shaders/directLight_shadow_mapping_depth_shader.vert.glsl", 
															     "assets/shaders/directLight_shadow_mapping_depth_shader.frag.glsl");

	pointLight_shadow_mapping_depth_shader  = new GLCore::Shader("assets/shaders/pointLight_shadow_mapping_depth_shader.vert.glsl", 
																 "assets/shaders/pointLight_shadow_mapping_depth_shader.frag.glsl",
																 "assets/shaders/pointLight_shadow_mapping_depth_shader.gs.glsl");

	postprocessing_shader                    = new GLCore::Shader("assets/shaders/HDR.vert.glsl",
																  "assets/shaders/HDR.frag.glsl");

	debug_shader                             = new GLCore::Shader("assets/shaders/debug_shader.vert.glsl", 
																  "assets/shaders/debug_shader.frag.glsl");


	pbrShader								= new GLCore::Shader("assets/shaders/pbr.vert.glsl",
																 "assets/shaders/pbr.frag.glsl");

	equirectangularToCubemapShader          = new GLCore::Shader("assets/shaders/2.2.2.cubemap.vs", 
																  "assets/shaders/2.2.2.equirectangular_to_cubemap.fs");

	irradianceShader					    = new GLCore::Shader("assets/shaders/2.2.2.cubemap.vs", 
																  "assets/shaders/2.2.2.irradiance_convolution.fs");


	prefilterShader							= new GLCore::Shader("assets/shaders/2.2.2.cubemap.vs",
																  "assets/shaders/2.2.2.prefilter.fs");

	brdfShader                               = new GLCore::Shader("assets/shaders/2.2.2.brdf.vs",
																  "assets/shaders/2.2.2.brdf.fs");

	backgroundShader	                     = new GLCore::Shader("assets/shaders/2.2.2.background.vs", 
																  "assets/shaders/2.2.2.background.fs");
	//----------------------------------------------------------------------------------------------------------------------------
	


	//--GRID WORLD REFERENCE COMPONENT
	gridWorldReference = &manager.addEntity();
	gridWorldReference->name = "GridWorld";
	gridWorldReference->addComponent<GridWorldReferenceComp>();
	gridWorldReference->getComponent<GridWorldReferenceComp>().debugShader = debug_shader;
	//--------------------------------------------------------------------------------------


	//--INIT VIEWPORT SIZE
	viewportHeight = windowHeight;
	viewportWidth = windowWidth - width_dock_Inspector - width_dock_AssetScene;
	//--------------------------------------------------------------------------------------






	pbrShader->use();
	pbrShader->setInt("irradianceMap", 0);
	pbrShader->setInt("prefilterMap", 1);
	pbrShader->setInt("brdfLUT", 2);

	backgroundShader->use();
	backgroundShader->setInt("environmentMap", 0);


	// pbr: setup framebuffer
	// ----------------------
	
	glGenFramebuffers(1, &captureFBO);
	glGenRenderbuffers(1, &captureRBO);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);



	// pbr: load the HDR environment map
	// ---------------------------------
	hdrTexture = GLCore::Loaders::loadHDR("assets/textures/HDR/newport_loft.hdr");




	// pbr: setup cubemap to render to and attach to framebuffer
	// ---------------------------------------------------------
	glGenTextures(1, &envCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


	// pbr: set up projection and view matrices for capturing data onto the 6 cubemap face directions
	// ----------------------------------------------------------------------------------------------
	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	glm::mat4 captureViews[] =
	{
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};

	// pbr: convert HDR equirectangular environment map to cubemap equivalent
	// ----------------------------------------------------------------------
	equirectangularToCubemapShader->use();
	equirectangularToCubemapShader->setInt("equirectangularMap", 0);
	equirectangularToCubemapShader->setMat4("projection", captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, hdrTexture);

	glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	for (unsigned int i = 0; i < 6; ++i)
	{
		equirectangularToCubemapShader->setMat4("view", captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderCube();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// then let OpenGL generate mipmaps from first mip face (combatting visible dots artifact)
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);



	// pbr: create an irradiance cubemap, and re-scale capture FBO to irradiance scale.
	// --------------------------------------------------------------------------------
	glGenTextures(1, &irradianceMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

	// pbr: solve diffuse integral by convolution to create an irradiance (cube)map.
	// -----------------------------------------------------------------------------
	irradianceShader->use();
	irradianceShader->setInt("environmentMap", 0);
	irradianceShader->setMat4("projection", captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

	glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	for (unsigned int i = 0; i < 6; ++i)
	{
		irradianceShader->setMat4("view", captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderCube();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	// pbr: create a pre-filter cubemap, and re-scale capture FBO to pre-filter scale.
	// --------------------------------------------------------------------------------
	
	glGenTextures(1, &prefilterMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // be sure to set minification filter to mip_linear 
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	// pbr: run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map.
	// ----------------------------------------------------------------------------------------------------
	prefilterShader->use();
	prefilterShader->setInt("environmentMap", 0);
	prefilterShader->setMat4("projection", captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	unsigned int maxMipLevels = 5;
	for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
	{
		// reisze framebuffer according to mip-level size.
		unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mip));
		unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
		glViewport(0, 0, mipWidth, mipHeight);

		float roughness = (float)mip / (float)(maxMipLevels - 1);
		prefilterShader->setFloat("roughness", roughness);
		for (unsigned int i = 0; i < 6; ++i)
		{
			prefilterShader->setMat4("view", captureViews[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			renderCube();
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// pbr: generate a 2D LUT from the BRDF equations used.
	// ----------------------------------------------------
	unsigned int brdfLUTTexture;
	glGenTextures(1, &brdfLUTTexture);

	// pre-allocate enough memory for the LUT texture.
	glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
	// be sure to set wrapping mode to GL_CLAMP_TO_EDGE
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

	glViewport(0, 0, 512, 512);
	brdfShader->use();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	renderQuad();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);










	//--POSTPROCCESS FrameBuffer
	glGenFramebuffers(1, &postprocessingFBO);
	// create floating point color buffer
	glGenTextures(1, &colorBuffer);
	glBindTexture(GL_TEXTURE_2D, colorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, viewportWidth, viewportHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// create depth buffer (renderbuffer)
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, viewportWidth, viewportHeight);

	// attach buffers
	glBindFramebuffer(GL_FRAMEBUFFER, postprocessingFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) std::cout << "Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0); //unbind texture
	//------------------------------------------------------------------------------------------------------------

	postprocessing_shader->use();
	postprocessing_shader->setInt("hdrBuffer", 0);




	//loadFileModel("assets/meshes/Tunel/", "Tunel.fbx");


	//--CREAMOS GRID
	/*ModelParent modelParent = {};
	try {
		modelParent = GLCore::Loaders::loadModelAssimp("assets/meshes/grid/", "cell.fbx");
		int gridWidth = 20;
		int gridHeight = 20;

		ECS::Entity* entityGrid = &manager.addEntity();
		entityGrid->name = "THE_GRID";

		for (int i = 0; i < gridHeight; ++i) {
			for (int j = 0; j < gridWidth; ++j) {

				ECS::Entity* entityCell = &manager.addEntity();
							 entityCell->getComponent<Transform>().position = glm::vec3(i,0,j);
							 entityCell->getComponent<Transform>().parent = entityGrid;
							 entityCell->addComponent<Drawable>().setModelInfo(modelParent.modelInfos[0], pbrShader, debug_shader,false);
							 entityCell->name = "cell_" + std::to_string(i) + "_" + std::to_string(j);

				entityGrid->getComponent<Transform>().children.push_back(entityCell);
			}
		}
	}
	catch (std::runtime_error& e) {
		std::cerr << "Error cargando el modelo: " << e.what() << std::endl;
	}*/
	//----------------------------------------------------------------------------------------------------------------------------
}


void TestLayer::generatePointLight()
{
	ECS::Entity* pointLight_entity = &manager.addEntity();
	pointLight_entity->addComponent<PointLight>();
	pointLight_entity->getComponent<PointLight>().setLightValues(glm::vec3(0.0f), pointLights.size(), pbrShader, pointLight_shadow_mapping_depth_shader, debug_shader);
	pointLights.push_back(pointLight_entity);
}

void TestLayer::generateDirectionalLight()
{
	if (directionalLight == nullptr)
	{
		directionalLight = &manager.addEntity();
		directionalLight->addComponent<DirectionalLight>();
		directionalLight->getComponent<DirectionalLight>().setLightValues(glm::vec3(0.0f, 5.0f, 0.0f), 0, pbrShader, debug_shader, directLight_shadow_mapping_depth_shader);
	}
	else 
	{
		std::cerr << "Ya exsite una luz direccional en la escena " << std::endl;
	}
}


void TestLayer::getModelPathFromAssets(const std::string& filePath, const std::string& fileName)
{
	loadFileModel(filePath, fileName);
}

ModelParent TestLayer::loadFileModel(const std::string& filePath, const std::string& fileName)
{
	ModelParent modelParent = {};

	try {
		modelParent = GLCore::Loaders::loadModelAssimp(filePath, fileName);

		ECS::Entity* entityParent = &manager.addEntity();
					 entityParent->name = modelParent.name;
		
		if (modelParent.modelInfos.size() > 1)
		{
			for (int i = 0; i < modelParent.modelInfos.size(); i++)
			{
				ECS::Entity* entityChild = &manager.addEntity();
				entityChild->name = modelParent.modelInfos[i].model_mesh.meshName + std::to_string(i);
				entityChild->getComponent<Transform>().parent = entityParent;
				entityChild->addComponent<Drawable>().setModelInfo(modelParent.modelInfos[i], pbrShader, debug_shader,true);
				entityParent->getComponent<Transform>().children.push_back(entityChild);
			}
		}
		else
		{
			entityParent->addComponent<Drawable>().setModelInfo(modelParent.modelInfos[0], pbrShader, debug_shader,true);
			entityParent->name = modelParent.modelInfos[0].model_mesh.meshName + std::to_string(0);
		}
	}
	catch (std::runtime_error& e) {
		std::cerr << "Error cargando el modelo: " << e.what() << std::endl;
	}


	return modelParent;
}

void TestLayer::OnDetach(){}



void TestLayer::OnEvent(GLCore::Event& event)
{
	if (event.GetEventType() == GLCore::EventType::WindowResize)
	{
		GLCore::WindowResizeEvent& resizeEvent = dynamic_cast<GLCore::WindowResizeEvent&>(event);
		windowWidth = resizeEvent.GetWidth();
		windowHeight = resizeEvent.GetHeight();

		viewportHeight = windowHeight;
		viewportWidth = windowWidth -width_dock_Inspector - width_dock_AssetScene;

		float viewportWidthT = windowWidth -(width_dock_Inspector * 1.5f) - (width_dock_AssetScene * 1.5f);
		float viewportHeightT = windowHeight;
		
		//// Actualiza la textura adjunta al framebuffer personalizado
		//glBindTexture(GL_TEXTURE_2D, colorBuffer);
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, viewportWidthT, viewportHeightT, 0, GL_RGBA, GL_FLOAT, NULL);

		//// Actualiza el objeto de renderbuffer adjunto para profundidad y stencil
		//glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
		//glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, viewportWidthT, viewportHeightT);

		//// Configura el viewport de OpenGL al nuevo tamaño de la ventana
		glViewport(0, 0, viewportWidthT, viewportHeightT);

		//// Es una buena práctica desvincular los objetos OpenGL después de usarlos para evitar efectos secundarios
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}



	if (Input::IsKeyPressed(HZ_KEY_1))
	{
		m_GizmoOperation = GizmoOperation::Translate;
	}
	else if (Input::IsKeyPressed(HZ_KEY_2))
	{
		m_GizmoOperation = GizmoOperation::Rotate3D;
	}
	else if (Input::IsKeyPressed(HZ_KEY_3))
	{
		m_GizmoOperation = GizmoOperation::Scale;
	}

	m_PerspectiveCameraController.OnEvent(event);
}



void TestLayer::OnUpdate(GLCore::Timestep ts)
{
	manager.refresh();
	manager.update();

	entitiesInScene = manager.getAllEntities();

	viewportHeight = windowHeight;
	viewportWidth = windowWidth - width_dock_Inspector - width_dock_AssetScene;


	//-POINTS LIGHT ->SHADOW MAP RENDERING--------------------------------------------------------------------
	pointLight_shadow_mapping_depth_shader->use();
	pointLight_shadow_mapping_depth_shader->setInt("numPointLights", pointLights.size());
	for (unsigned int i = 0; i < pointLights.size(); i++)
	{
		if (pointLights[i]->getComponent<PointLight>().drawShadows == true)
		{
			pointLights[i]->getComponent<PointLight>().shadowMappingProjection(entitiesInScene);
		}
	}
	//----------------------------------------------------------------------------------------------------------------------------


	////-DIRECTIONAL LIGHT ->SHADOW MAP RENDERING--------------------------------------------------------------------
	//if (directionalLight != nullptr)
	//{
	//	if (directionalLight->getComponent<DirectionalLight>().drawShadows == true)
	//	{
	//		directionalLight->getComponent <DirectionalLight>().shadowMappingProjection(entitiesInScene);
	//	}
	//}
	////----------------------------------------------------------------------------------------------------------------------------



	//--SELECT FRAMEBUFFER---------------------------------------------------------------
	if (postpreccessing)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, postprocessingFBO);
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	//----------------------------------------------------------------------------------------------------------------------------




	glViewport(0, 0, viewportWidth, viewportHeight);
	glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	//--CAMERA
	aspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
	m_PerspectiveCameraController.GetCamera().SetProjection(m_PerspectiveCameraController.GetCamera().GetFov(), aspectRatio, 0.1f, 100.0f);
	m_PerspectiveCameraController.OnUpdate(ts);
	//----------------------------------------------------------------------------------------------------------------------------


	//--DEBUG SHADER
	debug_shader->use();
	debug_shader->setMat4("projection", m_PerspectiveCameraController.GetCamera().GetProjectionMatrix());
	debug_shader->setMat4("view", m_PerspectiveCameraController.GetCamera().GetViewMatrix());
	//----------------------------------------------------------------------------------------------------------------------------


	//--MAIN SHADER (PBR)
	pbrShader->use();
	pbrShader->setMat4("projection", m_PerspectiveCameraController.GetCamera().GetProjectionMatrix());
	pbrShader->setMat4("view", m_PerspectiveCameraController.GetCamera().GetViewMatrix());
	pbrShader->setVec3("camPos", m_PerspectiveCameraController.GetCamera().GetPosition());

	pbrShader->setVec3("globalAmbient", globalAmbient);
	//----------------------------------------------------------------------------------------------------------------------------


	////--DRAW DIR LIGHT
	//if (directionalLight != nullptr)
	//{
	//	directionalLight->getComponent<DirectionalLight>().setDataInShader();
	//}
	////----------------------------------------------------------------------------------------------------------------------------

	manager.draw();





	//--DRAW POINTS LIGHT
	pbrShader->use();
	pbrShader->setInt("numPointLights", pointLights.size());
	for (unsigned int i = 0; i < pointLights.size(); i++)//set lighting points uniforms
	{
		pointLights[i]->getComponent<PointLight>().setDataInShader();
	}
	//----------------------------------------------------------------------------------------------------------------------------
	
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	

	pbrShader->setBool("useHDR", useHDRIlumination);
	if (useHDRIlumination == true)
	{
		// render skybox (render as last to prevent overdraw)
		backgroundShader->use();
		backgroundShader->setMat4("view", m_PerspectiveCameraController.GetCamera().GetViewMatrix());
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
		glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap); // display irradiance map
		glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);  // display prefilter map
		renderCube();

		// render BRDF map to screen
		//brdfShader->use();
		//renderQuad();
	}
	
	
	
	

	//--POST-PROCCESSING
	if (postpreccessing)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		postprocessing_shader->use();
		glActiveTexture(GL_TEXTURE8);
		glBindTexture(GL_TEXTURE_2D, colorBuffer);
		
		postprocessing_shader->setInt("hdr", postpreccessing);
		postprocessing_shader->setInt("hdrBuffer", 8);
		postprocessing_shader->setFloat("exposure", exposure);
		postprocessing_shader->setFloat("gammaInput", gammaInput);
		
		glDisable(GL_DEPTH_TEST);
		renderQuad();
		glEnable(GL_DEPTH_TEST);
	}
	// --------------------------------------------------------------------------------------------------------------------------
	

	//--CHECK CLICK DRAWABLES
	CheckIfPointerIsOverObject();
}



void TestLayer::CheckIfPointerIsOverObject()
{
	int mouseX = Input::GetMouseX();
	int mouseY = Input::GetMouseY();

	if (ImGuizmo::IsOver() || ImGuizmo::IsUsing())
	{
		return;
	}


	if (mouseX >= 0 && mouseX <= (0 + viewportWidth) && mouseY >= windowHeight - (viewportWidth / aspectRatio) && mouseY <= (windowHeight - (viewportWidth / aspectRatio)+(viewportWidth / aspectRatio))) {
	}
	else {
		return;
	}



	if (Input::IsMouseButtonReleased(0))
	{
		pickingObj = false;
	}

	// Aquí empieza el raycasting
	if (Input::IsMouseButtonPressed(0))
	{
		if (pickingObj) return; //Si esta bool está a true, retornará, y significa que hemos pulsado ya el mouse y hasta que no soltemos el boton, no se devuelve a false

		if (m_SelectedEntity != nullptr) //Si ya existe un objeto seleccionado y tiene drawable, desactivamos su BB
			if (m_SelectedEntity->hascomponent<Drawable>()) m_SelectedEntity->getComponent<Drawable>().drawLocalBB = false;



		m_SelectedEntity = nullptr; //Reset de la variable que almacena la entity seleccioanda preparandola para recibit o no una nueva selección


		//llevamos un punto 2D a un espacio 3D (mouse position -> escene)
		float normalizedX = (2.0f * Input::GetMouseX()) / viewportWidth - 1.0f;
		float normalizedY = ((2.0f * Input::GetMouseY()) / windowHeight - 1.0f) * -1.0f;
		glm::vec3 clipSpaceCoordinates(normalizedX, normalizedY, 1.0);

		glm::vec4 homogenousCoordinates = glm::inverse(m_PerspectiveCameraController.GetCamera().GetProjectionMatrix() *
			m_PerspectiveCameraController.GetCamera().GetViewMatrix()) * glm::vec4(clipSpaceCoordinates, 1.0);
		glm::vec3 worldCoordinates = glm::vec3(homogenousCoordinates / homogenousCoordinates.w);


		//Preparamos el rayo para lanzarlo desde la camara hasta la posicion del mouse ya convertido al espacio 3D
		glm::vec3 rayOrigin = m_PerspectiveCameraController.GetCamera().GetPosition();
		glm::vec3 rayDirection = glm::normalize(worldCoordinates - rayOrigin);

		//En esta variable se guardará el maximo valor que de un float (a modo de inicializador con valor "infinito") 
		float closestDistance = std::numeric_limits<float>::max();

		//La lista de entidades actual que vamos a comprobar si atraviensan el rayo
		std::vector<ECS::Entity*> entities = manager.getAllEntities();

		//Recorremos la lista de entidades 
		for (int i = 0; i < entities.size(); i++)
		{
			if (entities[i]->hascomponent<Drawable>())
			{
				//Si el rayo atraviesa la BBox de un objeto, entra x aqui
				if (rayIntersectsBoundingBox(rayOrigin, 
											 rayDirection, 
											 entities[i]->getComponent<Drawable>().modelInfo.model_mesh.boundingBoxMin,
										     entities[i]->getComponent<Drawable>().modelInfo.model_mesh.boundingBoxMax))
				{
					//Miramos la distancia del origen del rayo (camara)  al centro de la boundingBox del objeto impactado con el rayo
					float distance = glm::distance(rayOrigin,
												   (entities[i]->getComponent<Drawable>().modelInfo.model_mesh.boundingBoxMin +
													entities[i]->getComponent<Drawable>().modelInfo.model_mesh.boundingBoxMax) * 0.5f);

					//Teniendo en cuenta que un rayo lo mas probable es que atraviese n objetos, con esta condicion nos quedaremos con el mas cercano, comparando la distancia a cada objeto y nos quedamos con el más cercano. 
					if (distance < closestDistance)
					{
						closestDistance = distance;
						m_SelectedEntity = entities[i]; //Al final del bucle, se quedará con el que nos interesa
					}
				}
			}
		}

		if (m_SelectedEntity) //Si hemos obtenido un objeto, revisamos si tiene posibilidad de drawable y en ese caso activamos su BB
			if (m_SelectedEntity->hascomponent<Drawable>()) m_SelectedEntity->getComponent<Drawable>().drawLocalBB = true;


		//Flag para evitar que se vuelva a pasar por esta funcion hasta que se levante el dedo del boton del mouse
		pickingObj = true;
	}
}

bool TestLayer::rayIntersectsBoundingBox(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3 boxMin, glm::vec3 boxMax)
{
	float tMin = (boxMin.x - rayOrigin.x) / rayDirection.x;
	float tMax = (boxMax.x - rayOrigin.x) / rayDirection.x;

	if (tMin > tMax) std::swap(tMin, tMax);

	float tyMin = (boxMin.y - rayOrigin.y) / rayDirection.y;
	float tyMax = (boxMax.y - rayOrigin.y) / rayDirection.y;

	if (tyMin > tyMax) std::swap(tyMin, tyMax);

	if ((tMin > tyMax) || (tyMin > tMax))
		return false;

	if (tyMin > tMin)
		tMin = tyMin;

	if (tyMax < tMax)
		tMax = tyMax;

	float tzMin = (boxMin.z - rayOrigin.z) / rayDirection.z;
	float tzMax = (boxMax.z - rayOrigin.z) / rayDirection.z;

	if (tzMin > tzMax) std::swap(tzMin, tzMax);

	if ((tMin > tzMax) || (tzMin > tMax))
		return false;

	if (tzMin > tMin)
		tMin = tzMin;

	if (tzMax < tMax)
		tMax = tzMax;

	return true;
}
void TestLayer::renderQuad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};

		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}

	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}





// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------

void TestLayer::renderCube()
{
	// initialize (if necessary)
	if (cubeVAO == 0)
	{
		float vertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			 // bottom face
			 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			  1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			 -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			 // top face
			 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			  1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			  1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			  1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			 -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};
		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(cubeVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

std::string TestLayer::ws2s(const std::wstring& wide)
{
	int len = WideCharToMultiByte(CP_ACP, 0, wide.c_str(), wide.size(), NULL, 0, NULL, NULL);
	std::string narrow(len, ' ');
	WideCharToMultiByte(CP_ACP, 0, wide.c_str(), wide.size(), &narrow[0], len, NULL, NULL);
	return narrow;
}

void TestLayer::OnImGuiRender()
{
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

	// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
	// because it would be confusing to have two docking targets within each others.
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;


	// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
	if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
	{
		window_flags |= ImGuiWindowFlags_NoBackground;
	}

	// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
	// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
	// all active windows docked into it will lose their parent and become undocked.
	// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
	// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace", nullptr, window_flags);
	ImGui::PopStyleVar();
	ImGui::PopStyleVar(2);


	// DockSpace
	ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);


	static auto first_time = true;
	if (first_time && !ini_file_exists)
	{
		std::cout << "First time GUI" << std::endl;
		first_time = false;

		//ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockBuilderRemoveNode(dockspace_id);
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetIO().DisplaySize);

		ImGuiID dock_main = dockspace_id;
		//dock_id_left = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left, 0.7f, nullptr, &dock_main);
		dock_id_Inspector  = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.2f, nullptr, &dock_main);
		dock_id_AssetScene = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.2f, nullptr, &dock_main);
		dock_id_up = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Up, 0.065f, nullptr, &dock_main);


		ImGui::DockBuilderDockWindow("Dock_INSPECTOR", dock_id_Inspector);
		ImGui::DockBuilderDockWindow("Dock_ASSETS_SCENE", dock_id_AssetScene);
		ImGui::DockBuilderDockWindow("Dock_Top", dock_id_up);

		ImGui::DockBuilderFinish(dockspace_id);
	}

	//---------------------------ImGUIZMO------------------------------------------
	if (m_SelectedEntity != nullptr)
	{
		if (m_SelectedEntity->hascomponent<Transform>())
		{
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(0, 0, viewportWidth, viewportHeight);

			glm::mat4 camera_view = m_PerspectiveCameraController.GetCamera().GetViewMatrix();
			glm::mat4 camera_projection = m_PerspectiveCameraController.GetCamera().GetProjectionMatrix();

			glm::mat4 entity_transform = m_SelectedEntity->getComponent<Transform>().GetTransform();

			// Comprobación de parentesco
			if (m_SelectedEntity->getComponent<Transform>().parent != nullptr) {
				entity_transform = m_SelectedEntity->getComponent<Transform>().parent->getComponent<Transform>().GetTransform() * entity_transform;
			}

			switch (m_GizmoOperation)
			{
			case GizmoOperation::Translate:
				ImGuizmo::Manipulate(glm::value_ptr(camera_view), glm::value_ptr(camera_projection),
					ImGuizmo::TRANSLATE, ImGuizmo::LOCAL, glm::value_ptr(entity_transform));
				break;
			case GizmoOperation::Rotate3D:
				ImGuizmo::Manipulate(glm::value_ptr(camera_view), glm::value_ptr(camera_projection),
					ImGuizmo::ROTATE, ImGuizmo::LOCAL, glm::value_ptr(entity_transform));
				break;
			case GizmoOperation::Scale:
				ImGuizmo::Manipulate(glm::value_ptr(camera_view), glm::value_ptr(camera_projection),
					ImGuizmo::SCALE, ImGuizmo::LOCAL, glm::value_ptr(entity_transform));
				break;
			}

			if (ImGuizmo::IsUsing())
			{
				glm::vec3 translation, rotation, scale, skew;
				glm::quat orientation;
				glm::vec4 perspective;

				glm::decompose(entity_transform, scale, orientation, translation, skew, perspective);

				// Cálculo de la transformación local
				if (m_SelectedEntity->getComponent<Transform>().parent != nullptr) {
					glm::mat4 parent_transform = m_SelectedEntity->getComponent<Transform>().parent->getComponent<Transform>().GetTransform();
					glm::mat4 local_transform = glm::inverse(parent_transform) * entity_transform;
					glm::decompose(local_transform, scale, orientation, translation, skew, perspective);
				}

				m_SelectedEntity->getComponent<Transform>().rotation = orientation;
				m_SelectedEntity->getComponent<Transform>().position = translation;
				m_SelectedEntity->getComponent<Transform>().scale = scale;
			}
		}
	}


	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			ImGui::MenuItem("SANDBOX", NULL, false, false);
			if (ImGui::MenuItem("New"))
			{
				
			}

			if (ImGui::MenuItem("Open", "Ctrl+O"))
			{
				
			}

			if (ImGui::MenuItem("Save", "Ctrl+S"))
			{
				

			}
			if (ImGui::MenuItem("Save As..")) {}

			ImGui::Separator();

			if (ImGui::BeginMenu("Options"))
			{
				static bool b = true;
				ImGui::Checkbox("Auto Save", &b);
				ImGui::EndMenu();
			}

			if (ImGui::MenuItem("Quit", "Alt+F4")) {}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit")) { ImGui::EndMenu(); }
		if (ImGui::BeginMenu("Project")) { ImGui::EndMenu(); }
		if (ImGui::BeginMenu("View")) { ImGui::EndMenu(); }
		if (ImGui::BeginMenu("GameObjects")) 
		{ 
			if (ImGui::Button("Add Direcctional Light"))
			{
				generateDirectionalLight();
			}
			if (ImGui::Button("Add Point Light"))
			{
				generatePointLight();
			}

			ImGui::EndMenu(); 
		}
		if (ImGui::BeginMenu("Tools")) { ImGui::EndMenu(); }
		if (ImGui::BeginMenu("About")) { ImGui::EndMenu(); }
		ImGui::EndMenuBar();
	}
	ImGui::End();


	ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);





	//-------------------------------------------DOCK UP--------------------------------------------
	ImGui::SetNextWindowDockID(dock_id_up, ImGuiCond_FirstUseEver);
	ImGui::Begin("Panel TOP", nullptr);
	ImGui::Text("CLEAR COLOR");
	if (ImGui::ColorEdit4("", glm::value_ptr(backgroundColor))) {};

	ImGui::ColorEdit3("Iluminación Ambiental", (float*)&globalAmbient);
	ImGui::Checkbox("HDR ILUMINATION", &useHDRIlumination);
	ImGui::Checkbox("HDR", &postpreccessing);
	ImGui::SliderFloat("Exposure", &exposure, 0.0f, 20.0f);
	ImGui::SliderFloat("Gamma"   , &gammaInput, 0.0f, 20.0f);

	//MAIN CAMERA
	//ImGui::Text("Camera");
	//// Obtiene la posición actual de la cámara
	//glm::vec3 position = m_PerspectiveCameraController.GetCamera().GetPosition();
	//// Muestra y permite modificar la posición de la cámara
	//if (ImGui::DragFloat3("Position", &position[0], 0.01f))
	//{
	//	m_PerspectiveCameraController.GetCamera().SetPosition(position);
	//}

	//// Utiliza DragFloat2 para la rotación en vez de DragFloat3
	//glm::vec2 rotation = m_PerspectiveCameraController.GetCamera().GetRotation();
	//if (ImGui::DragFloat2("Rotation", &rotation[0], 0.1f))
	//{
	//	m_PerspectiveCameraController.GetCamera().SetRotation(rotation);
	//}

	//// Ahora puedes obtener y modificar el FOV con las nuevas funciones
	//float fov = m_PerspectiveCameraController.GetCamera().GetFov();
	//if (ImGui::SliderFloat("Fov", &fov, 1.0f, 180.0f))
	//{
	//	m_PerspectiveCameraController.GetCamera().SetFov(fov);
	//}
	ImGui::End();
	//------------------------------------------------------------------------------------------------



	//ImGui::Begin("FrameBuffer"); // Crea una nueva ventana de ImGui.
	//ImGui::Image((void*)(intptr_t)irradianceMap, ImVec2(512, 512));
	//ImGui::End(); // Finaliza la ventana de ImGui.



	//-------------------------------------------PANEL _DERECHO--------------------------------------
	ImGui::SetNextWindowDockID(dock_id_AssetScene, ImGuiCond_FirstUseEver);
	ImGui::Begin("Scene", nullptr);
	std::vector<ECS::Entity*> entities = manager.getAllEntities();
	for (int i = 0; i < entities.size(); i++)
	{
		if (entities[i]->getComponent<Transform>().parent == nullptr) // Only root entities
		{
			ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
			if (m_SelectedEntity == entities[i])
				node_flags |= ImGuiTreeNodeFlags_Selected;

			std::string treeLabel = entities[i]->name;

			bool treeOpen = ImGui::TreeNodeEx((void*)(intptr_t)i, node_flags, treeLabel.c_str());
			if (ImGui::IsItemClicked()) // Select on (left) click
			{
				if (m_SelectedEntity != nullptr)
				{
					if (m_SelectedEntity->hascomponent<Drawable>())
					{
						m_SelectedEntity->getComponent<Drawable>().drawLocalBB = false;
					}
				}
				m_SelectedEntity = entities[i];
			}

			if (treeOpen)
			{
				for (int j = 0; j < entities[i]->getComponent<Transform>().children.size(); j++)
				{
					auto& child = entities[i]->getComponent<Transform>().children[j];

					ImGuiTreeNodeFlags child_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
					if (m_SelectedEntity == child)
						child_flags |= ImGuiTreeNodeFlags_Selected;

					std::string childLabel = child->name;

					ImGui::TreeNodeEx((void*)(intptr_t)(i * 1000 + j), child_flags, childLabel.c_str()); // Ensure the ID is unique
					if (ImGui::IsItemClicked())
					{
						if (m_SelectedEntity)
						{
							if (m_SelectedEntity->hascomponent<Drawable>())
							{
								m_SelectedEntity->getComponent<Drawable>().drawLocalBB = false;
							}
						}
						m_SelectedEntity = child;
					}
				}
				ImGui::TreePop();
			}
		}
	}
	ImGui::End();








	ImGui::SetNextWindowDockID(dock_id_Inspector, ImGuiCond_FirstUseEver);
	ImGui::Begin("Inspector", nullptr);
	if (m_SelectedEntity != nullptr)
	{
		//---------------------------GRID REFERENCE COMPONENT------------------------------------------
		if (m_SelectedEntity != nullptr && m_SelectedEntity->hascomponent<GridWorldReferenceComp>())
		{
			ImGui::Text("Grid Reference");
			GridWorldReferenceComp* gridWorldReference = &m_SelectedEntity->getComponent<GridWorldReferenceComp>();

			if (gridWorldReference != nullptr)
			{
				ImGui::Checkbox("Active", &gridWorldReference->showGrid);
			}
			ImGui::Dummy(ImVec2(0.0f, 20.0f));
		}
		//-----------------------------------------------------------------------------------------


		//---------------------------TRANSFORM COMPONENT------------------------------------------
		if (m_SelectedEntity != nullptr && m_SelectedEntity->hascomponent<Transform>())
		{
			ImGui::Text("Transform");
			Transform* transform = &m_SelectedEntity->getComponent<Transform>();

			if (transform != nullptr)
			{
				ImGui::DragFloat3("Position", glm::value_ptr(transform->position), 0.01f);
				glm::vec3 eulers = glm::degrees(glm::eulerAngles(transform->rotation));
				if (ImGui::DragFloat3("Rotation", glm::value_ptr(eulers), 0.01f)) {
					transform->rotation = glm::quat(glm::radians(eulers));
				}
				ImGui::DragFloat3("Scale", glm::value_ptr(transform->scale), 0.01f);
			}

			ImGui::Separator();

			if (ImGui::Button("Delete"))
			{
				m_SelectedEntity->active = false;
				m_SelectedEntity = nullptr;
			}

			ImGui::Dummy(ImVec2(0.0f, 10.0f));
		}
		//-----------------------------------------------------------------------------------------





		//---------------------------DRAWABLE COMPONENT------------------------------------------
		if (m_SelectedEntity != nullptr && m_SelectedEntity->hascomponent<Drawable>())
		{
			m_SelectedEntity->getComponent<Drawable>().drawLocalBB = true;

			ImGui::Text("Drawable");
			Drawable* drawable = &m_SelectedEntity->getComponent<Drawable>();
			if (drawable != nullptr)
			{
				ImGui::Checkbox("Visible Model", &drawable->visibleModel);
				ImGui::Checkbox("Draw BBox", &drawable->drawLocalBB);
				ImGui::Checkbox("Drop Shadow", &drawable->dropShadow);

				ImGui::Separator();
				ImGui::Dummy(ImVec2(0.0f, 10.0f));

				ImGui::Text("Materials");
				ImGui::Dummy(ImVec2(10.0f, 10.0f));
				

				ImGui::Text("Albedo map");
				ImGui::Text(drawable->modelInfo.model_material.albedoMap.image.path.c_str());
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_DRAG"))
					{
						if (!ImGui::IsMouseDown(0))  // Chequeo si el botón izquierdo del ratón se ha liberado
						{
							std::string dropped_fpath = (const char*)payload->Data;
							std::cout << dropped_fpath << std::endl;
							Texture textureLoaded;
							textureLoaded.image = GLCore::Loaders::getOrLoadImage("assets/" + dropped_fpath);
							textureLoaded.image.path = "assets/" + dropped_fpath;
							drawable->modelInfo.model_material.albedoMap = textureLoaded;
							drawable->modelInfo.model_material.PrepareMaterial();
						}
					}
					ImGui::EndDragDropTarget();
				}
				ImGui::Image((void*)(intptr_t)drawable->modelInfo.model_material.albedoMap.textureID, ImVec2(128, 128), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255));
				ImGui::ColorEdit3("Albedo color", glm::value_ptr(drawable->modelInfo.model_material.albedoColor));
				ImGui::Dummy(ImVec2(10.0f, 10.0f));
				



				ImGui::Text("Normal map");
				ImGui::Text(drawable->modelInfo.model_material.normalMap.image.path.c_str());
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_DRAG"))
					{
						if (!ImGui::IsMouseDown(0))  // Chequeo si el botón izquierdo del ratón se ha liberado
						{
							std::string dropped_fpath = (const char*)payload->Data;
							std::cout << dropped_fpath << std::endl;
							Texture textureLoaded;
							textureLoaded.image = GLCore::Loaders::getOrLoadImage("assets/" + dropped_fpath);
							textureLoaded.image.path = "assets/" + dropped_fpath;
							drawable->modelInfo.model_material.normalMap = textureLoaded;
							drawable->modelInfo.model_material.PrepareMaterial();
						}
					}
					ImGui::EndDragDropTarget();
				}
				ImGui::SliderFloat("Normal Intensity", &drawable->modelInfo.model_material.normalIntensity, 0.0f, 10.0f);
				ImGui::Image((void*)(intptr_t)drawable->modelInfo.model_material.normalMap.textureID, ImVec2(128, 128), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255));
				ImGui::Dummy(ImVec2(10.0f, 10.0f));
				



				ImGui::Text("Metallic map");
				ImGui::Text(drawable->modelInfo.model_material.metallicMap.image.path.c_str());
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_DRAG"))
					{
						if (!ImGui::IsMouseDown(0))  // Chequeo si el botón izquierdo del ratón se ha liberado
						{
							std::string dropped_fpath = (const char*)payload->Data;
							std::cout << dropped_fpath << std::endl;
							Texture textureLoaded;
							textureLoaded.image = GLCore::Loaders::getOrLoadImage("assets/" + dropped_fpath);
							textureLoaded.image.path = "assets/" + dropped_fpath;
							drawable->modelInfo.model_material.metallicMap = textureLoaded;
							drawable->modelInfo.model_material.PrepareMaterial();
						}
					}
					ImGui::EndDragDropTarget();

				}
				ImGui::SliderFloat("Metallic", &drawable->modelInfo.model_material.metallicValue, 0.0f, 50.0f);
				ImGui::SliderFloat("F0", &drawable->modelInfo.model_material.reflectanceValue, 0.0f, 1.0f, "%.2f");
				ImGui::SliderFloat("Fresnel Co", &drawable->modelInfo.model_material.fresnelCoefValue, 0.0f, 10.0f, "%.2f");
				ImGui::Image((void*)(intptr_t)drawable->modelInfo.model_material.metallicMap.textureID, ImVec2(128, 128), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255));
				ImGui::Dummy(ImVec2(10.0f, 10.0f));



				ImGui::Text("Roughtness map");
				ImGui::Text(drawable->modelInfo.model_material.rougnessMap.image.path.c_str());
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_DRAG"))
					{
						if (!ImGui::IsMouseDown(0))  // Chequeo si el botón izquierdo del ratón se ha liberado
						{
							std::string dropped_fpath = (const char*)payload->Data;
							std::cout << dropped_fpath << std::endl;
							Texture textureLoaded;
							textureLoaded.image = GLCore::Loaders::getOrLoadImage("assets/" + dropped_fpath);
							textureLoaded.image.path = "assets/" + dropped_fpath;
							drawable->modelInfo.model_material.rougnessMap = textureLoaded;
							drawable->modelInfo.model_material.PrepareMaterial();
						}
					}
					ImGui::EndDragDropTarget();
				}
				ImGui::SliderFloat("Roughtness", &drawable->modelInfo.model_material.roughnessValue, 0.05f, 1.0f);
				ImGui::Image((void*)(intptr_t)drawable->modelInfo.model_material.rougnessMap.textureID, ImVec2(128, 128), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255));
				ImGui::Dummy(ImVec2(10.0f, 10.0f));




				ImGui::Text("AO map");
				ImGui::Text(drawable->modelInfo.model_material.aOMap.image.path.c_str());
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_DRAG"))
					{
						if (!ImGui::IsMouseDown(0))  // Chequeo si el botón izquierdo del ratón se ha liberado
						{
							std::string dropped_fpath = (const char*)payload->Data;
							std::cout << dropped_fpath << std::endl;
							Texture textureLoaded;
							textureLoaded.image = GLCore::Loaders::getOrLoadImage("assets/" + dropped_fpath);
							textureLoaded.image.path = "assets/" + dropped_fpath;
							drawable->modelInfo.model_material.aOMap = textureLoaded;
							drawable->modelInfo.model_material.PrepareMaterial();
						}
					}
					ImGui::EndDragDropTarget();
				}
				//ImGui::SliderFloat("Shiniess", &drawable->modelInfo.model_material.metallicValue, 0.1f, 50.0f);
				ImGui::Image((void*)(intptr_t)drawable->modelInfo.model_material.aOMap.textureID, ImVec2(128, 128), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255));
			}
			ImGui::Dummy(ImVec2(0.0f, 20.0f));
		}
		//-----------------------------------------------------------------------------------------





		//---------------------------DIRLIGHT COMPONENT------------------------------------------
		if (m_SelectedEntity != nullptr && m_SelectedEntity->hascomponent<DirectionalLight>())
		{
			DirectionalLight* dirLight = &m_SelectedEntity->getComponent<DirectionalLight>();
			if (dirLight != nullptr)
			{
				ImGui::Text("Directional Light");
				ImGui::Checkbox("Active", &dirLight->active);
				ImGui::DragFloat3("Direction", glm::value_ptr(dirLight->direction), 0.1f);

				ImGui::Dummy(ImVec2(0.0f, 20.0f));
				ImGui::ColorEdit3("Ambient", glm::value_ptr(dirLight->ambient));
				ImGui::ColorEdit3("Diffuse", glm::value_ptr(dirLight->diffuse));
				ImGui::ColorEdit3("Specular", glm::value_ptr(dirLight->specular));
				ImGui::SliderFloat("Specular Power", &dirLight->specularPower, 0.0f, 100.0f);
				ImGui::Checkbox("Blinn-Phong Shading", &dirLight->blinn);

				ImGui::Dummy(ImVec2(0.0f, 20.0f));
				ImGui::Checkbox("Draw Shadows", &dirLight->drawShadows);
				if (dirLight->drawShadows == true)
				{
					ImGui::SliderFloat("Shadow Intensity", &dirLight->shadowIntensity, 0.0f, 1.0f);
					ImGui::Checkbox("Use Poison Disk", &dirLight->usePoisonDisk);

					ImGui::Dummy(ImVec2(0.0f, 20.0f));
					ImGui::Image((void*)(intptr_t)dirLight->shadowTex, ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0), ImColor(255, 255, 255, 255));

					ImGui::Dummy(ImVec2(0.0f, 20.0f));
					ImGui::SliderFloat("Left", &dirLight->orthoLeft, -100.0f, 100.0f);
					ImGui::SliderFloat("Right", &dirLight->orthoRight, -100.0f, 100.0f);
					ImGui::SliderFloat("Top", &dirLight->orthoTop, -100.0f, 100.0f);
					ImGui::SliderFloat("Bottom", &dirLight->orthoBottom, -100.0f, 100.0f);
					
					ImGui::SliderFloat("Near", &dirLight->orthoNear, 0.0f, 10.0f);
					ImGui::SliderFloat("Far", &dirLight->orthoFar, 0.0f, 200.0f);
				}
				
			}
			ImGui::Dummy(ImVec2(0.0f, 20.0f));
		}
		//-----------------------------------------------------------------------------------------





		//---------------------------POINT LIGHT COMPONENT------------------------------------------
		if (m_SelectedEntity != nullptr && m_SelectedEntity->hascomponent<PointLight>())
		{
			ImGui::Text("PointLight");
			PointLight* pointLight = &m_SelectedEntity->getComponent<PointLight>();

			if (pointLight != nullptr)
			{
				ImGui::Checkbox("Active", &pointLight->active);
				ImGui::Checkbox("Draw Sphere", &pointLight->drawSphereDebug);
				ImGui::Checkbox("Draw Range", &pointLight->showRange);

				ImGui::Dummy(ImVec2(0.0f, 5.0f));
				ImGui::ColorEdit3("Ambient", glm::value_ptr(pointLight->ambient));
				ImGui::ColorEdit3("Diffuse", glm::value_ptr(pointLight->diffuse));
				ImGui::ColorEdit3("Specular", glm::value_ptr(pointLight->specular));

				ImGui::SliderFloat("Specular Power", &pointLight->specularPower, 0.0f, 100.0f);
				ImGui::Checkbox("Blinn-Phong", &pointLight->blinn);

				ImGui::Dummy(ImVec2(0.0f, 5.0f));
				ImGui::SliderFloat("Strength", &pointLight->strength, 0.0f, 1000.0f);
				ImGui::SliderFloat("Range", &pointLight->range, 0.0f, 100.0f);
				ImGui::SliderFloat("Smoothness", &pointLight->lightSmoothness, 0.0f, 1.0f);

				ImGui::Dummy(ImVec2(0.0f, 5.0f));
				const char* items[] = { "No Attenuation", "Linear", "Cuadratic" };
				if (ImGui::Combo("Attenuation Type", &pointLight->currentAttenuation, items, IM_ARRAYSIZE(items)))
				{
					switch (pointLight->currentAttenuation)
					{
					case 0: // Sin atenuación
						pointLight->constant = 1.0f;
						pointLight->linear = 0.0f;
						pointLight->quadratic = 0.0f;
						break;

					case 1: // Atenuación lineal
						pointLight->constant = 1.0f;
						pointLight->linear = 0.09f;
						pointLight->quadratic = 0.032f;
						break;

					case 2: // Atenuación cuadrática
						pointLight->constant = 1.0f;
						pointLight->linear = 0.07f;
						pointLight->quadratic = 0.017f;
						break;
					}
				}

				ImGui::SliderFloat("Constant", &pointLight->constant, 0.0f, 1.0f);
				ImGui::SliderFloat("Linear", &pointLight->linear, 0.0f, 1.0f);
				ImGui::SliderFloat("Quadratic", &pointLight->quadratic, 0.0f, 1.0f);


				ImGui::Checkbox("Draw shadows", &pointLight->drawShadows);


				ImGui::Dummy(ImVec2(0.0f, 20.0f));
				//ImGui::Image((void*)(intptr_t) pointLight->depthMapFBO, ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0), ImColor(255, 255, 255, 255));
			}
			ImGui::Dummy(ImVec2(0.0f, 20.0f));
		}
		//-----------------------------------------------------------------------------------------


	}
	ImGui::End();
	//------------------------------------------------------------------------------------------------




	//-------------------------------------------PANEL ASSETS--------------------------------------------
	ImGui::SetNextWindowDockID(dock_id_AssetScene, ImGuiCond_FirstUseEver);
	assetsPanel.OnImGuiRender();
	//ImGui::Begin("Assets", nullptr);

	//ImGui::BeginChild("FileRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));

	//if (m_CurrentDirectory != std::filesystem::path(s_AssetPath))
	//{
	//	if (ImGui::Button("<-"))
	//	{
	//		m_CurrentDirectory = m_CurrentDirectory.parent_path();
	//	}
	//}

	//static float padding = 0.0f;
	//static float thumbnailSize = 64.0f;
	//float cellSize = thumbnailSize + padding;

	//float panelWidth = ImGui::GetContentRegionAvail().x;
	//int columnCount = thumbnailSize < 40 ? 1 : (int)(panelWidth / cellSize);

	//if (columnCount < 1)
	//	columnCount = 1;

	//ImGui::Columns(columnCount, 0, false);





	//for (auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
	//{
	//	const auto& path = directoryEntry.path();
	//	auto relativePath = std::filesystem::relative(path, s_AssetPath);
	//	std::string filenameString = path.filename().string();

	//	GLuint iconTexture = 0; // Reemplazar con la ID de tu textura real


	//	if (directoryEntry.is_directory())
	//	{
	//		iconTexture = folderTexture; // Asumiendo que folderTexture es la ID de tu textura para carpetas
	//	}
	//	else if (path.extension() == ".fbx" || path.extension() == ".obj" || path.extension() == ".gltf")
	//	{
	//		iconTexture = modelTexture; // Asumiendo que modelTexture es la ID de tu textura para modelos
	//	}
	//	else if (path.extension() == ".png" || path.extension() == ".jpg")
	//	{
	//		iconTexture = imageTexture; // Asumiendo que imageTexture es la ID de tu textura para imágenes

	//		// Carga la textura desde el archivo
	//		//GLuint texture = GLCore::Loaders::LoadIconTexture(filenameString.c_str());
	//		//ImGui::ImageButton((ImTextureID)(intptr_t)texture, ImVec2(thumbnailSize, thumbnailSize));
	//		//iconTexture = texture; // Asumiendo que imageTexture es la ID de tu textura para imágenes
	//	}

	//	if (iconTexture != 0)
	//	{
	//		ImGui::ImageButton((ImTextureID)(intptr_t)iconTexture, ImVec2(thumbnailSize - 15, thumbnailSize-15));
	//	}
	//	else
	//	{
	//		ImGui::Button(filenameString.c_str(), { thumbnailSize, thumbnailSize });
	//	}

	//	if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
	//	{
	//		if (directoryEntry.is_directory())
	//		{
	//			m_CurrentDirectory /= path.filename();
	//		}
	//		else
	//		{
	//			std::filesystem::path fullPath = directoryEntry.path();
	//			std::string filePath = fullPath.parent_path().string();
	//			std::string fileName = fullPath.filename().string();

	//			// Agrega una barra al final de la ruta si no está ya presente
	//			if (filePath.back() != '\\')
	//			{
	//				filePath += '\\';
	//			}

	//			std::cout << "Selected file: " << fullPath << std::endl;
	//			std::cout << "File path: " << filePath << std::endl;
	//			std::cout << "File name: " << fileName << std::endl;

	//			ModelParent modelParent = loadFileModel(filePath, fileName);
	//		}
	//	};




	//	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
	//	{
	//		const string payload_n = filenameString.c_str();
	//		// Puedes enviar lo que quieras, en este caso, la ruta completa del archivo
	//		//const std::string payload_n = relativePath.string();
	//		//ImGui::SetDragDropPayload("ASSET_DRAG", payload_n, wcslen(payload_n) * sizeof(string), ImGuiCond_Once);

	//		// Esto es lo que se muestra cuando estás arrastrando
	//		//ImGui::Text("Arrastrar %s", ws2s(payload_n));
	//		std::cout << payload_n << std::endl;
	//		ImGui::EndDragDropSource();
	//	}


	//	ImGui::Text(filenameString.c_str());
	//	ImGui::NextColumn();
	//}

	//ImGui::Columns(1);

	//ImGui::EndChild();
	//ImGui::SliderFloat("Thumbnail Size", &thumbnailSize, 16, 512);
	//ImGui::End();
	//-----------------------------------------------------------------------------------------------------














	////-------------------------------------------PANEL ASSETS--------------------------------------------
	//ImGui::SetNextWindowDockID(dock_id_AssetScene, ImGuiCond_FirstUseEver);
	//ImGui::Begin("Assets", nullptr);

	//ImGui::BeginChild("FileRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));

	//if (m_CurrentDirectory != std::filesystem::path(s_AssetPath))
	//{
	//	if (ImGui::Button("<-"))
	//	{
	//		m_CurrentDirectory = m_CurrentDirectory.parent_path();
	//	}
	//}

	//static float padding = 0.0f;
	//static float thumbnailSize = 64.0f;
	//float cellSize = thumbnailSize + padding;

	//float panelWidth = ImGui::GetContentRegionAvail().x;

	//int columnCount;

	//if (thumbnailSize < 40)
	//{
	//	columnCount = 1;  // Si el tamaño es menor de 40, se usa una sola columna
	//}
	//else
	//{
	//	columnCount = (int)(panelWidth / cellSize);
	//	if (columnCount < 1)
	//		columnCount = 1;
	//}

	//ImGui::Columns(columnCount, 0, false);

	//for (auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
	//{
	//	const auto& path = directoryEntry.path();

	//	if (directoryEntry.is_directory() || (path.extension() == ".fbx" || path.extension() == ".obj" || path.extension() == ".gltf" || path.extension() == ".png" || path.extension() == ".jpg"))
	//	{
	//		auto relativePath = std::filesystem::relative(path, s_AssetPath);
	//		std::string filenameString = path.filename().string();

	//		ImGui::Button(filenameString.c_str(), { thumbnailSize, thumbnailSize });







	//		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
	//		{
	//			// Puedes enviar lo que quieras, en este caso, la ruta completa del archivo
	//			const std::string payload_n = path.string();
	//			ImGui::SetDragDropPayload("ASSET_DRAG", payload_n.c_str(), payload_n.size() + 1, ImGuiCond_Once);

	//			// Esto es lo que se muestra cuando estás arrastrando
	//			ImGui::Text("Arrastrar %s", filenameString.c_str());
	//			ImGui::EndDragDropSource();
	//		}



	//		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
	//		{
	//			if (directoryEntry.is_directory())
	//			{
	//				m_CurrentDirectory /= path.filename();
	//			}
	//			else
	//			{
	//				std::filesystem::path fullPath = directoryEntry.path();
	//				std::string filePath = fullPath.parent_path().string();
	//				std::string fileName = fullPath.filename().string();

	//				// Agrega una barra al final de la ruta si no está ya presente
	//				if (filePath.back() != '\\')
	//				{
	//					filePath += '\\';
	//				}

	//				std::cout << "Selected file: " << fullPath << std::endl;
	//				std::cout << "File path: " << filePath << std::endl;
	//				std::cout << "File name: " << fileName << std::endl;

	//				ModelParent modelParent = loadFileModel(filePath, fileName);
	//			}
	//		};

	//		ImGui::Text(filenameString.c_str());
	//		ImGui::NextColumn();
	//	}
	//}

	//ImGui::Columns(1);

	//ImGui::EndChild();

	//ImGui::SliderFloat("Thumbnail Size", &thumbnailSize, 16, 512);
	//ImGui::End();
	////------------------------------------------------------------------------------------------------







	





	dock_id_Inspector = 0x00000002;
	// Recuperar el nodo de docking usando el ImGuiID
	ImGuiDockNode* node_dock_id_inspector = ImGui::DockBuilderGetNode(dock_id_Inspector);

	if (node_dock_id_inspector) {
		// Ahora puedes obtener la información del nodo de docking
		ImVec2 size = node_dock_id_inspector->Size;
		width_dock_Inspector = size.x;
	}


	dock_id_AssetScene = 0x00000004;
	// Recuperar el nodo de docking usando el ImGuiID
	ImGuiDockNode* node_dock_id_AssetScene = ImGui::DockBuilderGetNode(dock_id_AssetScene);

	if (node_dock_id_AssetScene) {
		// Ahora puedes obtener la información del nodo de docking
		ImVec2 size = node_dock_id_AssetScene->Size;
		width_dock_AssetScene = size.x;
	}
}



