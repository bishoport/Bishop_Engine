#pragma once
#include "glpch.h"
#include "ECSCore.h"
#include <GLFW/glfw3.h>


namespace ECS
{
	class PointLight : public ECS::Component
	{
	public:

		bool active = true;
		bool drawSphereDebug = true;
		bool showRange = false;
		bool blinn = true;

		glm::vec3 ambient  = glm::vec3(1.0f);
		glm::vec3 diffuse  = glm::vec3(1.0f);
		glm::vec3 specular = glm::vec3(1.0f);

		int currentAttenuation = 0; // 0 = Sin atenuación, 1 = Atenuación lineal, 2 = Atenuación cuadrática
		float constant = 1.0f;
		float linear = 1.0f;
		float quadratic = 1.0f;

		float strength = 10.0f;
		float range = 1.0f;
		float lightSmoothness = 0.8f;
		float specularPower = 1.0f;


		/*const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
		unsigned int depthMapFBO;
		unsigned int depthCubemap;*/

		bool drawShadows = false;
		int shadowMapResolution = 1024;
		GLuint shadowTex = 0;
		GLuint shadowDepth = 0;
		GLuint shadowFBO = 0;



		void init() override {}

		void setLightValues(glm::vec3 position, unsigned int lightID, GLCore::Shader* mainShader, GLCore::Shader* lightPointShadowShader, GLCore::Shader* debugShader)
		{
			this->lightID = lightID;
			this->mainShader = mainShader;
			this->shadowMappingDepthShader = lightPointShadowShader;
			this->debugShader = debugShader;
			


			//By default add a transform component
			entity->getComponent<Transform>().position.x = position.x;
			entity->getComponent<Transform>().position.y = position.y;
			entity->getComponent<Transform>().position.z = position.z;


			entity->name = "PointLight_" + std::to_string(lightID);





			//SHADOWS
			bool drawShadows;
			float shadowIntensity;
			int usePoisonDisk;


			//GET LOCATIONS
			//Active
			activeL		     << "pointLights[" << lightID << "].isActive";
			//DrawShadow
			drawShadowsL     << "pointLights[" << lightID << "].drawShadows";
			//Ambient	     
			ambientL	     << "pointLights[" << lightID << "].ambient";
			//Diffuse	     
			diffuseL	     << "pointLights[" << lightID << "].diffuse";
			//Specular	     
			specularL	     << "pointLights[" << lightID << "].specular";
			//Position	     
			positionL	     << "pointLights[" << lightID << "].position";
			//Strength	     
			strengthL	     << "pointLights[" << lightID << "].strength";
			//Constant	     
			constantL	     << "pointLights[" << lightID << "].constant";
			//Linear	     
			linearL		     << "pointLights[" << lightID << "].linear";
			//Quadratic	     
			quadraticL	     << "pointLights[" << lightID << "].quadratic";
			//Range		     
			rangeL		     << "pointLights[" << lightID << "].range";
			//Light Smoothness
			lightSmoothnessL << "pointLights[" << lightID << "].lightSmoothness";
			//Blinn
			blinnL		     << "pointLights[" << lightID << "].blinn";
			//Specular Power
			specularPowerL   << "pointLights[" << lightID << "].specularPower";
			//Mapa de sombras
			depthMapL        << "pointLights[" << lightID << "].shadowMap";


			prepareShadows();
			prepareDebug();
		}



