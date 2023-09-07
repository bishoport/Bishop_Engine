#pragma once

#include "glpch.h"
#include "ECSCore.h"

#include <GLCore/Core/Render/Shader.h>
#include <GLCore/DataStructs.h>
#include <vector>

using namespace std;
using namespace glm;


class GridWorldReferenceComp : public ECS::Component
{
public:
    GLCore::Shader* debugShader;
    int slices = 10;
    float sizeMultiplicator = 10.0f;
    bool showGrid = true;
    

    void init() override
    {
        //-----------------------------------------------GRID----------------------------------------
        std::vector<glm::vec3> gridVertices;
        std::vector<glm::uvec4> gridIndices;

        //Vertical lines
        for (int j = 0; j <= slices; ++j) {
            for (int i = 0; i <= slices; ++i) {
                float x = (float)i / (float)slices * sizeMultiplicator;
                float y = 0;
                float z = (float)j / (float)slices * sizeMultiplicator;
                gridVertices.push_back(glm::vec3(x - sizeMultiplicator / 2, y, z - sizeMultiplicator / 2));
            }
        }

        //Horizontal lines
        for (int j = 0; j < slices; ++j) {
            for (int i = 0; i < slices; ++i) {

                int row1 = j * (slices + 1);
                int row2 = (j + 1) * (slices + 1);

                gridIndices.push_back(glm::uvec4(row1 + i, row1 + i + 1, row1 + i + 1, row2 + i + 1));
                gridIndices.push_back(glm::uvec4(row2 + i + 1, row2 + i, row2 + i, row1 + i));
            }
        }

        glGenVertexArrays(1, &gridVAO);
        glBindVertexArray(gridVAO);

        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(glm::vec3), glm::value_ptr(gridVertices[0]), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        GLuint ibo;
        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, gridIndices.size() * sizeof(glm::uvec4), glm::value_ptr(gridIndices[0]), GL_STATIC_DRAW);

        glBindVertexArray(0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        lenght = (GLuint)gridIndices.size() * 4;
        //-------------------------------------------------------------------------------------------------------



        //-----------------------------------------------WORLD AXES----------------------------------------
        // Ahora generamos los ejes
        std::vector<glm::vec3> axesVertices = {
            glm::vec3(0.0f, 0.0f, 0.0f), // Origen
            glm::vec3(1.0f, 0.0f, 0.0f), // Eje X
            glm::vec3(0.0f, 0.0f, 0.0f), // Origen
            glm::vec3(0.0f, 1.0f, 0.0f), // Eje Y
            glm::vec3(0.0f, 0.0f, 0.0f), // Origen
            glm::vec3(0.0f, 0.0f, 1.0f)  // Eje Z
        };

        glGenVertexArrays(1, &axesVAO);
        glBindVertexArray(axesVAO);

        GLuint axesVBO;
        glGenBuffers(1, &axesVBO);
        glBindBuffer(GL_ARRAY_BUFFER, axesVBO);
        glBufferData(GL_ARRAY_BUFFER, axesVertices.size() * sizeof(glm::vec3), glm::value_ptr(axesVertices[0]), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        //-------------------------------------------------------------------------------------------------------
    }

    void update() override{}

    void draw() override
    {
        //-----------------------------------------------GRID----------------------------------------
        if (showGrid)
        {
            glBindVertexArray(gridVAO);

            glm::mat4 transform{ glm::mat4(1.0f) };

            /* Apply object's transformation matrix */
            debugShader->use();
            debugShader->setMat4("model", transform);
            debugShader->setVec4("u_Color", glm::vec4(0.8, 0.8, 0.8, 1.0));

            glLineWidth(3);
            glDrawElements(GL_LINES, lenght, GL_UNSIGNED_INT, NULL);
            glBindVertexArray(0);
        

            //-----------------------------------------------WORLD AXIS----------------------------------------
            glBindVertexArray(axesVAO);

            glm::mat4 transformAxes{ glm::mat4(1.0f) };
            transformAxes = glm::translate(transformAxes, glm::vec3(-sizeMultiplicator / 2, 1.0f, -sizeMultiplicator / 2)); // traslación a la esquina de la rejilla

            debugShader->setMat4("model", transformAxes);
            debugShader->setVec4("u_Color", glm::vec4(1.0, 0.5, 0.2, 1.0));
            glLineWidth(5);
            glDrawArrays(GL_LINES, 0, 6);
            glBindVertexArray(0);
        }
        //------------------------------------------------------------------------------------------
    }





    void onDestroy() override
    {
        glDeleteVertexArrays(1, &gridVAO);
        glDeleteVertexArrays(1, &axesVAO);
    }


    private:
        unsigned int axesVAO;
        unsigned int gridVAO;
        vector<float> vertices;
        GLuint lenght;
};