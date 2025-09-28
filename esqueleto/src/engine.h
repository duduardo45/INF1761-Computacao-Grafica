#pragma once
#include "physicsBody.h"

class Engine
{
private:
    glm::vec2 gravity = glm::vec2(0.0f, -9.81f);
    int numSubsteps;

public:
    std::vector<PhysicsBodyPtr> bodies;
    Engine(){}
    Engine(int substeps = 5) : numSubsteps(substeps) {}
    void update(float deltaTime)
    {
        for (int i = 0; i < numSubsteps; i++)
        {
            float substepDelta = deltaTime / static_cast<float>(numSubsteps);
            for (auto &body : bodies)
            {
                body->accelerate(gravity);
                constrainToArea(-1.0f, 1.0f, -1.0f, 1.0f, body); // Constrain within a default area
                solveCollisions();
                body->calculateNextPosition(substepDelta);
            }
        }
    }

    void addBody(PhysicsBodyPtr body)
    {
        bodies.push_back(body);
    }

    void clearBodies()
    {
        bodies.clear();
    }

    //
    void constrainToArea(float minX, float maxX, float minY, float maxY, PhysicsBodyPtr body)
    {

        glm::vec2 pos = body->getPosition();
        if (pos.x < minX)
        {
            pos.x = minX;
        }
        else if (pos.x > maxX)
        {
            pos.x = maxX;
        }
        if (pos.y < minY)
        {
            pos.y = minY;
        }
        else if (pos.y > maxY)
        {
            pos.y = maxY;
        }
        body->setPosition(pos);
    }

    void multiplyGravity(float factor)
    {
        gravity *= factor;
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
                float minDistance = bodies[i]->radius + bodies[j]->radius;

                if (distance < minDistance && distance > 0.0f)
                {
                    glm::vec2 collisionNormal = glm::normalize(posB - posA);
                    glm::vec2 correction = collisionNormal * (minDistance - distance) * 0.5f;

                    bodies[i]->setPosition(posA - correction);
                    bodies[j]->setPosition(posB + correction);
                }
            }
        }
    }
};