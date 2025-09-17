#ifndef TRANSFORM_H
#define TRANSFORM_H
#pragma once

#include <memory>
class Transform;
using TransformPtr = std::shared_ptr<Transform>;

class TransformStack;
using TransformStackPtr = std::shared_ptr<TransformStack>;


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>



class Transform {
    glm::mat4 matrix; // Matriz de transformação 4x4
    Transform() {
        // Inicializa a matriz como identidade
        matrix = glm::mat4(1.0f);
    }
    public:
        static TransformPtr MakeTransform() {
            return TransformPtr(new Transform());
        }

        ~Transform()=default;

        void translate(float x, float y, float z) {
            matrix = glm::translate(matrix, glm::vec3(x, y, z));
        }

        void rotate(float angle_degrees, float axis_x, float axis_y, float axis_z) {
            // checks if axis is normalized
            float angle_radians = glm::radians(angle_degrees);
            glm::vec3 axis(axis_x, axis_y, axis_z);

            if (glm::length(axis) > 0.0f) {
                axis = glm::normalize(axis);
            }
            matrix = glm::rotate(matrix, angle_radians, axis);
        }

        void scale(float x, float y, float z) {
            matrix = glm::scale(matrix, glm::vec3(x, y, z));
        }
        
        const glm::mat4* getMatrix() const {
            return &matrix;
        }
};

class TransformStack {
    std::vector<glm::mat4> stack;
    TransformStack() {
        // Inicializa a pilha com a matriz identidade
        stack.push_back(glm::mat4(1.0f));
    }
    public:
        static TransformStackPtr MakeTransformStack() {
            return TransformStackPtr(new TransformStack());
        }

        ~TransformStack()=default;

        void push(const glm::mat4& matrix) {
            stack.push_back(*(top()) * matrix);
        }

        void pop() {
            if (stack.size() > 1) { // Evita remover a última matriz
                stack.pop_back();
            } else {
                std::cerr << "Warning: Attempt to pop from an empty transform stack." << std::endl;
            }
        }

        const glm::mat4* top() const {
            return &stack.back();
        }
};

#endif