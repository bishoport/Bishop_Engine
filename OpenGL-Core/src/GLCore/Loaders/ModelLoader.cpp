#include "glpch.h"
#include "ModelLoader.h"


#include "IMGLoader.h"
#include <ECSCore.h>

using namespace GLCore::Loaders;

namespace GLCore::Loaders
{
    std::string mFilePath;
    int modelCounter = 0;


    //Función recursiva que recorre el árbol de nodos de la escena
    ModelParent loadModelAssimp(const std::string& filePath, const std::string& fileName)
    {
        mFilePath = filePath;

        std::string completePathMesh = filePath + fileName;
        std::cout << "File ->" << completePathMesh << std::endl;

        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(completePathMesh,
                                                 aiProcess_GenNormals |
                                                 aiProcess_ValidateDataStructure |
                                                 aiProcess_Triangulate | 
                                                 aiProcess_FlipUVs | 
                                                 aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cout << "Error al cargar el modelo: " << importer.GetErrorString() << std::endl;
            throw std::runtime_error("Error al cargar el modelo");
        }

        std::cout << "Meshes->" << scene->mNumMeshes << std::endl;


        aiMatrix4x4 nodeTransform = scene->mRootNode->mTransformation;


        ModelParent modelParent;
        modelParent.name = fileName;  // Puedes cambiar esto por cualquier nombre que quieras para tu entidad


        // Llamamos a la función processNode
        processNode(scene->mRootNode, scene, modelParent, nodeTransform);

        return modelParent;
    }


    void processNode(aiNode* node, const aiScene* scene, ModelParent& modelParent, aiMatrix4x4 _nodeTransform)
    {
        // Convertir a glm
        glm::mat4 glmNodeTransform = aiMatrix4x4ToGlm(_nodeTransform);
        glm::mat4 glmNodeTransformation = aiMatrix4x4ToGlm(node->mTransformation);

        // Crear una matriz de rotación para la rotación de -90 grados alrededor del eje X
        glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        // Aplicar la rotación antes de la transformación del nodo
        glm::mat4 glmFinalTransform = rotationX * glmNodeTransform * glmNodeTransformation;

        // Convertir de nuevo a Assimp
        aiMatrix4x4 finalTransform = glmToAiMatrix4x4(glmFinalTransform);


        // Procesamos cada malla ubicada en el nodo actual
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // El objeto nodo solo contiene índices para indexar los objetos reales en la escena.
            // La escena contiene todos los datos
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

            // Pasamos el globalInverseTransform a la función donde extraemos los vértices, normales, etc.
            ModelInfo modelInfo = processMesh(mesh, scene, finalTransform);

            modelParent.modelInfos.push_back(modelInfo);
        }


