#ifndef SCENE_H
#define SCENE_H
#pragma once

#include "gl_includes.h"
#include "polygon.h"
#include "shape.h"
#include "transform.h"
#include <vector>
#include <memory>
#include <cmath>
#include <iostream>
#include <glm/gtc/type_ptr.hpp> // Para glm::value_ptr
#include <glm/gtc/matrix_transform.hpp> // Para glm::translate, glm::rotate, glm::scale


// Encapsulamos toda a lógica de desenho em um namespace para organização.
namespace scene {


class Node;
using NodePtr = std::shared_ptr<Node>;

class SceneGraph;
using SceneGraphPtr = std::shared_ptr<SceneGraph>;

class Node {
    ShapePtr shape;
    std::vector<Node*> children;
    TransformPtr transform; // Transformação local deste nó

    public:
        Node(ShapePtr shape, TransformPtr transform) : shape(shape), transform(transform) {}

        void addChild(Node* child) {
            children.push_back(child);
        }

        void draw(const TransformStackPtr transform_stack, unsigned int shader_program) {
            // Combina a transformação do pai com a transformação local
            transform_stack->push(*transform->getMatrix());

            // Envia a matriz de transformação para o shader
            unsigned int transformLoc = glGetUniformLocation(shader_program, "transform");
            glUniformMatrix4fv(transformLoc, 1, GL_FALSE, transform_stack->top()->value_ptr());

            // Desenha a forma associada a este nó, se existir
            if (shape) {
                shape->Draw();
            }

            // Desenha os filhos
            for (Node* child : children) {
                child->draw(global_transform, shader_program);
            }
        }
};

class SceneGraph {
};
    

}

#endif