#pragma once
#include "glpch.h"
#include "ECSCore.h"
#include <GLCoreUtils.h>

namespace ECS
{
    class DirectionalLight : public ECS::Component
    {
    public:

        bool active = true;

        //Light values
        glm::vec3 m_Color = glm::vec3(1.0f, 1.0f, 1.0f);
        glm::vec3 direction = glm::vec3(-6.0f, -10.0f,-50.0f);
        glm::vec3 ambient = glm::vec3(1.0f, 1.0f, 1.0f);
        glm::vec3 diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
        glm::vec3 specular = glm::vec3(1.0f, 1.0f, 1.0f);
        float specularPower = 1.0f;

        //Shadow values
        int shadowMapResolution = 1024;
        GLuint shadowTex = 0;
        GLuint shadowDepth = 0;
        GLuint shadowFBO = 0;

        mat4 shadowMVP = mat4(1.0f);

        bool drawShadows = false;

        float near_plane = 0.1f, far_plane = 100.5f;
        float shadowIntensity = 1.0f;
        bool usePoisonDisk = true;
        float orthoLeft = -10.0f;
        float orthoRight = 10.0f;
        float orthoBottom = -10.0f;
        float orthoTop = 10.0f;
        float orthoNear = 0.1f;
        float orthoFar = 100.0f;

        //bright values
        bool blinn = true;


        void init() override
        {
            entity->getComponent<Transform>().position.y = 5;
        }


        void setLightValues(const glm::vec3& position,
            unsigned int lightID,
            GLCore::Shader* mainShader,
            GLCore::Shader* debugShader,
            GLCore::Shader* shadowMappingDepthShader)
        {

            this->lightID = lightID;
            this->debugShader = debugShader;
            this->mainShader = mainShader;
            this->shadowMappingDepthShader = shadowMappingDepthShader;

            entity->name = "DirectionalLight_" + std::to_string(lightID);

            prepareShadows();
        }



        void update() override {}

        void prepareShadows()
        {
            //--SHADOW MAPPING
            glGenTextures(1, &shadowTex);
            glBindTexture(GL_TEXTURE_2D, shadowTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, shadowMapResolution, shadowMapResolution, 0, GL_RGB, GL_FLOAT, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glGenTextures(1, &shadowDepth);
            glBindTexture(GL_TEXTURE_2D, shadowDepth);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, shadowMapResolution, shadowMapResolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


            glGenFramebuffers(1, &shadowFBO);
            glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, shadowTex, 0);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowDepth, 0);

            GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(1, drawBuffers);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) std::cout << "Framebuffer not complete!" << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glBindTexture(GL_TEXTURE_2D, 0); //unbind texture
            //-------------------------------------------------------------------------------------------------------------------------------------------------------
        }



        void shadowMappingProjection(std::vector<ECS::Entity*> entitiesInScene)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
            glViewport(0, 0, shadowMapResolution, shadowMapResolution);
            glClearColor(1, 1, 1, 1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            shadowMappingDepthShader->use();
            mat4 shadowProjMat = ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, orthoNear, orthoFar);
            mat4 shadowViewMat = lookAt(entity->getComponent<Transform>().position, direction, vec3(0, 1, 0));
            shadowMVP = shadowProjMat * shadowViewMat;

            for (int i = 0; i < entitiesInScene.size(); i++)
            {
                if (entitiesInScene[i]->hascomponent<Drawable>())
                {
                    if (entitiesInScene[i]->getComponent<Drawable>().dropShadow)
                    {
                        mat4 entityShadowMVP = shadowMVP * entitiesInScene[i]->getComponent<Drawable>().model_transform_matrix;
                        shadowMappingDepthShader->setMat4("shadowMVP", entityShadowMVP);
                        entitiesInScene[i]->getComponent<Drawable>().bindModelVAO();
                    }
                }
            }
            glBindTexture(GL_TEXTURE_2D, 0);
        }


        void setDataInShader()
        {
            glActiveTexture(GL_TEXTURE0 + 6);              // donde SHADOW_SLOT es un índice que no se solape con tus otras texturas.
            glBindTexture(GL_TEXTURE_2D, shadowTex);   // 'shadowTextureID' es el ID de la textura de sombra que creaste.

            //--CONFIGURE DIRECTIONAL LIGHT
            mainShader->use();
            mainShader->setBool("useDirLight", true);
            mainShader->setBool("dirLight.drawShadows", drawShadows);
            mainShader->setInt("dirLight.shadowMap", 6);
            mainShader->setVec3("dirLight.lightColor", m_Color);
            mainShader->setVec3("dirLight.lightPos", entity->getComponent<Transform>().position);
            mainShader->setVec3("dirLight.direction", direction);
            mainShader->setVec3("dirLight.ambient", ambient);
            mainShader->setVec3("dirLight.diffuse", diffuse);
            mainShader->setVec3("dirLight.specular", specular);
            mainShader->setFloat("dirLight.shadowIntensity", shadowIntensity);
            mainShader->setFloat("dirLight.specularPower",specularPower);
            mainShader->setInt("dirLight.usePoisonDisk", usePoisonDisk);
            mainShader->setBool("dirLight.blinn", blinn);
            mainShader->setBool("dirLight.isActive", active);
            mat4 shadowBias = translate(vec3(0.5)) * scale(vec3(0.5));
            mainShader->setMat4("dirLight.shadowBiasMVP", shadowBias * shadowMVP);
        }


        void draw() override
        {
            drawLineDebug();
            drawSphereDebug();
        }

        void onDestroy() override {}

    private:

        void drawSphereDebug()
        {
            // Llamar a la función drawMesh con los datos de la esfera
            GLCore::Utils::MeshHelper::drawMesh(sphereVertices, sphereNormals, sphereIndices);

            glm::mat4 modelMatrix = glm::mat4(1.0f);
            //glm::vec3 size = glm::vec3(sphereDebugRadius, sphereDebugRadius, sphereDebugRadius);
            glm::vec3 size = glm::vec3(sphereDebugRadius);
            modelMatrix = glm::translate(modelMatrix, entity->getComponent<Transform>().position) * glm::scale(glm::mat4(1), size);

            debugShader->use();
            debugShader->setMat4("model", modelMatrix);
            debugShader->setVec4("u_Color", glm::vec4(m_Color.r, m_Color.g, m_Color.b, 1.0));
            glLineWidth(2);
            // Dibujar la esfera
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        void drawLineDebug() {
            // Crea un array con los puntos de la línea
            glm::vec3 rotationVector = direction;

            glm::vec3 linePoints[] = {
                entity->getComponent<Transform>().position,
                entity->getComponent<Transform>().position + rotationVector * 10.0f  // Aquí asumimos que la longitud de la línea es 10
            };

            // Crea el VBO y el VAO para la línea
            GLuint lineVBO, lineVAO;
            glGenBuffers(1, &lineVBO);
            glGenVertexArrays(1, &lineVAO);

            // Rellena el VBO con los puntos de la línea
            glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(linePoints), linePoints, GL_STATIC_DRAW);

            // Enlaza el VBO al VAO
            glBindVertexArray(lineVAO);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

            // Dibuja la línea
            debugShader->use();
            debugShader->setMat4("model", glm::mat4(1.0f));
            debugShader->setVec4("u_Color", glm::vec4(m_Color.r, m_Color.g, m_Color.b, 1.0));
            glLineWidth(5);

            glBindVertexArray(lineVAO);
            glDrawArrays(GL_LINES, 0, 2);

            // Elimina el VBO y el VAO de la línea
            glDeleteBuffers(1, &lineVBO);
            glDeleteVertexArrays(1, &lineVAO);
        }


        unsigned int lightID = 0;   
        float pi = 3.1415926535;

        glm::mat4 lightProjection, lightView;
        glm::mat4 lightSpaceMatrix;

        //Shaders
        GLCore::Shader* debugShader = nullptr;
        GLCore::Shader* mainShader = nullptr;
        GLCore::Shader* shadowMappingDepthShader = nullptr;




        //------------------------DEBUG---------------------------------
        const int numSlices = 6;
        const int numStacks = 6;
        const float sphereDebugRadius = 0.3f;
        GLuint sphereDebugVao;

        std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<GLuint>> sphereData = GLCore::Utils::MeshHelper::createSphere(sphereDebugRadius, numStacks, numSlices);
        std::vector<glm::vec3> sphereVertices = std::get<0>(sphereData);
        std::vector<glm::vec3> sphereNormals = std::get<1>(sphereData);
        std::vector<GLuint>    sphereIndices = std::get<2>(sphereData);
    };
}