        // Después de haber procesado todas las mallas (si las hay) procesamos recursivamente cada uno de los nodos hijos
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene, modelParent, _nodeTransform);
        }
    }



    ModelInfo processMesh(aiMesh* mesh, const aiScene* scene,  aiMatrix4x4 finalTransform)
    {
        ModelInfo modelInfo;
        MeshInfo meshInfo;

        meshInfo.meshLocalPosition = glm::vec3(finalTransform.a4, finalTransform.b4, finalTransform.c4);

        //Reset de la posicion original para que nos devuelva la matriz en la posicion 0,0,0
        finalTransform.a4 = 0.0;
        finalTransform.b4 = 0.0;
        finalTransform.c4 = 0.0;


        //-SACAMOS LOS INDICES
        for (unsigned int j = 0; j < mesh->mNumFaces; j++)
        {
            aiFace face = mesh->mFaces[j];
            for (unsigned int k = 0; k < face.mNumIndices; k++) {
                meshInfo.indices.push_back(face.mIndices[k]);
            }
        }
        //------------------------------------------------------




        //-Recorremos todos los indices
        for (unsigned int j = 0; j < meshInfo.indices.size(); j++)
        {

            //-SACAMOS LOS VERTICES
            glm::vec4 pos = aiMatrix4x4ToGlm(finalTransform) * glm::vec4(
                mesh->mVertices[meshInfo.indices[j]].x,
                mesh->mVertices[meshInfo.indices[j]].y,
                mesh->mVertices[meshInfo.indices[j]].z,
                1);

            meshInfo.vertices.push_back(pos.x);
            meshInfo.vertices.push_back(pos.y);
            meshInfo.vertices.push_back(pos.z);


            //-SACAMOS LAS NORMALES
            glm::vec3 normal = vec3(0.0, 0.0, 0.0);
            if (mesh->HasNormals())
            {
                normal = aiMatrix4x4ToGlm(finalTransform) * glm::vec4(
                    mesh->mNormals[meshInfo.indices[j]].x,
                    mesh->mNormals[meshInfo.indices[j]].y,
                    mesh->mNormals[meshInfo.indices[j]].z,
                    0); // Este valor debería ser 0 para las normales

                normal = glm::normalize(glm::mat3(1.0f) * normal);
            }


            //-SACAMOS LAS COORDENADAS DE MAPEADO
            glm::vec2 texcoord = vec2(0.0f, 0.0f);
            if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                texcoord = {
                    mesh->mTextureCoords[0][meshInfo.indices[j]].x,
                    mesh->mTextureCoords[0][meshInfo.indices[j]].y
                };
            }


            //-SACAMOS LAS TANGENTES
            glm::vec4 tangent   = vec4(0.0f);
            glm::vec4 bitangent = vec4(0.0f);
            if (mesh->HasTangentsAndBitangents())
            {
                tangent = aiMatrix4x4ToGlm(finalTransform) * glm::vec4(
                    mesh->mTangents[meshInfo.indices[j]].x,
                    mesh->mTangents[meshInfo.indices[j]].y,
                    mesh->mTangents[meshInfo.indices[j]].z,
                    0); // Este valor debería ser 0 para las tangentes

                meshInfo.tangents.push_back(tangent.x);
                meshInfo.tangents.push_back(tangent.y);
                meshInfo.tangents.push_back(tangent.z);


                //-SACAMOS LAS BITANGENTES
                bitangent = aiMatrix4x4ToGlm(finalTransform) * glm::vec4(
                    mesh->mBitangents[meshInfo.indices[j]].x,
                    mesh->mBitangents[meshInfo.indices[j]].y,
                    mesh->mBitangents[meshInfo.indices[j]].z,
                    0); // Este valor debería ser 0 para las bitangentes

                meshInfo.bitangents.push_back(bitangent.x);
                meshInfo.bitangents.push_back(bitangent.y);
                meshInfo.bitangents.push_back(bitangent.z);
            }





            //-ALMACENAMOS TODO EN EL BUFFER
            meshInfo.allBuffer.push_back(pos.x);
            meshInfo.allBuffer.push_back(pos.y);
            meshInfo.allBuffer.push_back(pos.z);

            meshInfo.allBuffer.push_back(texcoord.x);
            meshInfo.allBuffer.push_back(texcoord.y);

            meshInfo.allBuffer.push_back(normal.x);
            meshInfo.allBuffer.push_back(normal.y);
            meshInfo.allBuffer.push_back(normal.z);

            meshInfo.allBuffer.push_back(tangent.x);
            meshInfo.allBuffer.push_back(tangent.y);
            meshInfo.allBuffer.push_back(tangent.z);

            meshInfo.allBuffer.push_back(bitangent.x);
            meshInfo.allBuffer.push_back(bitangent.y);
            meshInfo.allBuffer.push_back(bitangent.z);


            // Actualizamos el número de índices
            meshInfo.num_indices += 1;
        }
        //-----------------------------------------------------------------------------





        //MESH NAME
        std::string meshNameBase = mesh->mName.C_Str();
        meshNameBase.append(" id:");
        meshInfo.meshName = meshNameBase + std::to_string(modelCounter);
        modelCounter += 1;

        meshInfo.CreateVAO();

        //Nos guardamos la información del modelo
        modelInfo.model_mesh = meshInfo;




        //-Almacenamos las transformaciones originales
        //aiVector3D position, scale;
        //aiQuaternion rotation;
        //finalTransform.Decompose(scale, rotation, position);

        //// Assimp's aiQuaternion has a different order of components than glm::quat.
        //// Assimp: w, x, y, z
        //// GLM:    x, y, z, w
        //glm::quat glmQuat(rotation.x, rotation.y, rotation.z, rotation.w);

        //meshInfo.meshPosition = glm::vec3(position.x, position.y, position.z);
        //
        //meshInfo.meshRotation = glm::eulerAngles(glmQuat);
        //meshInfo.meshScale = glm::vec3(scale.x, scale.y, scale.z);
        // 
        // 
        //---------------------------------------------------------------------------









        //EMPIEZAN LOS MATERIALES
        const aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];

        Material material;

        aiString texturePath;

        //COLOR DIFUSSE
        aiColor3D color(0.f, 0.f, 0.f);
        mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
        material.albedoColor.r = color.r;
        material.albedoColor.g = color.g;
        material.albedoColor.b = color.b;


        // Agregamos la carga de la textura DIFFUSSE aquí
        if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS)
        {
            std::string completePathTexture = mFilePath + texturePath.C_Str();
            std::cout << "-------------------------------------------" << std::endl;
            std::cout << "Diffuse Texture->" << completePathTexture << std::endl;

            Texture diffuseTexture;
            diffuseTexture.image = GLCore::Loaders::getOrLoadImage(completePathTexture.c_str());
            diffuseTexture.image.path = texturePath.C_Str();

            material.albedoMap = diffuseTexture;
            std::cout << "-------------------------------------------" << std::endl;
        }




        // Agregamos la carga de la textura NORMAL aquí
        if (mat->GetTexture(aiTextureType_NORMALS, 0, &texturePath) == AI_SUCCESS)
        {
            std::string completePathTexture = mFilePath + texturePath.C_Str();
            std::cout << "Normal Map Texture->" << completePathTexture << std::endl;

            Texture normalTexture;
            normalTexture.image = GLCore::Loaders::getOrLoadImage(completePathTexture.c_str());
            normalTexture.image.path = texturePath.C_Str();

            material.normalMap = normalTexture;

            std::cout << "-------------------------------------------" << std::endl;
        }
        else
        {
            std::string completePathTexture = "assets/default/default_normal.jpg";
            std::cout << "Normal Map Texture->" << completePathTexture << std::endl;

            Texture normalTexture;
            normalTexture.image = GLCore::Loaders::getOrLoadImage(completePathTexture.c_str());
            normalTexture.image.path = texturePath.C_Str();

            material.normalMap = normalTexture;

            std::cout << "-------------------------------------------" << std::endl;
        }




        // Agregamos la carga de la textura METALLIC aquí
        if (mat->GetTexture(aiTextureType_METALNESS, 0, &texturePath) == AI_SUCCESS)
        {
            std::string completePathTexture = mFilePath + texturePath.C_Str();
            std::cout << "METALLIC Map Texture->" << completePathTexture << std::endl;

            Texture metallicTexture;
            metallicTexture.image = GLCore::Loaders::getOrLoadImage(completePathTexture.c_str());
            metallicTexture.image.path = texturePath.C_Str();

            material.metallicMap = metallicTexture; 

            std::cout << "-------------------------------------------" << std::endl;
        }




        // Agregamos la carga de la textura ROUGHTNESS aquí
        if (mat->GetTexture(aiTextureType_SHININESS, 0, &texturePath) == AI_SUCCESS)
        {
            std::string completePathTexture = mFilePath + texturePath.C_Str();
            std::cout << "ROUGHTNESS Map Texture->" << completePathTexture << std::endl;

            Texture roughtnessTexture;
            roughtnessTexture.image = GLCore::Loaders::getOrLoadImage(completePathTexture.c_str());
            roughtnessTexture.image.path = texturePath.C_Str();

            material.rougnessMap = roughtnessTexture;

            std::cout << "-------------------------------------------" << std::endl;
        }




        // Agregamos la carga de la textura AO aquí
        if (mat->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &texturePath) == AI_SUCCESS)
        {
            std::string completePathTexture = mFilePath + texturePath.C_Str();
            std::cout << "AO Map Texture->" << completePathTexture << std::endl;

            Texture aoTexture;
            aoTexture.image = GLCore::Loaders::getOrLoadImage(completePathTexture.c_str());
            aoTexture.image.path = texturePath.C_Str();

            material.aOMap = aoTexture;

            std::cout << "-------------------------------------------" << std::endl;
        }








        //// Agregamos la carga de la textura SPECULAR aquí
        //if (mat->GetTexture(aiTextureType_SPECULAR, 0, &texturePath) == AI_SUCCESS)
        //{
        //    std::string completePathTexture = mFilePath + texturePath.C_Str();
        //    std::cout << "Specular Map Texture->" << completePathTexture << std::endl;

        //    Texture specularTexture;
        //    specularTexture.image = GLCore::Loaders::getOrLoadImage(completePathTexture.c_str());
        //    specularTexture.image.path = texturePath.C_Str();

        //    material.specularMap = specularTexture; // Asegúrate de tener specularMap en tu estructura Material

        //    std::cout << "-------------------------------------------" << std::endl;
        //}



        //Nos guardamos la información del material
        modelInfo.model_material = material;


        return modelInfo;
    }
    //------------------------------------------------------------------------------------------------------------------------------------------------------------





    // Funciones de conversión
    glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4& from)
    {
        glm::mat4 to;

        // Transponer y convertir a glm
        to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
        to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
        to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
        to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;

        return to;
    }

    aiMatrix4x4 glmToAiMatrix4x4(const glm::mat4& from)
    {
        aiMatrix4x4 to;

        // Transponer y convertir a Assimp
        to.a1 = from[0][0]; to.a2 = from[1][0]; to.a3 = from[2][0]; to.a4 = from[3][0];
        to.b1 = from[0][1]; to.b2 = from[1][1]; to.b3 = from[2][1]; to.b4 = from[3][1];
        to.c1 = from[0][2]; to.c2 = from[1][2]; to.c3 = from[2][2]; to.c4 = from[3][2];
        to.d1 = from[0][3]; to.d2 = from[1][3]; to.d3 = from[2][3]; to.d4 = from[3][3];

        return to;
    }
}















