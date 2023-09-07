#pragma once

#include "PerspectiveCamera.h"
#include "GLCore/Core/Timestep.h"

#include "GLCore/Events/ApplicationEvent.h"
#include "GLCore/Events/MouseEvent.h"

namespace GLCore::Utils {

    class PerspectiveCameraController
    {
    public:
        PerspectiveCameraController(float aspectRatio);

        void OnUpdate(Timestep ts);
        void OnEvent(Event& e);

        PerspectiveCamera& GetCamera() { return m_Camera; }
        const PerspectiveCamera& GetCamera() const { return m_Camera; }

    private:
        bool OnMouseMoved(MouseMovedEvent& e);
        bool OnWindowResized(WindowResizeEvent& e);

    private:
        float m_AspectRatio;
        float m_MouseSensitivity = 0.3f;
        PerspectiveCamera m_Camera;

        glm::vec2 m_LastMousePosition = { 0.0f, 0.0f };
        bool m_FirstMouse = true;

        float m_CameraSpeed = 10.5f;  // Añadir esto


    };

} // namespace GLCore::Utils
