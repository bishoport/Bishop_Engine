 #pragma once
#include "glpch.h"
#include "ECSCore.h"

#include <GLCore/Core/Render/Shader.h>
#include <GLCore/DataStructs.h>


namespace ECS
{

	class Drawable : public ECS::Component
	{
	public:

		GLCore::ModelInfo modelInfo;

		GLCore::Shader* mainShader = nullptr;
		GLCore::Shader* debugShader = nullptr;

		bool visibleModel = true;
		bool drawGlobalBB = false;
		bool drawLocalBB = false;

		bool dropShadow = true;

		GLuint texture = 0;

		glm::mat4 model_transform_matrix{ glm::mat4(1.0f) };

		void init() override{}



		void setModelInfo(GLCore::ModelInfo _modelInfo, GLCore::Shader* _mainShader = nullptr, GLCore::Shader* _debugShader = nullptr, bool keepOriginalPosition = false)
		{
			this->mainShader = _mainShader;
			this->debugShader = _debugShader;
			this->modelInfo = _modelInfo;

			entity->name = _modelInfo.model_mesh.meshName;

			if (keepOriginalPosition)
			{
				entity->getComponent<Transform>().position = _modelInfo.model_mesh.meshLocalPosition;
			}


			//------------------------------------PREPARE MODELS-----------------------------------------------
			std::vector<glm::mat4> instanceMatrices;
			instanceMatrices.push_back(model_transform_matrix);
			modelInfo.model_mesh.CreateInstanceVBO(instanceMatrices);
			//------------------------------------------------------------------------------------------------


			//------------------------------------CALCULATE BOUNDING BOX--------------------------------------
			modelInfo.model_mesh.CalculateBoundingBox(entity->getComponent<Transform>().GetTransform());
			//------------------------------------------------------------------------------------------------


			//-----------------------------------PREPARE BOUNDING BOX LOCAL-----------------------------------
			glCreateVertexArrays(1, &modelInfo.model_mesh.VAO_BB);
			glCreateBuffers(1, &modelInfo.model_mesh.VBO_BB);

			glVertexArrayVertexBuffer(modelInfo.model_mesh.VAO_BB, 0, modelInfo.model_mesh.VBO_BB, 0, 3 * sizeof(GLfloat));
			glNamedBufferStorage(modelInfo.model_mesh.VBO_BB, sizeof(vertices_BB), vertices_BB, GL_DYNAMIC_STORAGE_BIT);

			glEnableVertexArrayAttrib(modelInfo.model_mesh.VAO_BB, 0);
			glVertexArrayAttribFormat(modelInfo.model_mesh.VAO_BB, 0, 3, GL_FLOAT, GL_FALSE, 0);
			glVertexArrayAttribBinding(modelInfo.model_mesh.VAO_BB, 0, 0);
			//-------------------------------------------------------------------------------------------------


			//--------------------------------------PREPARE TEXTURES-------------------------------------------
			modelInfo.model_material.PrepareMaterial();
			//-------------------------------------------------------------------------------------------------
		}


		void update() override
		{
			// Recuperamos el componente Transform de la entidad
			Transform& transform = entity->getComponent<Transform>();

			// Ahora, directamente podemos obtener la matriz de transformación del objeto
			model_transform_matrix = transform.GetTransform();

			// Si hay un padre, combinamos nuestras transformaciones con las de él.
			if (entity->getComponent<Transform>().parent != nullptr) {
				model_transform_matrix = entity->getComponent<Transform>().parent->getComponent<Transform>().GetTransform() * model_transform_matrix;
			}
		}



		void bindModelVAO()
		{
			modelInfo.model_mesh.BindVAO(false);
		}


