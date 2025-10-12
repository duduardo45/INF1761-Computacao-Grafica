#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <gl_base/transform.h>

class PhysicsBody;
using PhysicsBodyPtr = std::shared_ptr<PhysicsBody>;

class PhysicsBody
{
private:
    glm::vec2 positionOld;
    glm::vec2 positionCurrent;
    glm::vec2 acceleration;
    float radius;
    transform::TransformPtr nodeTransform;

    PhysicsBody(const glm::vec2 &oldPosition, const glm::vec2 &initialPosition, transform::TransformPtr nodeTransform, float radius)
        : positionOld(oldPosition), positionCurrent(initialPosition), acceleration(0.0f, 0.0f), radius(radius), nodeTransform(nodeTransform) {}

public:
    static PhysicsBodyPtr Make(const glm::vec2 &initialPosition, transform::TransformPtr nodeTransform, float radius = 1.0f)
    {
        return PhysicsBodyPtr(new PhysicsBody(initialPosition, initialPosition, nodeTransform, radius));
    }

    static PhysicsBodyPtr Make(const glm::vec2 &oldPosition, const glm::vec2 &initialPosition, transform::TransformPtr nodeTransform, float radius = 1.0f)
    {
        return PhysicsBodyPtr(new PhysicsBody(oldPosition, initialPosition, nodeTransform, radius));
    }

    void setNodeTransform(transform::TransformPtr t)
    {
        nodeTransform = t;
    }

    void calculateNextPosition(float deltaTime)
    {
        glm::vec2 velocity = positionCurrent - positionOld;
        positionOld = positionCurrent;
        positionCurrent += velocity + acceleration * deltaTime * deltaTime;
        acceleration = glm::vec2(0.0f, 0.0f);

        // Atualiza o transform do nó, se existir
        if (nodeTransform)
        {
            nodeTransform->setTranslate(positionCurrent.x, positionCurrent.y, 0.0f);
        }
    }

    void accelerate(const glm::vec2 &accel)
    {
        acceleration += accel;
    }
    glm::vec2 getPosition() const
    {
        return positionCurrent;
    }
    float getRadius() const
    {
        return radius;
    }
    void setRadius(float radius)
    {
        this->radius = radius;
    }
    void move(const glm::vec2 &newPosition)
    {
        positionCurrent += newPosition;
        // Atualiza o transform do nó, se existir
        if (nodeTransform)
        {
            nodeTransform->setTranslate(positionCurrent.x, positionCurrent.y, 0.0f);
        }
    }
    void moveOld(const glm::vec2 &newPosition)
    {
        positionOld += newPosition;
    }
};