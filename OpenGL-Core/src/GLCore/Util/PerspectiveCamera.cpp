#include "glpch.h"
#include "PerspectiveCamera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace GLCore::Utils {

    PerspectiveCamera::PerspectiveCamera()
        : m_Position(0.0f, 0.0f, 0.0f), 
          m_Front(0.0f, 0.0f, -1.0f), 
          m_Up(0.0f, 1.0f, 0.0f), 
          m_Right(0.0f, 0.0f, 0.0f),
          m_WorldUp(0.0f, 1.0f, 0.0f), 
          m_Rotation(0.0f, -90.0f)
    {
        m_Fov = 45.0f;
        SetProjection(45.0f, 16.0f / 9.0f, 0.1f, 100.0f);
        RecalculateViewMatrix();
    }

    void PerspectiveCamera::SetProjection(float fov, float aspectRatio, float nearClip, float farClip)
    {
        m_Fov = fov;
        m_ProjectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip);
        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }

    void PerspectiveCamera::SetPosition(const glm::vec3& position)
    {
        m_Position = position;
        RecalculateViewMatrix();
    }

    void PerspectiveCamera::SetRotation(const glm::vec2& rotation)
    {
        m_Rotation = rotation;
        RecalculateViewMatrix();
    }

    void PerspectiveCamera::SetFov(float fov) {
        m_Fov = fov;
        // Reconfigura la proyección con el nuevo FOV
        SetProjection(m_Fov, 16.0f / 9.0f, 0.1f, 100.0f);
    }

    void PerspectiveCamera::RecalculateViewMatrix()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(m_Rotation.y)) * cos(glm::radians(m_Rotation.x));
        front.y = sin(glm::radians(m_Rotation.x));
        front.z = sin(glm::radians(m_Rotation.y)) * cos(glm::radians(m_Rotation.x));
        m_Front = glm::normalize(front);

        m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
        m_Up = glm::normalize(glm::cross(m_Right, m_Front));

        m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_Front, m_Up);
        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }

} // namespace GLCore::Utils