		void bindTextures()
		{
			//--Albedo Map
			if (modelInfo.model_material.hasAlbedoMap) { // si tienes un mapa difuso
				glActiveTexture(GL_TEXTURE3);
				glBindTexture(GL_TEXTURE_2D, modelInfo.model_material.albedoMap.textureID);
				mainShader->setInt("material.albedoMap", 3);
				mainShader->setBool("material.hasAlbedoMap", true);
			}
			else 
			{
				mainShader->setBool("material.hasAlbedoMap", false);
				glActiveTexture(GL_TEXTURE3);
				glBindTexture(GL_TEXTURE_2D, 0);
			}

			//--Normal Map
			if (modelInfo.model_material.hasNormalMap) { // si tienes un mapa normales
				glActiveTexture(GL_TEXTURE4);
				glBindTexture(GL_TEXTURE_2D, modelInfo.model_material.normalMap.textureID);
				mainShader->setInt("material.normalMap", 4);
				mainShader->setBool("material.hasNormalMap", true);
			}
			else
			{
				mainShader->setBool("material.hasNormalMap", false);
				glActiveTexture(GL_TEXTURE4);
				glBindTexture(GL_TEXTURE_2D, 0);
			}


			//--METALLIC Map
			if (modelInfo.model_material.hasMetallicMap) { // si tienes un mapa normales
				glActiveTexture(GL_TEXTURE5);
				glBindTexture(GL_TEXTURE_2D, modelInfo.model_material.metallicMap.textureID);
				mainShader->setInt("material.metallicMap", 5);
				mainShader->setBool("material.hasMetallicMap", true);
			}
			else
			{
				mainShader->setBool("material.hasMetallicMap", false);
				glActiveTexture(GL_TEXTURE5);
				glBindTexture(GL_TEXTURE_2D, 0);
			}



			//--ROUGHNESS Map
			if (modelInfo.model_material.hasRougnessMap) { // si tienes un mapa normales
				glActiveTexture(GL_TEXTURE6);
				glBindTexture(GL_TEXTURE_2D, modelInfo.model_material.rougnessMap.textureID);
				mainShader->setInt("material.roughnessMap", 6);
				mainShader->setBool("material.hasRougnessMap", true);
			}
			else
			{
				mainShader->setBool("material.hasRougnessMap", false);
				glActiveTexture(GL_TEXTURE6);
				glBindTexture(GL_TEXTURE_2D, 0);
			}



			//--AO Map
			if (modelInfo.model_material.hasAoMap) { // si tienes un mapa especulares
				glActiveTexture(GL_TEXTURE7);
				glBindTexture(GL_TEXTURE_2D, modelInfo.model_material.aOMap.textureID);
				mainShader->setInt("material.aoMap", 7);
				mainShader->setBool("material.hasAoMap", true);
			}
			else
			{
				mainShader->setBool("material.hasAoMap", false);
				glActiveTexture(GL_TEXTURE7);
				glBindTexture(GL_TEXTURE_2D, 0);
			}


			mainShader->setVec3( "material.albedoColor",     modelInfo.model_material.albedoColor);
			mainShader->setFloat("material.metallicValue",   modelInfo.model_material.metallicValue);
			mainShader->setFloat("material.roughnessValue",  modelInfo.model_material.roughnessValue);
			mainShader->setFloat("material.normalIntensity", modelInfo.model_material.normalIntensity);
			mainShader->setFloat("material.reflectanceValue", modelInfo.model_material.reflectanceValue);
			mainShader->setFloat("material.fresnelCoefValue", modelInfo.model_material.fresnelCoefValue);
		}




		void draw() override
		{
			//----------------------------------------PINTAMOS CADA MODELO------------------------------------------------
			if (visibleModel) {
				if (mainShader != nullptr)
				{
					mainShader->use();
					mainShader->setMat4("model", model_transform_matrix);
					mainShader->setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model_transform_matrix))));
					
