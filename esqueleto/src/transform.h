#ifndef TRANSFORM_H
#define TRANSFORM_H
#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


namespace transform {
    
    
class Transform;
using TransformPtr = std::shared_ptr<Transform>;

class TransformStack;
using TransformStackPtr = std::shared_ptr<TransformStack>;

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

        void orthographic(float left, float right, float bottom, float top, float near, float far) {
            matrix = glm::ortho(left, right, bottom, top, near, far);
        }
        
        const glm::mat4& getMatrix() const {
            return matrix;
        }
};

TransformStack& stack();

class TransformStack {
private:
    std::vector<glm::mat4> stack;

    TransformStack() {
        stack.push_back(glm::mat4(1.0f));
    }

    // A amizade agora é concedida à função livre 'stack()' do namespace.
    friend TransformStack& stack();

public:
    TransformStack(const TransformStack&) = delete;
    TransformStack& operator=(const TransformStack&) = delete;
    ~TransformStack() = default;

    void push(const glm::mat4& matrix_to_apply) {
        stack.push_back(top() * matrix_to_apply);
    }

    void pop() {
        if (stack.size() > 1) {
            stack.pop_back();
        } else {
            std::cerr << "Warning: Attempt to pop the base identity matrix from the transform stack." << std::endl;
        }
    }

    const glm::mat4& top() const {
        return stack.back();
    }
};

// Definição da função de acesso (inline para uso no header)
inline TransformStack& stack() {
    static TransformStack instance;
    return instance;
}

}
#endif