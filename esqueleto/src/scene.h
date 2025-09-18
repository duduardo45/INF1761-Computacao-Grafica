#ifndef SCENE_H
#define SCENE_H
#pragma once

#include "gl_includes.h"
#include "polygon.h"
#include "shape.h"
#include "shader.h"
#include "transform.h"
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <memory>
#include <cmath>
#include <iostream>
#include <glm/gtc/type_ptr.hpp> // Para glm::value_ptr

// Encapsulamos toda a lógica de desenho em um namespace para organização.
namespace scene {


class Node;
using NodePtr = std::shared_ptr<Node>;

class SceneGraph;
using SceneGraphPtr = std::shared_ptr<SceneGraph>;

class Node { // BACALHAU falta fazer as subclasses de nó
    int id;
    static int next_id;
    std::string name;
    ShapePtr shape;
    ShaderPtr shader;
    std::vector<NodePtr> children;
    transform::TransformPtr transform; // Transformação local deste nó
    
    Node(std::string name, ShapePtr shape, ShaderPtr shader, transform::TransformPtr transform) : 
    name(name), shape(shape), shader(shader), transform(transform) 
    {
        id = next_id++;
    }

    public:

        static NodePtr MakeNode(std::string name, ShapePtr shape, ShaderPtr shader, transform::TransformPtr transform) {
            return NodePtr(new Node(name, shape, shader, transform));
        }

        int getId() {
            return id;
        }

        void addChild(NodePtr child) {
            children.push_back(child);
        }

        void addChild(NodePtr child, int index) {
            if (index < 0 || index > children.size()) {
                std::cerr << "Index out of bounds in addChild" << std::endl;
                return;
            }
            children.insert(children.begin() + index, child);
        }

        void addChildFront(NodePtr child) {
            children.insert(children.begin(), child);
        }

        void addChildAfter(NodePtr child, NodePtr after) {
            auto it = std::find(children.begin(), children.end(), after);
            if (it != children.end()) {
                children.insert(it + 1, child);
            } else {
                std::cerr << "Reference child not found in addChildAfter" << std::endl;
            }
        }

        void removeChild(NodePtr child) {
            auto it = std::find(children.begin(), children.end(), child);
            if (it != children.end()) {
                children.erase(it);
            } else {
                std::cerr << "Child not found in removeChild" << std::endl;
            }
        }

        void setShader(ShaderPtr new_shader) {
            shader = new_shader;
        }

        void draw() {
            transform::TransformStack& transform_stack = transform::stack();
            // Combina a transformação do pai com a transformação local
            transform_stack.push(transform->getMatrix());

            if (shader) shaderStack().push(shader);


            // Desenha a forma associada a este nó, se existir
            if (shape) {
                
                // Envia a matriz de transformação para o shader
                unsigned int shader_program = shaderStack().top()->GetShaderID();
                unsigned int transformLoc = glGetUniformLocation(shader_program, "transform");
                glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform_stack.top()));
                shape->Draw();
            }

            // Desenha os filhos
            for (NodePtr child : children) {
                child->draw();
            }
            transform_stack.pop();
            if (shader) shaderStack().pop();
        }
};

class SceneGraph {
    private:
        NodePtr root;
        ShaderPtr base_shader;
        std::map<std::string, NodePtr> name_map; // Mapa de nomes para nós
        std::map<int, NodePtr> node_map; // Mapa de IDs para nós
        NodePtr currentNode; // Nó atualmente selecionado
        transform::TransformPtr view_transform;

        
        SceneGraph(ShaderPtr base) {
            root = Node::MakeNode("root", nullptr, nullptr, transform::Transform::MakeTransform());
            base_shader = base;
            currentNode = root;
            name_map["root"] = root;
            node_map[root->getId()] = root;
            view_transform = transform::Transform::MakeTransform();
            view_transform->orthographic(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f); // Inicializa com ortográfica padrão
        }

        friend SceneGraphPtr graph();

    public:

        // BACALHAU
        NodePtr getRoot() const {
            return root;
        }

        void setView(float left, float right, float bottom, float top, float near, float far) {
            view_transform->orthographic(left, right, bottom, top, near, far);
        }

        void initializeBaseShader(const std::string& vertex_shader_file, const std::string& fragment_shader_file) {
            base_shader->AttachVertexShader(vertex_shader_file);
            base_shader->AttachFragmentShader(fragment_shader_file);
            base_shader->Link();
        }

        void draw() {
            // Aplica a transformação de visão
            transform::stack().push(view_transform->getMatrix());
            if (root) {
                root->draw();
            }
            transform::stack().pop();
        }
};
    
inline SceneGraphPtr graph() {
    static ShaderPtr default_shader = Shader::Make();
    static SceneGraphPtr instance = SceneGraphPtr(new SceneGraph(default_shader));
    return instance;
}

}

#endif