					//mainShader->setMat4("modelMat", model_transform_matrix);
					//glBindTexture(GL_TEXTURE_2D, 0);
					bindTextures();
					bindModelVAO();
				}
			}
			//---------------------------------------------------------------------------------------------------------------






			//----------------------------------------CALCULATE BOUNDING BOX FOR EACH MESH----------------------------------------
			// Calculamos el tamaño y el centro de la caja original (en el espacio del objeto)
			glm::vec3 size = (modelInfo.model_mesh.originalBoundingBoxMax - modelInfo.model_mesh.originalBoundingBoxMin);
			glm::vec3 center = (modelInfo.model_mesh.originalBoundingBoxMax + modelInfo.model_mesh.originalBoundingBoxMin) / 2.0f;

			// Creamos la matriz de transformación de la caja delimitadora inicial
			glm::mat4 bb_transform_matrix = glm::mat4(1.0f);
			bb_transform_matrix = glm::translate(bb_transform_matrix, center);
			bb_transform_matrix = glm::scale(bb_transform_matrix, size);

			// Obtenemos la matriz de transformación del objeto
			glm::mat4 obj_transform_matrix = entity->getComponent<Transform>().GetTransform();

			// Multiplicamos la matriz de transformación del objeto por la matriz de transformación de la caja delimitadora
			bb_transform_matrix = obj_transform_matrix * bb_transform_matrix;

			if (entity->getComponent<Transform>().parent != nullptr) {
				bb_transform_matrix = entity->getComponent<Transform>().parent->getComponent<Transform>().GetTransform() * bb_transform_matrix;
			}

			// Aquí actualizamos el boundingBoxMin y boundingBoxMax con la caja delimitadora transformada
			glm::vec3 boundingBoxMin = glm::vec3(bb_transform_matrix * glm::vec4(modelInfo.model_mesh.originalBoundingBoxMin, 1.0f));
			glm::vec3 boundingBoxMax = glm::vec3(bb_transform_matrix * glm::vec4(modelInfo.model_mesh.originalBoundingBoxMax, 1.0f));

			modelInfo.model_mesh.boundingBoxMin = boundingBoxMin;
			modelInfo.model_mesh.boundingBoxMax = boundingBoxMax;


			//--Draw
			if (drawLocalBB)
			{
				debugShader->use();
				debugShader->setMat4("model", bb_transform_matrix);
				debugShader->setVec4("u_Color", glm::vec4(1.0, 0.5, 0.2, 1.0));

				glLineWidth(5);
				glBindVertexArray(modelInfo.model_mesh.VAO_BB);
				glDrawArrays(GL_LINES, 0, sizeof(indices_BB));
			}
			//----------
			
			
			//---------------------------------------------------------------------------------------------------------------
		}



		void onDestroy() override {}




	private:

		GLfloat vertices_BB[72] = {
			-0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  // Línea 1
			 0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  // Línea 2
			 0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  // Línea 3
			-0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  // Línea 4
			-0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  // Línea 5
			 0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  // Línea 6
			 0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  // Línea 7
			-0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  // Línea 8
			-0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f,  // Línea 9
			 0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  // Línea 10
			 0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  // Línea 11
			-0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f   // Línea 12
		};

		// Definimos los índices de los vértices del bounding box que forman las líneas
		GLuint indices_BB[24] = {
			0, 1, 1, 2, 2, 3, 3, 0,  // Líneas de la cara frontal
			4, 5, 5, 6, 6, 7, 7, 4,  // Líneas de la cara trasera
			0, 4, 1, 5, 2, 6, 3, 7   // Líneas diagonales
		};
	};
}





/*if (entity->parent != nullptr) {
	bb_transform_matrix = entity->parent->getComponent<Transform>().GetTransform() * bb_transform_matrix;
}*/


//-MODO SIN Direct State Access (DSA) de OpenGL
			//glGenVertexArrays(1, &modelInfo.model_mesh.VAO);
			//glGenBuffers(1, &modelInfo.model_mesh.VBO);
			//glGenBuffers(1, &modelInfo.model_mesh.EBO);

			//// 1. bind Vertex Array Object
			//glBindVertexArray(modelInfo.model_mesh.VAO);

			//// 2. copy our vertices array in a vertex buffer for OpenGL to use
			//glBindBuffer(GL_ARRAY_BUFFER, modelInfo.model_mesh.VBO);
			//glBufferData(GL_ARRAY_BUFFER, modelInfo.model_mesh.allBuffer.size(), &modelInfo.model_mesh.allBuffer, GL_STATIC_DRAW);

			//// 3. copy our index array in a element buffer for OpenGL to use
			//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelInfo.model_mesh.EBO);
			//glBufferData(GL_ELEMENT_ARRAY_BUFFER, modelInfo.model_mesh.indices.size(), &modelInfo.model_mesh.indices, GL_STATIC_DRAW);

			//// 4. then set the vertex attributes pointers
			//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
			//glEnableVertexAttribArray(0);

			//glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(2 * sizeof(float)));
			//glEnableVertexAttribArray(1);

			//glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
			//glEnableVertexAttribArray(2);
			//-----------------------------------------------------------------------------------------------------------------------------