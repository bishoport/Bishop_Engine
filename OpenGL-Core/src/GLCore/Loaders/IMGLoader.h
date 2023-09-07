#pragma once
#include <assimp/texture.h>
#include <unordered_map>  // Incluye la librería necesaria
#include <string>

namespace GLCore::Loaders {

	// Declara el mapa directamente dentro del namespace
	extern std::unordered_map<std::string, GLCore::Image> loadedImages;

	// Declara la nueva función para obtener o cargar la imagen
	GLCore::Image getOrLoadImage(const std::string& path);

	GLCore::Image load_from_file(const char* filename);
	GLCore::Image Load_from_memory(const aiTexture* texture);

	unsigned int loadTexture(char const* path);

	GLuint LoadIconTexture(const char* filepath);

	void free_image_memory(Image oldImage);
	float rand_FloatRange(float a, float b);


	GLuint loadHDR(const char* filename);
}