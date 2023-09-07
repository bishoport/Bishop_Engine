#pragma once
#include "glpch.h"


#include <vector>
#include <glm/glm.hpp>

namespace GLCore::Utils {

    class MeshHelper
    {
    public:

        static std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<GLuint>> createSphere(float radius, int stacks, int slices)
        {
            std::vector<glm::vec3> vertices;
            std::vector<glm::vec3> normals;
            std::vector<GLuint> indices;

            float stackAngle = glm::pi<float>() / stacks;
            float sliceAngle = 2.0f * glm::pi<float>() / slices;

            for (int i = 0; i <= stacks; ++i)
            {
                float phi = i * stackAngle;
                float sinPhi = glm::sin(phi);
                float cosPhi = glm::cos(phi);

                for (int j = 0; j <= slices; ++j)
                {
                    float theta = j * sliceAngle;
                    float sinTheta = glm::sin(theta);
                    float cosTheta = glm::cos(theta);

                    float x = radius * sinPhi * cosTheta;
                    float y = radius * sinPhi * sinTheta;
                    float z = radius * cosPhi;

                    glm::vec3 vertex(x, y, z);
                    glm::vec3 normal = glm::normalize(vertex);

                    vertices.push_back(vertex);
                    normals.push_back(normal);
                }
            }

            for (int i = 0; i < stacks; ++i)
            {
                for (int j = 0; j < slices; ++j)
                {
                    int first = i * (slices + 1) + j;
                    int second = first + slices + 1;

                    indices.push_back(first);
                    indices.push_back(second);
                    indices.push_back(first + 1);

                    indices.push_back(second);
                    indices.push_back(second + 1);
                    indices.push_back(first + 1);
                }
            }

            return std::make_tuple(vertices, normals, indices);
        }








        static void drawMesh(const std::vector<glm::vec3>& vertices, const std::vector<glm::vec3>& normals, const std::vector<GLuint>& indices)
        {
            GLuint vao;
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            GLuint vbo;
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(0);

            GLuint nbo;
            glGenBuffers(1, &nbo);
            glBindBuffer(GL_ARRAY_BUFFER, nbo);
            glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(1);

            GLuint ebo;
            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);
        }
    };
}