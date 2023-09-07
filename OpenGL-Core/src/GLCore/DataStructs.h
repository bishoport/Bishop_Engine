#pragma once
#include "glpch.h"


namespace GLCore 
{

	struct MeshInfo
	{
        std::string meshName;

		unsigned int num_indices = 0;
                                                                                                        //Vertice------normals
		std::vector<GLfloat> allBuffer;  //Secuenciados los datos de vetices normals textcords indices (1.0,2.0,0.0, 1.0,0.5,1.0, 2.3,4.5  ,2  )

		std::vector<GLfloat> vertices;
        std::vector<GLfloat> normals;
        std::vector<GLfloat> texcoords;
        std::vector<GLuint> indices;

        //Esto es para las sombras y las normales
        std::vector<float> tangents;
        std::vector<float> bitangents;

        GLuint VBO, VAO, EBO, vertexCount;
        GLuint instanceVBO;
        GLuint VBO_BB, VAO_BB;

        //BOUNDIG BOX
        // Guarda los vértices originales en alguna parte
        glm::vec3 originalBoundingBoxMax;
        glm::vec3 originalBoundingBoxMin;

        glm::vec3 boundingBoxMin;
        glm::vec3 boundingBoxMax;

        glm::vec3 meshLocalPosition;
        glm::vec3 meshPosition;
        glm::vec3 meshRotation; // En ángulos de Euler
        glm::vec3 meshScale;




