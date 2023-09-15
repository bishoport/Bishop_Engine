#include "glpch.h"
#include "IMGLoader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Inicializa el mapa
std::unordered_map<std::string, GLCore::Image> GLCore::Loaders::loadedImages = {};


GLCore::Image GLCore::Loaders::getOrLoadImage(const std::string& path) {
	// Si la imagen ya ha sido cargada, simplemente devuélvela
	if (loadedImages.find(path) != loadedImages.end()) {
		std::cout << "it already exists" << std::endl;
		return loadedImages[path];
	}

	// Si no ha sido cargada, cárgala y añádela al mapa
	GLCore::Image newImage = load_from_file(path.c_str());
	loadedImages[path] = newImage;

	return newImage;
}


GLCore::Image  GLCore::Loaders::load_from_file(const char* filename) {
	Image image;
	stbi_set_flip_vertically_on_load(false);
	image.pixels = stbi_load(filename, &(image.width), &(image.height), &(image.channels), 0);

	if (image.pixels)
	{
		std::cout << "Image loaded successfully" << std::endl;
	}
	else
	{
		std::cout << "Image failed to load at path: " << filename << std::endl;
	}

	return image;
}



GLCore::Image GLCore::Loaders::Load_from_memory(const aiTexture* texture)
{
	unsigned char* image_data = nullptr;
	GLCore::Image image;

	if (texture->mHeight == 0)
	{
		image.pixels = stbi_load_from_memory(reinterpret_cast<unsigned char*>(texture->pcData), texture->mWidth, &(image.width), &(image.height), &(image.channels), 0);
	}
	else
	{
		image.pixels = stbi_load_from_memory(reinterpret_cast<unsigned char*>(texture->pcData), texture->mWidth * texture->mHeight, &(image.width), &(image.height), &(image.channels), 0);
	}

	// Si tienes un constructor de GLCore::Image que acepta estos parámetros puedes retornar la imagen de esta manera
	return image;
}


GLuint GLCore::Loaders::LoadIconTexture(const char* filepath)
{
	int width, height, channels;
	unsigned char* data = stbi_load(filepath, &width, &height, &channels, 0);
	if (data)
	{
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, channels == 4 ? GL_RGBA : GL_RGB, width, height, 0, channels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);
		stbi_image_free(data);
		return texture;
	}
	else
	{
		std::cout << "Failed to load Icon texture: " << filepath << std::endl;
		return 0;
	}
}





unsigned int GLCore::Loaders::loadTexture(char const* path)
{
	stbi_set_flip_vertically_on_load(false);
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}







void GLCore::Loaders::free_image_memory(GLCore::Image oldImage) 
{
	stbi_image_free(oldImage.pixels);
}



float GLCore::Loaders::rand_FloatRange(float a, float b)
{
	return ((b - a) * ((float)rand() / RAND_MAX)) + a;
}

GLuint GLCore::Loaders::loadHDR(const char* filename)
{
	stbi_set_flip_vertically_on_load(true);
	int width, height, nrComponents;
	float* data = stbi_loadf(filename, &width, &height, &nrComponents, 0);
	unsigned int hdrTexture_daylight{};
	if (data)
	{
		glGenTextures(1, &hdrTexture_daylight);
		glBindTexture(GL_TEXTURE_2D, hdrTexture_daylight);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data); // note how we specify the texture's data value to be float

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);

		std::cout << "Image HDR loaded successfully" << std::endl;
	}
	else
	{
		std::cout << "Failed to load HDR image." << std::endl;
	}

	return hdrTexture_daylight;
}

// loads a cubemap texture from 6 individual texture faces
// order:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front) 
// -Z (back)
GLuint GLCore::Loaders::loadCubemap(std::vector<const char*> faces)
{
	stbi_set_flip_vertically_on_load(false);

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrComponents;


	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i], &width, &height, &nrComponents, 0);
		
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);

			std::cout << "Cubemap texture loaded at path: " << faces[i] << std::endl;
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}
