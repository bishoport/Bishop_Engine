#pragma once
#include <filesystem>
#include <glad/glad.h>
#include <GLCore/DataStructs.h>


class AssetsPanel
{
public:

	AssetsPanel();
	
	void setIcons(GLuint folderTexture, GLuint modelTexture, GLuint imageTexture);
	
	void OnImGuiRender();
	
	// Función para establecer el delegado
	void SetDelegate(std::function<void(const std::string&, const std::string&)> delegado) {
		getModelPathFromAssetsDelegate = delegado;
	}

	

private:
	std::filesystem::path m_BaseDirectory;
	std::filesystem::path m_CurrentDirectory;

	//Icons
	GLuint folderTexture;
	GLuint modelTexture;
	GLuint imageTexture;

	std::filesystem::path s_AssetPath = "assets";


	//callbacks:
	std::function<void(const std::string&, const std::string&)> getModelPathFromAssetsDelegate;

	// Función que utiliza el delegado
	void UsarDelegado(const std::string& filePath, const std::string& fileName) {
		if (getModelPathFromAssetsDelegate) {  // Verifica que el delegado haya sido establecido
			getModelPathFromAssetsDelegate(filePath, fileName);
		}
	}
};