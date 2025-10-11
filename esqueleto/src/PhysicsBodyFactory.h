#pragma once

#include "engine.h"
#include "scene.h"
#include "physicsBody.h"
#include "circle.h"
#include "shader.h"
#include "transform.h"

#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>

class PhysicsBodyFactory;
using PhysicsBodyFactoryPtr = std::shared_ptr<PhysicsBodyFactory>;

class PhysicsBodyFactory
{
private:
    EnginePtr engine;
    scene::SceneGraphPtr sceneGraph;
    shader::ShaderPtr shader;
    scene::NodePtr parentNode;

    PhysicsBodyFactory(
        EnginePtr engine,
        scene::SceneGraphPtr graph,
        scene::NodePtr parent,
        shader::ShaderPtr shaderPtr
    )
        : engine(engine), sceneGraph(graph), parentNode(parent)
    {
        if (!shaderPtr)
            this->shader = shader::Shader::Make();
        else
            this->shader = shaderPtr;
    }
public:

     static PhysicsBodyFactoryPtr make(
        EnginePtr engine,
        scene::SceneGraphPtr graph,
        scene::NodePtr parent,
        shader::ShaderPtr shaderPtr = nullptr
    )
    {
        return std::make_shared<PhysicsBodyFactory>(engine, graph, parent, shaderPtr);
    }

    std::vector<PhysicsBodyPtr> createMultiple(
        const std::string &baseName,
        const std::vector<glm::vec2> &positions,
        float radius = 0.05f,
        const float color[3] = nullptr,
        unsigned int edgePoints = 24
    )
    {
        std::vector<PhysicsBodyPtr> createdBodies;

        scene::NodePtr effectiveParent = parentNode;
        float defaultColor[3] = {1.0f, 1.0f, 1.0f};
        const float* colorPtr = (color) ? color : defaultColor;

        int counter = 0;
        for (const auto &pos : positions)
        {
            std::string nodeName = baseName + "_" + std::to_string(counter++);

            float colorCopy[3] = { colorPtr[0], colorPtr[1], colorPtr[2] };
            auto circleShape = Circle::Make(0.0f, 0.0f, radius, colorCopy, edgePoints, true);

            auto transform = transform::Transform::Make();
            transform->setTranslate(pos.x, pos.y, 0.0f);

            sceneGraph->addNode(nodeName, circleShape, shader, transform, effectiveParent);

            auto body = PhysicsBody::Make(pos, radius, transform);
            engine->addBody(body);

            createdBodies.push_back(body);
        }

        return createdBodies;
    }

    void setParentNode(scene::NodePtr newParent)
    {
        parentNode = newParent;
    }

    scene::NodePtr getParentNode() const
    {
        return parentNode;
    }
};