//-----------------------------------------AQUI Leemos solo las texturas, sirve para cargar las texturas embebidas
       //Pero por ahora no lo consigo hacer funcionar y tampoco se si es util en todos los casos.
       //std::vector<Texture> embeddedTextures;
       //// Check if scene has textures.

       //if (scene->HasTextures())
       //{
       //    // Cargar todas las texturas embebidas
       //    for (size_t ti = 0; ti < scene->mNumTextures; ti++)
       //    {
       //        Texture diffuseTexture;
       //        diffuseTexture.image = GLCore::Loaders::Load_from_memory(scene->mTextures[ti]);
       //        diffuseTexture.image.path = scene->mTextures[ti]->mFilename.C_Str();
       //        diffuseTexture.textureID = ti;
       //        embeddedTextures.push_back(diffuseTexture);
       //    }

       //    //Esto funciona
       //    /*Material material;

       //    std::cout << "Textures->" << scene->mNumTextures << std::endl;

       //    for (size_t ti = 0; ti < scene->mNumTextures; ti++)
       //    {

       //        Texture diffuseTexture;
       //        diffuseTexture.image = GLCore::Loaders::Load_from_memory(scene->mTextures[ti]);
       //        diffuseTexture.image.path = "embebed texture";

       //        material.albedoMap = diffuseTexture;
       //        modelInfo.materials.push_back(material);
       //    }*/
       //}

       //if (scene->HasMaterials())
       //{
       //    for (unsigned int i = 0; i < scene->mNumMaterials; i++)
       //    {
       //        const aiMaterial* mat = scene->mMaterials[i];
       //        Material material;

       //        aiString texturePath;

       //        if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0 && mat->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS)
       //        {
       //            material.albedoMap = embeddedTextures[i];
       //        }

       //        modelInfo.materials.push_back(material);
       //    }
       //}
       //------------------------------------------------------------------------------------------------------------------------------------------------