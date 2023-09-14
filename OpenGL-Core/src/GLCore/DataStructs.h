#pragma once
#include "glpch.h"
#include <ECS/ECS.h>
#include <memory>

namespace GLCore 
{
    struct Plane
    {
        glm::vec3 normal = { 0.f, 1.f, 0.f }; // unit vector
        float     distance = 0.f;        // Distance with origin

        Plane() = default;

        Plane(const glm::vec3& p1, const glm::vec3& norm)
            : normal(glm::normalize(norm)),
            distance(glm::dot(normal, p1))
        {}

        float getSignedDistanceToPlane(const glm::vec3& point) const
        {
            return glm::dot(normal, point) - distance;
        }
    };

    struct Frustum
    {
        Plane topFace;
        Plane bottomFace;

        Plane rightFace;
        Plane leftFace;

        Plane farFace;
        Plane nearFace;
    };

    struct BoundingVolume
    {
        virtual bool isOnFrustum(const Frustum& camFrustum, const ECS::Transform& transform) const = 0;

        virtual bool isOnOrForwardPlane(const Plane& plane) const = 0;

        bool isOnFrustum(const Frustum& camFrustum) const
        {
            return (isOnOrForwardPlane(camFrustum.leftFace) &&
                isOnOrForwardPlane(camFrustum.rightFace) &&
                isOnOrForwardPlane(camFrustum.topFace) &&
                isOnOrForwardPlane(camFrustum.bottomFace) &&
                isOnOrForwardPlane(camFrustum.nearFace) &&
                isOnOrForwardPlane(camFrustum.farFace));
        };
    };

    struct AABB : public BoundingVolume
    {
        glm::vec3 center{ 0.f, 0.f, 0.f };
        glm::vec3 extents{ 0.f, 0.f, 0.f };

        glm::vec3 m_max{ 0.f, 0.f, 0.f };
        glm::vec3 m_min{ 0.f, 0.f, 0.f };

        AABB() = default; // Constructor por defecto

        AABB(const glm::vec3& min, const glm::vec3& max): BoundingVolume{}, center{ (max + min) * 0.5f }, extents{ max.x - center.x, max.y - center.y, max.z - center.z }
        {
            m_max = max;
            m_min = min;
        }

        AABB(const glm::vec3& inCenter, float iI, float iJ, float iK): BoundingVolume{}, center{ inCenter }, extents{ iI, iJ, iK }
        {}

        std::array<glm::vec3, 8> getVertice() const
        {
            std::array<glm::vec3, 8> vertice;
            vertice[0] = { center.x - extents.x, center.y - extents.y, center.z - extents.z };
            vertice[1] = { center.x + extents.x, center.y - extents.y, center.z - extents.z };
            vertice[2] = { center.x - extents.x, center.y + extents.y, center.z - extents.z };
            vertice[3] = { center.x + extents.x, center.y + extents.y, center.z - extents.z };
            vertice[4] = { center.x - extents.x, center.y - extents.y, center.z + extents.z };
            vertice[5] = { center.x + extents.x, center.y - extents.y, center.z + extents.z };
            vertice[6] = { center.x - extents.x, center.y + extents.y, center.z + extents.z };
            vertice[7] = { center.x + extents.x, center.y + extents.y, center.z + extents.z };
            return vertice;
        }

        //see https://gdbooks.gitbooks.io/3dcollisions/content/Chapter2/static_aabb_plane.html
        bool isOnOrForwardPlane(const Plane& plane) const final
        {
            // Compute the projection interval radius of b onto L(t) = b.c + t * p.n
            const float r = extents.x * std::abs(plane.normal.x) + extents.y * std::abs(plane.normal.y) +
                extents.z * std::abs(plane.normal.z);

            return -r <= plane.getSignedDistanceToPlane(center);
        }

