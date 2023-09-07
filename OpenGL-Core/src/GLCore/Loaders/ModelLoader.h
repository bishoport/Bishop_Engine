#pragma once
#include "glpch.h"
#include "../DataStructs.h"
#include <glad/glad.h>
#include <glm/glm.hpp>


#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

namespace GLCore::Loaders
{

    ModelParent loadModelAssimp(const std::string& filePath, const std::string& fileName);

    void processNode(aiNode* node, const aiScene* scene, ModelParent& modelParent, aiMatrix4x4 _nodeTransform);
    ModelInfo processMesh(aiMesh* mesh, const aiScene* scene , aiMatrix4x4 _nodeTransform);


    glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4& from);
    aiMatrix4x4 glmToAiMatrix4x4(const glm::mat4& from);
}
