#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include <bitset>
#include <array>
#include <glm/gtx/matrix_decompose.hpp>


namespace ECS
{

	class Component;
	class Entity;

	using ComponentID = std::size_t;

	inline ComponentID getComponentTypeID()
	{
		static ComponentID lastID = 0;
		return lastID++;
	}

	template <typename T> inline ComponentID getComponentTypeID() noexcept
	{
		static ComponentID typeID = getComponentTypeID();
		return typeID;
	}

	constexpr std::size_t maxComponents = 32;

	using ComponentBitSet = std::bitset<maxComponents>;
	using ComponentArray = std::array<Component*, maxComponents>;





	class Component
	{
	public:
		Entity* entity;
		virtual void init() {}
		virtual void update() {}
		virtual void draw() {}
		virtual void onDestroy() {}
		~Component() { onDestroy(); }
	};



	class Transform : public Component
	{
	public:
		glm::vec3 position = { 0.0f, 0.0f, 0.0f };
		glm::quat rotation; // Cambiamos 'eulers' por 'rotation'
		glm::vec3 scale = { 1.0f, 1.0f, 1.0f };

		Entity* parent;
		std::vector<Entity*> children;

		glm::mat4 GetTransform() const
		{
			return glm::translate(glm::mat4(1.0f), position)
				 * glm::toMat4(rotation)
				 * glm::scale(glm::mat4(1.0f), scale);
		}

		void SetTransform(const glm::mat4& transformMatrix)
		{
			// Descomponer la matriz de transformación
			glm::vec3 skew;
			glm::vec4 perspective;

			glm::decompose(transformMatrix, scale, rotation, position, skew, perspective);
			rotation = glm::conjugate(rotation);

			// Ya no necesitamos convertir la rotación a ángulos eulerianos
			// eulers = glm::eulerAngles(rotation);
		}

		// Función para obtener los ángulos de Euler
		glm::vec3 GetVec() const
		{
			return glm::eulerAngles(rotation);
		}
	};


	class Entity
	{
	private:
		
		std::vector<std::unique_ptr<Component>> components;

		

		ComponentArray componentArray;
		ComponentBitSet componentBitSet;

	public:
		std::string name;
		bool active = true;
		

		void update()
		{
			for (auto& c : components) c->update();
		}

		void draw()
		{
			for (auto& c : components) c->draw();
		}


		bool isActive() const { return active; }
		void destroy() { active = false; }

		template <typename T> bool hascomponent() const
		{
			if (&getComponent<T>() != NULL)
			{
				return true;
			}
			return false;

			//return ComponentBitSet[getComponentTypeID<T>];
		}

		template <typename T, typename... TArgs> T& addComponent(TArgs&&... mArgs)
		{
			T* c(new T(std::forward<TArgs>(mArgs)...));
			c->entity = this;
			std::unique_ptr<Component> uPtr{ c };
			components.emplace_back(std::move(uPtr));

			componentArray[getComponentTypeID<T>()] = c;
			componentBitSet[getComponentTypeID<T>()] = true;

			c->init();
			return *c;
		}


		template<typename T> T& getComponent() const
		{
			auto ptr(componentArray[getComponentTypeID<T>()]);
			return *static_cast<T*>(ptr);
		}
	};





	class Manager
	{
	private:
		std::vector<std::unique_ptr<Entity>> entities;
	public:
		void update()
		{
			for (auto& e : entities) e->update();
		}

		void draw()
		{
			for (auto& e : entities) e->draw();
		}

		void refresh()
		{
			entities.erase(std::remove_if(std::begin(entities), std::end(entities),
				[](const std::unique_ptr<Entity>& mEntity)
				{
					return !mEntity->isActive();
				}),
				std::end(entities));
		}

		Entity& addEntity()
		{
			Entity* e = new Entity();
			std::unique_ptr<Entity> uPtr{ e };
			entities.emplace_back(std::move(uPtr));
			
			float pi = 3.1415926535f;

			e->addComponent<Transform>();
			e->getComponent<Transform>().position = { 0.0f, 0.0f, 0.0f };
			e->getComponent<Transform>().scale = { 1.0f, 1.0f, 1.0f };

			// Inicializa el cuaternión de rotación con ángulos de Euler
			glm::vec3 eulers = { (0 * pi) / 180, (0 * pi) / 180, (0 * pi) / 180 };
			e->getComponent<Transform>().rotation = glm::quat(eulers);

			return *e;
		}

		// Devolver un vector de punteros crudos a las entidades
		std::vector<Entity*> getAllEntities()
		{
			std::vector<Entity*> rawEntities;
			for (auto& e : entities)
			{
				if(e.get()->active == true)
					rawEntities.push_back(e.get());
			}
			return rawEntities;
		}
	};
}




