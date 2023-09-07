 #pragma once
#include "glpch.h"
#include "ECSCore.h"

namespace ECS
{
    class CubeRotator : public ECS::Component
    {
        void update() override
        {
            //entity->getComponent<Transform>().eulers.y += 0.01f;
        }
    };
}