        bool isOnFrustum(const Frustum& camFrustum, const ECS::Transform& transform) const final
        {
            //Get global scale thanks to our transform
            const glm::vec3 globalCenter{ transform.getModelMatrix() * glm::vec4(center, 1.f) };

            // Scaled orientation
            const glm::vec3 right = transform.getRight() * extents.x;
            const glm::vec3 up = transform.getUp() * extents.y;
            const glm::vec3 forward = transform.getForward() * extents.z;

            const float newIi = std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, right)) +
                std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, up)) +
                std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, forward));

            const float newIj = std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, right)) +
                std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, up)) +
                std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, forward));

            const float newIk = std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, right)) +
                std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, up)) +
                std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, forward));

            const AABB globalAABB(globalCenter, newIi, newIj, newIk);

            return (globalAABB.isOnOrForwardPlane(camFrustum.leftFace) &&
                globalAABB.isOnOrForwardPlane(camFrustum.rightFace) &&
                globalAABB.isOnOrForwardPlane(camFrustum.topFace) &&
                globalAABB.isOnOrForwardPlane(camFrustum.bottomFace) &&
                globalAABB.isOnOrForwardPlane(camFrustum.nearFace) &&
                globalAABB.isOnOrForwardPlane(camFrustum.farFace));
        };
    };


    struct VertexInfo {
        glm::vec3 position;
    };

	struct MeshInfo
	{

        // Constructor predeterminado
        MeshInfo() = default;

        std::string meshName;

		unsigned int num_indices = 0;
                                                                                                        //Vertice------normals
		std::vector<GLfloat> allBuffer;  //Secuenciados los datos de vetices normals textcords indices (1.0,2.0,0.0, 1.0,0.5,1.0, 2.3,4.5  ,2  )

		//std::vector<GLfloat> vertices;
        std::vector<VertexInfo> vertices;
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
        //std::unique_ptr<AABB> boundingVolume;

        AABB* boundingVolume;

        // 
        // Guarda los vértices originales en alguna parte
        glm::vec3 originalBoundingBoxMax;
        glm::vec3 originalBoundingBoxMin;

        glm::vec3 boundingBoxMin;
        glm::vec3 boundingBoxMax;

        glm::vec3 meshLocalPosition;
        glm::vec3 meshPosition;
        glm::vec3 meshRotation; // En ángulos de Euler
        glm::vec3 meshScale;


        AABB * generateAABB()
        {
            glm::vec3 minAABB = glm::vec3(std::numeric_limits<float>::max());
            glm::vec3 maxAABB = glm::vec3(std::numeric_limits<float>::min());
            
            for (unsigned int i = 0; i < vertices.size(); i += 3)
            {

                minAABB.x = std::min(minAABB.x, vertices[i].position.x);
                minAABB.y = std::min(minAABB.y, vertices[i].position.y);
                minAABB.z = std::min(minAABB.z, vertices[i].position.z);

                maxAABB.x = std::max(maxAABB.x, vertices[i].position.x);
                maxAABB.y = std::max(maxAABB.y, vertices[i].position.y);
                maxAABB.z = std::max(maxAABB.z, vertices[i].position.z);
            }
            return new AABB(minAABB, maxAABB);
        }

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


        void CalculateBoundingBox()
        {
            boundingVolume = generateAABB();
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

        //VALUES
        float normalIntensity = 0.5f;
        float metallicValue = 0.0f;
        float roughnessValue = 0.05f;
        float reflectanceValue = 0.04f;
        float fresnelCoefValue = 5.0f;

        void PrepareMaterial()
        {
            //--------------------------------------DIFFUSE MAP TEXTURE--------------------------------------
            if (albedoMap.image.pixels && albedoMap.image.width > 0) {
                glGenTextures(1, &albedoMap.textureID); //Aqui se genera por OpenGL un ID para la textura
                glBindTexture(GL_TEXTURE_2D, albedoMap.textureID); //Bind-> abrir la textura para poder usarla para parametrizarla

                //Flags 
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

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
        // Constructor predeterminado
        ModelInfo() = default;

        MeshInfo model_mesh;
        Material model_material;
    };

    struct ModelParent
    {
        std::string name;
        std::vector<ModelInfo> modelInfos;
    };
}