        void CreateVAO()
        {
            //------------------------------------PREPARE MODELS-----------------------------------------------
            //-MODO Direct State Access (DSA) de OpenGL
            glCreateVertexArrays(1, &VAO);
            glCreateBuffers(1, &VBO);
            glCreateBuffers(1, &EBO);

            

            //0 en vez de GL_DYNAMIC_STORAGE_BIT para indicar que los datos son estaticos: 
            glNamedBufferStorage(VBO, allBuffer.size() * sizeof(GLfloat), allBuffer.data(), 0);
            glNamedBufferStorage(EBO, indices.size() * sizeof(GLuint), indices.data(), 0);

            glVertexArrayElementBuffer(VAO, EBO);

            glEnableVertexArrayAttrib(VAO, 0); //Pos xyz    3 * sizeof(GLfloat)
            glEnableVertexArrayAttrib(VAO, 1); //Coord      2 * sizeof(GLfloat)
            glEnableVertexArrayAttrib(VAO, 2); //Normales   3 * sizeof(GLfloat)
            glEnableVertexArrayAttrib(VAO, 3); //tangents   3 * sizeof(GLfloat)
            glEnableVertexArrayAttrib(VAO, 4); //bitangents 3 * sizeof(GLfloat)

            glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
            glVertexArrayAttribFormat(VAO, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat));
            glVertexArrayAttribFormat(VAO, 2, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat));
            glVertexArrayAttribFormat(VAO, 3, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat));
            glVertexArrayAttribFormat(VAO, 4, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat));

            glVertexArrayAttribBinding(VAO, 0, 0);
            glVertexArrayAttribBinding(VAO, 1, 0);
            glVertexArrayAttribBinding(VAO, 2, 0);
            glVertexArrayAttribBinding(VAO, 3, 0);
            glVertexArrayAttribBinding(VAO, 4, 0);

            //(3 + 3 + 2 + 3 + 3 = 14 )
            glVertexArrayVertexBuffer(VAO, 0, VBO, 0, 14 * sizeof(GLfloat));
            //------------------------------------------------------------------------------------------------
        }






        void CreateInstanceVBO(const std::vector<glm::mat4>& instanceMatrices)
        {
            glCreateBuffers(1, &instanceVBO);
            glNamedBufferStorage(instanceVBO, instanceMatrices.size() * sizeof(glm::mat4), instanceMatrices.data(), 0);
            glVertexArrayVertexBuffer(VAO, 1, instanceVBO, 0, sizeof(glm::mat4));

            // Configure instance attributes
            glEnableVertexArrayAttrib(VAO, 5);
            glEnableVertexArrayAttrib(VAO, 6);
            glEnableVertexArrayAttrib(VAO, 7);
            glEnableVertexArrayAttrib(VAO, 8);

            glVertexArrayAttribFormat(VAO, 5, 4, GL_FLOAT, GL_FALSE, 0);
            glVertexArrayAttribFormat(VAO, 6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4));
            glVertexArrayAttribFormat(VAO, 7, 4, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::vec4));
            glVertexArrayAttribFormat(VAO, 8, 4, GL_FLOAT, GL_FALSE, 3 * sizeof(glm::vec4));

            glVertexArrayAttribBinding(VAO, 5, 1);
            glVertexArrayAttribBinding(VAO, 6, 1);
            glVertexArrayAttribBinding(VAO, 7, 1);
            glVertexArrayAttribBinding(VAO, 8, 1);

            glVertexArrayBindingDivisor(VAO, 1, 1);
        }





        void BindVAO(bool byInstance = true)
        {
            if (byInstance)
            {
                glBindVertexArray(VAO);
                glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0, 1);
                glBindVertexArray(0);
            }
            else
            {
                glBindVertexArray(VAO);
                //glDrawArrays(GL_TRIANGLES, 0, num_indices); //USANDO VBO
                glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0); //USANDO EBO
                glBindVertexArray(0);
            }
        }


        void CalculateBoundingBox(glm::mat4 transformMatrix)
        {
            // Inicializar las bounding boxes a valores extremos
            glm::vec3 _originalBoundingBoxMin(FLT_MAX, FLT_MAX, FLT_MAX);
            glm::vec3 _originalBoundingBoxMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
            glm::vec3 _boundingBoxMin(FLT_MAX, FLT_MAX, FLT_MAX);
            glm::vec3 _boundingBoxMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);


            // Recorrer todos los vértices en el vector meshInfo.vertices
            for (unsigned int i = 0; i < vertices.size(); i += 3)
            {
                // Obtener las coordenadas del vértice actual
                glm::vec3 vertex = glm::vec3(vertices[i], vertices[i + 1], vertices[i + 2]);

                // Actualizar los valores mínimos y máximos de la bounding box original
                _originalBoundingBoxMin = glm::min(_originalBoundingBoxMin, vertex);
                _originalBoundingBoxMax = glm::max(_originalBoundingBoxMax, vertex);

                // Transformar las coordenadas del vértice al espacio mundial
                glm::vec4 transformedVertex = transformMatrix * glm::vec4(vertex, 1.0f);

                // Actualizar los valores mínimos y máximos de la bounding box en el espacio mundial
                _boundingBoxMin = glm::min(_boundingBoxMin, glm::vec3(transformedVertex));
                _boundingBoxMax = glm::max(_boundingBoxMax, glm::vec3(transformedVertex));
            }

            originalBoundingBoxMin = _originalBoundingBoxMin;
            originalBoundingBoxMax = _originalBoundingBoxMax;
            boundingBoxMin = _boundingBoxMin;
            boundingBoxMax = _boundingBoxMax;
        }
	};






    //LA VERSION EN RAM
    struct Image {
        unsigned char* pixels;
        int width, height, channels;
        std::string path = "NONE";
    };


    //LA VERSION EN GPU
    struct Texture {
        GLuint textureID = 0;
        std::string type;
        Image image;
    };



    //EMPAQUETA LAS TEXTURAS 
    struct Material {
        std::string name;


        //DIFFUSE
        glm::vec3 albedoColor = glm::vec3(1.0f,1.0f,1.0f);
		Texture albedoMap;
        bool hasAlbedoMap = false;

        //NORMAL
		Texture normalMap;
        bool hasNormalMap = false;

        //METALLIC
        Texture metallicMap;
        bool hasMetallicMap = false;


        //ROUGHTNESS
        Texture rougnessMap;
        bool hasRougnessMap = false;

        //AMBIENT OCLUSSION
        Texture aOMap;
        bool hasAoMap = false;



        //SPECULAR (OLD)
        Texture specularMap;
        bool hasSpecularMap = false;

        float normalIntensity = 0.5f;
        float metallicValue = 0.0f;
        float roughnessValue = 0.05f;
        float reflectanceValue = 0.04f;
        float fresnelCoefValue = 5.0f;

        void PrepareMaterial()
        {
            //glActiveTexture(GL_TEXTURE0);
            //glBindTexture(GL_TEXTURE_2D, 0);
            //glActiveTexture(GL_TEXTURE1);
            //glBindTexture(GL_TEXTURE_2D, 0);
            //glActiveTexture(GL_TEXTURE2);
            //glBindTexture(GL_TEXTURE_2D, 0);
            //glActiveTexture(GL_TEXTURE3);
            //glBindTexture(GL_TEXTURE_2D, 0);
            //glActiveTexture(GL_TEXTURE4);
            //glBindTexture(GL_TEXTURE_2D, 0);

            //--------------------------------------DIFFUSE MAP TEXTURE--------------------------------------
            if (albedoMap.image.pixels && albedoMap.image.width > 0) {
                glGenTextures(1, &albedoMap.textureID); //Aqui se genera por OpenGL un ID para la textura
                glBindTexture(GL_TEXTURE_2D, albedoMap.textureID); //Bind-> abrir la textura para poder usarla para parametrizarla

                //Flags 
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

               /* if (albedoMap.image.channels == 3)
                {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                        albedoMap.image.width,
                        albedoMap.image.height,
                        0, GL_RGB, GL_UNSIGNED_BYTE, albedoMap.image.pixels);
                }
                else if (albedoMap.image.channels == 4)
                {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                        albedoMap.image.width,
                        albedoMap.image.height,
                        0, GL_RGBA, GL_UNSIGNED_BYTE, albedoMap.image.pixels);

                }*/


                GLenum format;
                if (albedoMap.image.channels == 1)
                    format = GL_RED;
                else if (albedoMap.image.channels == 3)
                    format = GL_RGB;
                else if (albedoMap.image.channels == 4)
                    format = GL_RGBA;
                glTexImage2D(GL_TEXTURE_2D, 0, format, albedoMap.image.width, albedoMap.image.height, 0, format, GL_UNSIGNED_BYTE, albedoMap.image.pixels);

                glGenerateMipmap(GL_TEXTURE_2D);

                glBindTexture(GL_TEXTURE_2D, 0);  // Unbind the texture
                hasAlbedoMap = true;
            }


            //--------------------------------------NORMAL MAP TEXTURE--------------------------------------
            if (normalMap.image.pixels && normalMap.image.width > 0) {
                glGenTextures(1, &normalMap.textureID);
                glBindTexture(GL_TEXTURE_2D, normalMap.textureID);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);


                GLenum format;
                if (normalMap.image.channels == 1)
                    format = GL_RED;
                else if (normalMap.image.channels == 3)
                    format = GL_RGB;
                else if (normalMap.image.channels == 4)
                    format = GL_RGBA;
                glTexImage2D(GL_TEXTURE_2D, 0, format, normalMap.image.width, normalMap.image.height, 0, format, GL_UNSIGNED_BYTE, normalMap.image.pixels);

                glGenerateMipmap(GL_TEXTURE_2D);

                glBindTexture(GL_TEXTURE_2D, 0);  // Unbind the texture
                hasNormalMap = true;
            }





            //--------------------------------------METALLIC MAP TEXTURE--------------------------------------
            if (metallicMap.image.pixels && metallicMap.image.width > 0) {
                glGenTextures(1, &metallicMap.textureID); //Aqui se genera por OpenGL un ID para la textura
                glBindTexture(GL_TEXTURE_2D, metallicMap.textureID); //Bind-> abrir la textura para poder usarla para parametrizarla

                //Flags 
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);


                std::cout << "Canales: " << metallicMap.image.channels << std::endl;
                std::cout << "Ancho: " << metallicMap.image.width << std::endl;
                std::cout << "Alto: " << metallicMap.image.height << std::endl;

                GLenum format;
                if (metallicMap.image.channels == 1)
                    format = GL_RED;
                else if (metallicMap.image.channels == 3)
                    format = GL_RGB;
                else if (metallicMap.image.channels == 4)
                    format = GL_RGBA;
                glTexImage2D(GL_TEXTURE_2D, 0, format, metallicMap.image.width, metallicMap.image.height, 0, format, GL_UNSIGNED_BYTE, metallicMap.image.pixels);

                glGenerateMipmap(GL_TEXTURE_2D);

                glBindTexture(GL_TEXTURE_2D, 0);  // Unbind the texture
                hasMetallicMap = true;
            }



            //--------------------------------------ROUGHNESS MAP TEXTURE--------------------------------------
            if (rougnessMap.image.pixels && rougnessMap.image.width > 0) {
                glGenTextures(1, &rougnessMap.textureID); //Aqui se genera por OpenGL un ID para la textura
                glBindTexture(GL_TEXTURE_2D, rougnessMap.textureID); //Bind-> abrir la textura para poder usarla para parametrizarla

                //Flags 
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

                std::cout << "Canales: " << rougnessMap.image.channels << std::endl;
                std::cout << "Ancho: " << rougnessMap.image.width << std::endl;
                std::cout << "Alto: " << rougnessMap.image.height << std::endl;

                GLenum format;
                if (rougnessMap.image.channels == 1)
                    format = GL_RED;
                else if (rougnessMap.image.channels == 3)
                    format = GL_RGB;
                else if (rougnessMap.image.channels == 4)
                    format = GL_RGBA;
                glTexImage2D(GL_TEXTURE_2D, 0, format, rougnessMap.image.width, rougnessMap.image.height, 0, format, GL_UNSIGNED_BYTE, rougnessMap.image.pixels);

                glGenerateMipmap(GL_TEXTURE_2D);

                glBindTexture(GL_TEXTURE_2D, 0);  // Unbind the texture
                hasRougnessMap = true;
            }


            //--------------------------------------AO MAP TEXTURE--------------------------------------
            if (aOMap.image.pixels && aOMap.image.width > 0) {

                glGenTextures(1, &aOMap.textureID);
                glBindTexture(GL_TEXTURE_2D, aOMap.textureID);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

                GLenum format;
                if (aOMap.image.channels == 1)
                    format = GL_RED;
                else if (aOMap.image.channels == 3)
                    format = GL_RGB;
                else if (aOMap.image.channels == 4)
                    format = GL_RGBA;
                glTexImage2D(GL_TEXTURE_2D, 0, format, aOMap.image.width, aOMap.image.height, 0, format, GL_UNSIGNED_BYTE, aOMap.image.pixels);

                glGenerateMipmap(GL_TEXTURE_2D);

                glBindTexture(GL_TEXTURE_2D, 0);  // Unbind the texture
                hasAoMap = true;
            }
        }
	};




    struct ModelInfo
    {
        MeshInfo model_mesh;
        Material model_material;

        // Global Bounding Box
        //unsigned int VBO_BB, VAO_BB;
        //glm::vec3 globalBoundingBoxMin; // Minimo global
        //glm::vec3 globalBoundingBoxMax; // Maximo global
    };

    struct ModelParent
    {
        std::string name;
        std::vector<ModelInfo> modelInfos;  // Contiene la información de todas las mallas
    };
}
