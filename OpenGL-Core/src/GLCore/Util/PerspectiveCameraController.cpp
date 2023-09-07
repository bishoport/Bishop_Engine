#include "glpch.h"
#include "PerspectiveCameraController.h"

#include "GLCore/Core/Input.h"
#include "GLCore/Core/KeyCodes.h"


namespace GLCore::Utils {

    GLCore::Utils::PerspectiveCameraController::PerspectiveCameraController(float aspectRatio)
        : m_AspectRatio(aspectRatio), m_Camera()
    {
        m_Camera.SetProjection(45.0f, m_AspectRatio, 0.1f, 100.0f);
    }




    void GLCore::Utils::PerspectiveCameraController::OnUpdate(Timestep ts)
    {
        float cameraSpeed = m_CameraSpeed * ts;

        glm::vec3 cameraFront = m_Camera.GetFront();
        glm::vec3 cameraRight = m_Camera.GetRight();


        if (Input::IsKeyPressed(HZ_KEY_W))
            m_Camera.SetPosition(m_Camera.GetPosition() + cameraFront * cameraSpeed);
        if (Input::IsKeyPressed(HZ_KEY_S))
            m_Camera.SetPosition(m_Camera.GetPosition() - cameraFront * cameraSpeed);
        if (Input::IsKeyPressed(HZ_KEY_A))
            m_Camera.SetPosition(m_Camera.GetPosition() - cameraRight * cameraSpeed);
        if (Input::IsKeyPressed(HZ_KEY_D))
            m_Camera.SetPosition(m_Camera.GetPosition() + cameraRight * cameraSpeed);

        if (Input::IsKeyPressed(HZ_KEY_R))
            m_Camera.SetPosition(m_Camera.GetPosition() + glm::vec3(0.0f, 1.0f, 0.0f) * cameraSpeed);
        if (Input::IsKeyPressed(HZ_KEY_F))
            m_Camera.SetPosition(m_Camera.GetPosition() - glm::vec3(0.0f, 1.0f, 0.0f) * cameraSpeed);

        static bool wasSpacePressed = false;
        if (Input::IsKeyPressed(HZ_KEY_SPACE))
        {
            wasSpacePressed = true;
        }
        else if (wasSpacePressed)
        {
            m_FirstMouse = true;
            wasSpacePressed = false;
        }
    }

    void GLCore::Utils::PerspectiveCameraController::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseMovedEvent>(GLCORE_BIND_EVENT_FN(PerspectiveCameraController::OnMouseMoved));
        dispatcher.Dispatch<WindowResizeEvent>(GLCORE_BIND_EVENT_FN(PerspectiveCameraController::OnWindowResized));
    }

    bool GLCore::Utils::PerspectiveCameraController::OnMouseMoved(MouseMovedEvent& e)
    {
        if (Input::IsKeyPressed(HZ_KEY_SPACE))
        {
            float x = e.GetX();
            float y = e.GetY();

            if (m_FirstMouse)
            {
                m_LastMousePosition = { x, y };
                m_FirstMouse = false;
                return false; // Añadido para ignorar el primer movimiento del ratón
            }

            float xOffset = x - m_LastMousePosition.x;
            float yOffset = m_LastMousePosition.y - y; // Cambiar el signo para invertir la dirección
            m_LastMousePosition = { x, y };

            xOffset *= m_MouseSensitivity;
            yOffset *= m_MouseSensitivity;

            m_Camera.SetRotation(m_Camera.GetRotation() + glm::vec2(yOffset, xOffset));

            

        }

        return false;
    }


    bool GLCore::Utils::PerspectiveCameraController::OnWindowResized(WindowResizeEvent& e)
    {
        m_AspectRatio = (float)e.GetWidth() / (float)e.GetHeight();
        m_Camera.SetProjection(45.0f, m_AspectRatio, 0.1f, 100.0f);
        return false;
    }

} // namespace GLCore::Utils
