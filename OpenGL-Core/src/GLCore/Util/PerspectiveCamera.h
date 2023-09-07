#pragma once

#include <glm/glm.hpp>

namespace GLCore::Utils {

    class PerspectiveCamera
    {
    public:
        PerspectiveCamera();

        void SetProjection(float fov, float aspectRatio, float nearClip, float farClip);
        void SetPosition(const glm::vec3& position);
        void SetRotation(const glm::vec2& rotation);

        void SetFov(float fov);
        float GetFov() const { return m_Fov; }

        const glm::vec3& GetPosition() const { return m_Position; }
        const glm::vec3& GetFront() const { return m_Front; }
        const glm::vec3& GetRight() const { return m_Right; }
        const glm::vec3& GetUp() const { return m_Up; }
        const glm::vec2& GetRotation() const { return m_Rotation; }

        const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
        const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
        const glm::mat4& GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }

    private:
        void RecalculateViewMatrix();

    private:
        glm::mat4 m_ProjectionMatrix;
        glm::mat4 m_ViewMatrix;
        glm::mat4 m_ViewProjectionMatrix;

        glm::vec3 m_Position;
        glm::vec3 m_Front;
        glm::vec3 m_Up;
        glm::vec3 m_Right;
        glm::vec3 m_WorldUp;

        glm::vec2 m_Rotation;

        float m_Fov;
    };

} // namespace GLCore::Utils
