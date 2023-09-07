#pragma once

#include "Core.h"
#include "Timestep.h"
#include "../Events/Event.h"
#include <GLCore/Events/ApplicationEvent.h>

namespace GLCore {

	class Layer
	{
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(Timestep ts) {}
		virtual void OnImGuiRender() {}
		virtual void OnEvent(Event& event) {}


		virtual void OnWindowResizeEvent(WindowResizeEvent& event) {}
		virtual void OnWindowCloseEvent(WindowCloseEvent& event) {}


		inline const std::string& GetName() const { return m_DebugName; }
	protected:
		std::string m_DebugName;
	};

}