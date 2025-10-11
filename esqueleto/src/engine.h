#pragma once
#include "physicsBody.h"
#include <glm/glm.hpp>

class Engine;
using EnginePtr = std::shared_ptr<Engine>;

class Engine
{
private:
    glm::vec2 gravity = glm::vec2(0.0f, -9.81f);
    int numSubsteps;
    std::vector<PhysicsBodyPtr> bodies;
    glm::vec2 areaMin = glm::vec2(-1.0f, -1.0f);
    glm::vec2 areaMax = glm::vec2(1.0f, 1.0f);
    
    Engine(int substeps) : numSubsteps(substeps) {}

    void constrainToArea(float minX, float maxX, float minY, float maxY, PhysicsBodyPtr body)
    {
        glm::vec2 pos = body->getPosition();
        float radius = body->getRadius();
        glm::vec2 correction(0.0f, 0.0f);

        if (pos.x - radius < minX)
        {
            correction.x = (minX + radius) - pos.x;
        }
        else if (pos.x + radius > maxX)
        {
            correction.x = (maxX - radius) - pos.x;
        }
        if (pos.y - radius < minY)
        {
            correction.y = (minY + radius) - pos.y;
        }
        else if (pos.y + radius > maxY)
        {
            correction.y = (maxY - radius) - pos.y;
        }
        body->move(correction);
    }


    void solveCollisions()
    {
        for (size_t i = 0; i < bodies.size(); i++)
        {
            for (size_t j = i + 1; j < bodies.size(); j++)
            {
                glm::vec2 posA = bodies[i]->getPosition();
                glm::vec2 posB = bodies[j]->getPosition();
                float distance = glm::length(posA - posB);
                float minDistance = bodies[i]->getRadius() + bodies[j]->getRadius();

                if (distance < minDistance && distance > 0.0f)
                {
                    glm::vec2 collisionNormal = glm::normalize(posB - posA);
                    glm::vec2 correction = collisionNormal * (minDistance - distance) * 0.5f;

                    bodies[i]->move(-correction);
                    bodies[j]->move(correction);
                }
            }
        }
    }
public:
    
    static EnginePtr make(int substeps = 5)
    {
        return EnginePtr (new Engine(substeps));
    }

    static EnginePtr make(float gravityX, float gravityY, int substeps = 5)
    {
        EnginePtr engine = Engine::make(substeps);
        engine->setGravity(glm::vec2(gravityX, gravityY));
        return engine;
    }

    static EnginePtr make(glm::vec2 gravity, int substeps = 5)
    {
        EnginePtr engine = Engine::make(substeps);
        engine->setGravity(gravity);
        return engine;
    }

    static EnginePtr make(glm::vec2 minArea, glm::vec2 maxArea, glm::vec2 gravity = glm::vec2(0.0f, -9.81f), int substeps = 5)
    {
        EnginePtr engine = Engine::make(substeps);
        engine->setArea(minArea, maxArea);
        engine->setGravity(gravity);
        return engine;
    }
    static EnginePtr make(float minX, float maxX, float minY, float maxY, glm::vec2 gravity = glm::vec2(0.0f, -9.81f), int substeps = 5)
    {
        EnginePtr engine = Engine::make(substeps);
        engine->setArea(minX, maxX, minY, maxY);
        engine->setGravity(gravity);
        return engine;
    }

    void update(float deltaTime)
    {
        for (int i = 0; i < numSubsteps; i++)
        {
            float substepDelta = deltaTime / static_cast<float>(numSubsteps);
            for (auto &body : bodies)
            {
                body->accelerate(gravity);
                constrainToArea(areaMin.x,areaMax.x,areaMin.y,areaMax.y, body); // Constrain within a default area
                solveCollisions();
                body->calculateNextPosition(substepDelta);
            }
        }
    }

    void multiplyGravity(float factor)
    {
        gravity *= factor;
    }

    void setGravity(glm::vec2 gravity)
    {
        this->gravity = gravity;
    }

    void addBody(PhysicsBodyPtr body)
    {
        bodies.push_back(body);
    }

    void clearBodies()
    {
        bodies.clear();
    }

    void setArea(float minX, float maxX, float minY, float maxY)
    {
        areaMin = glm::vec2(minX, minY);
        areaMax = glm::vec2(maxX, maxY);
    }

    void setArea(glm::vec2 min, glm::vec2 max)
    {
        areaMin = min;
        areaMax = max;
    }
};