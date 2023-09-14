#pragma once

#include <GLCore.h>
#include <GLCoreUtils.h>
#include <GLCoreLoaders.h>
#include <ECSCore.h>
#include <filesystem>
#include "ImGUI/AssetsPanel.h"

#include <chrono>

class TestLayer : public GLCore::Layer
{

public:
	TestLayer();
	virtual ~TestLayer();

	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnEvent(GLCore::Event& event) override;
	virtual void OnUpdate(GLCore::Timestep ts) override;
	virtual void OnImGuiRender() override;


	void getModelPathFromAssets(const std::string& filePath, const std::string& fileName);

private:

	enum class GizmoOperation
	{
		Translate,
		Rotate2D, Rotate3D,
		Scale
	};

	ECS::Entity* gridWorldReference = nullptr;

	GizmoOperation m_GizmoOperation;

	// Tamaño de la ventana actual
	int windowWidth = 1280, windowHeight = 720;

	ECS::Manager manager;

	GLCore::Utils::PerspectiveCameraController m_PerspectiveCameraController;

	ECS::Entity* directionalLight = nullptr;
	vec3 globalAmbient = glm::vec3(0.0f);


	bool useHDRIlumination = false;

	vector<ECS::Entity*> pointLights;

	
	GLCore::Shader* postprocessing_shader = nullptr;
	GLCore::Shader* directLight_shadow_mapping_depth_shader = nullptr;
	GLCore::Shader* pointLight_shadow_mapping_depth_shader = nullptr;
	GLCore::Shader* debug_shader = nullptr;
	GLCore::Shader* screen_shader = nullptr;

	GLCore::Shader* pbrShader = nullptr;
	GLCore::Shader* equirectangularToCubemapShader = nullptr;
	GLCore::Shader* irradianceShader = nullptr;
	GLCore::Shader* prefilterShader = nullptr;
	GLCore::Shader* brdfShader = nullptr;
	GLCore::Shader* backgroundShader = nullptr;

	unsigned int cubeVAO = 0;
	unsigned int cubeVBO = 0;

	unsigned int quadVAO = 0;
	unsigned int quadVBO;


	//POST-PROCESSING CONTROLS
	bool postpreccessing = false;
	float exposure = 1.0f;
	float gammaInput = 2.2f;
	unsigned int postprocessingFBO;
	unsigned int colorBuffer;
	unsigned int rboDepth;




	std::vector<ECS::Entity*> entitiesInScene;
	ECS::Entity* m_SelectedEntity = nullptr;


	glm::vec4 backgroundColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);


	bool pickingObj = false;
	void CheckIfPointerIsOverObject();
	bool rayIntersectsBoundingBox(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3 boxMin, glm::vec3 boxMax);


	std::filesystem::path m_BaseDirectory;
	std::filesystem::path m_CurrentDirectory;


	GLCore::ModelParent loadFileModel(const std::string& filePath, const std::string& fileName);


	//GUI
	float aspectRatio;
	float viewportX;
	float viewportY;
	float viewportWidth;
	float viewportHeight;

	

	bool ini_file_exists;

	ImGuiID dock_id_Inspector;
	ImGuiID dock_id_AssetScene;
	ImGuiID dock_id_up;


	float width_dock_Inspector = 0;
	float width_dock_AssetScene = 0;
	float height_dock_right = 0;

	unsigned int captureFBO;
	unsigned int captureRBO;
	unsigned int envCubemap;
	unsigned int irradianceMap = 0;
	unsigned int prefilterMap;
	unsigned int hdrTexture_daylight;
	unsigned int hdrTexture_nightlight;
	unsigned int brdfLUTTexture;


	float mixFactor = 0.8f;

	void renderQuad();
	void renderCube();


	//IMGUI
	AssetsPanel assetsPanel;


	string selectedPath = "";

	void generateDirectionalLight();
	void generatePointLight();
	static std::chrono::high_resolution_clock::time_point lastTime;
	bool isSliderActive = false;

	void cleanUp();
	void prepare_PBR_IBL();


	std::string ws2s(const std::wstring& wide);
};