		void prepareShadows()
		{
			// configure depth map FBO
			// -----------------------
			glGenFramebuffers(1, &shadowFBO);
			// create depth cubemap texture
			
			glGenTextures(1, &shadowTex);
			glBindTexture(GL_TEXTURE_CUBE_MAP, shadowTex);
			for (unsigned int i = 0; i < 6; ++i)
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, shadowMapResolution, shadowMapResolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			
			// attach depth texture as FBO's depth buffer
			glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowTex, 0);
			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, 0); //unbind texture
		}


		void shadowMappingProjection(std::vector<ECS::Entity*> entitiesInScene)
		{
			// move light position over time
			//entity->getComponent<Transform>().position.z = static_cast<float>(sin(glfwGetTime() * 0.5) * 3.0);

			// render
			// ------
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// 0. create depth cubemap transformation matrices
			// -----------------------------------------------
			float near_plane = 0.0f;
			float far_plane = range;
			glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)shadowMapResolution / (float)shadowMapResolution, near_plane, far_plane);
			std::vector<glm::mat4> shadowTransforms;

			glm::vec3 lightPos = entity->getComponent<Transform>().position;

			shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));



			// 1. render scene to depth cubemap
			// --------------------------------
			glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
			glViewport(0, 0, shadowMapResolution, shadowMapResolution);
			
			//glClear(GL_DEPTH_BUFFER_BIT);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			shadowMappingDepthShader->use();
	
			for (unsigned int i = 0; i < 6; ++i)
				shadowMappingDepthShader->setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
			
			shadowMappingDepthShader->setFloat("far_plane[" + std::to_string(lightID) + "]", far_plane);
			shadowMappingDepthShader->setVec3("lightPos[" + std::to_string(lightID) + "]", lightPos);

			//renderScene(simpleDepthShader);
			for (int i = 0; i < entitiesInScene.size(); i++)
			{
				if (entitiesInScene[i]->hascomponent<Drawable>())
				{
					if (entitiesInScene[i]->getComponent<Drawable>().dropShadow)
					{
						//mat4 entityShadowMVP = shadowMVP * entitiesInScene[i]->getComponent<Drawable>().model_transform_matrix;
						shadowMappingDepthShader->setMat4("model", entitiesInScene[i]->getComponent<Drawable>().model_transform_matrix);
						entitiesInScene[i]->getComponent<Drawable>().bindModelVAO();
					}
				}
			}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}



		void setDataInShader()
		{
			//glBindTexture(GL_TEXTURE_CUBE_MAP, 0); //unbind texture
			
			
			glActiveTexture(GL_TEXTURE0 + 5);              // donde SHADOW_SLOT es un índice que no se solape con tus otras texturas.
			glBindTexture(GL_TEXTURE_CUBE_MAP, shadowTex);   // 'shadowTextureID' es el ID de la textura de sombra que creaste.
			mainShader->setInt(depthMapL.str().c_str(), 5);

			mainShader->use();
			mainShader->setBool(activeL.str().c_str()          , active);
			mainShader->setBool(drawShadowsL.str().c_str()     , drawShadows);
			mainShader->setVec3(positionL.str().c_str()        , entity->getComponent<Transform>().position);
			mainShader->setVec3(ambientL.str().c_str()         , ambient);
			mainShader->setVec3(diffuseL.str().c_str()         , diffuse);
			mainShader->setVec3(specularL.str().c_str()        , specular);
			mainShader->setFloat(strengthL.str().c_str()       , strength);
			mainShader->setFloat(constantL.str().c_str()       , constant);
			mainShader->setFloat(linearL.str().c_str()         , linear);
			mainShader->setFloat(quadraticL.str().c_str()      , quadratic);
			mainShader->setFloat(rangeL.str().c_str()          , range);
			mainShader->setFloat(lightSmoothnessL.str().c_str(), lightSmoothness);
			mainShader->setFloat(specularPowerL.str().c_str()  , specularPower);
			mainShader->setBool(blinnL.str().c_str()           , blinn);
		}



		void update() override {}

		void draw() override
		{
			if (drawSphereDebug)
			{
				drawDebug();
			}
		}

		

		void onDestroy() override
		{
			glDeleteVertexArrays(1, &vao);
		}



	private:

		unsigned int lightID = 0;
		
		std::stringstream location;

		//Locations
		std::stringstream activeL;
		std::stringstream drawShadowsL;
		std::stringstream ambientL;
		std::stringstream diffuseL;
		std::stringstream specularL;
		std::stringstream positionL;
		std::stringstream strengthL;
		std::stringstream constantL;
		std::stringstream linearL;
		std::stringstream quadraticL;
		std::stringstream rangeL;
		std::stringstream lightSmoothnessL;
		std::stringstream blinnL;
		std::stringstream specularPowerL;
		std::stringstream depthMapL;

		

		GLCore::Shader* mainShader;
		GLCore::Shader* debugShader;
		GLCore::Shader* shadowMappingDepthShader;




		GLuint vao;
		std::vector<glm::vec3> sphereVertices;
		std::vector<unsigned int> sphereIndices;
		std::vector<glm::vec3> sphereNormals;

		float pi = 3.1415926535;

		void prepareDebug()
		{
			// Definir los parámetros de la esfera de debug
			const int numSlices = 6;
			const int numStacks = 6;
			const float radius = 1.0f;

			// Crear los vértices de la esfera
			for (int i = 0; i <= numStacks; i++) {
				float phi = glm::pi<float>() * i / numStacks;
				float cosPhi = cos(phi);
				float sinPhi = sin(phi);

				for (int j = 0; j <= numSlices; j++) {
					float theta = 2 * glm::pi<float>() * j / numSlices;
					float cosTheta = cos(theta);
					float sinTheta = sin(theta);

					glm::vec3 vertex(cosTheta * sinPhi, cosPhi, sinTheta * sinPhi);
					sphereVertices.push_back(radius * vertex);
				}
			}

			// Crear los índices de la esfera
			for (int i = 0; i < numStacks; i++) {
				int stackStart = i * (numSlices + 1);
				int nextStackStart = (i + 1) * (numSlices + 1);

				for (int j = 0; j < numSlices; j++) {
					int vertex1 = stackStart + j;
					int vertex2 = stackStart + j + 1;
					int vertex3 = nextStackStart + j + 1;
					int vertex4 = nextStackStart + j;

					sphereIndices.push_back(vertex1);
					sphereIndices.push_back(vertex2);
					sphereIndices.push_back(vertex3);

					sphereIndices.push_back(vertex1);
					sphereIndices.push_back(vertex3);
					sphereIndices.push_back(vertex4);
				}
			}


			// Calcular las normales de la esfera
			for (unsigned int i = 0; i < sphereVertices.size(); i++) {
				glm::vec3 normal = glm::normalize(sphereVertices[i]);
				sphereNormals.push_back(normal);
			}
		}

		void drawDebug()
		{
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

			// Crear los VBOs (Vertex Buffer Objects) para los vértices y las normales de la esfera
			GLuint vbo;
			glGenBuffers(1, &vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(glm::vec3), &sphereVertices[0], GL_STATIC_DRAW);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(0);

			GLuint nbo;
			glGenBuffers(1, &nbo);
			glBindBuffer(GL_ARRAY_BUFFER, nbo);
			glBufferData(GL_ARRAY_BUFFER, sphereNormals.size() * sizeof(glm::vec3), &sphereNormals[0], GL_STATIC_DRAW);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(1);

			// Crear el VBO (Vertex Buffer Object) para los índices de la esfera
			GLuint ebo;
			glGenBuffers(1, &ebo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(GLuint), &sphereIndices[0], GL_STATIC_DRAW);

			glm::mat4 modelMatrix = glm::mat4(1.0f);

			float radiusValue = 1.0f;
			if (showRange) radiusValue = range;

			glm::vec3 size = glm::vec3(radiusValue, radiusValue, radiusValue);
			modelMatrix = glm::translate(modelMatrix, entity->getComponent<Transform>().position) * glm::scale(glm::mat4(1), size);

			debugShader->use();
			debugShader->setMat4("model", modelMatrix);
			debugShader->setVec4("u_Color", glm::vec4(ambient.r, ambient.g, ambient.b, 1.0));


			// Dibujar la esfera
			glLineWidth(5);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

	};
}

