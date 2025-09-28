
#pragma once

#pragma once
#include <glm/glm.hpp>
#include <memory>

class PhysicsBody;
using PhysicsBodyPtr = std::shared_ptr<PhysicsBody>;

class PhysicsBody
{
private:
    glm::vec2 positionOld;
    glm::vec2 positionCurrent;
    glm::vec2 acceleration;

public:

    float radius = 1.0f;
    PhysicsBody(const glm::vec2 &initialPosition)
        : positionOld(initialPosition), positionCurrent(initialPosition), acceleration(0.0f, 0.0f) {}

    void calculateNextPosition(float deltaTime)
    {
        glm::vec2 velocity = positionCurrent - positionOld;
        positionOld = positionCurrent;
        positionCurrent += velocity + acceleration * deltaTime * deltaTime;
        acceleration = glm::vec2(0.0f, 0.0f);
    }

    void accelerate(const glm::vec2 &accel)
    {
        acceleration += accel;
    }
    glm::vec2 getPosition() const
    {
        return positionCurrent;
    }
    void setPosition(const glm::vec2 &newPosition)
    {
        positionCurrent = newPosition;
    }
};