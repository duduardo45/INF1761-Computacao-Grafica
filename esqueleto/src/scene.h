#ifndef SCENE_H
#define SCENE_H
#pragma once

#include "gl_includes.h"
#include "polygon.h"
#include "shape.h"
#include "shader.h"
#include "transform.h"
#include "error.h"
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
    inline static int next_id = 0;
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

        const std::string& getName() const {
            return name;
        }

        transform::TransformPtr getTransform() {
            return transform;
        }

        NodePtr getChild(int index) {
            if (index < 0 || index >= children.size()) {
                std::cerr << "Index out of bounds in getChild" << std::endl;
                return nullptr;
            }
            return children[index];
        }

        NodePtr getChildByName(const std::string& child_name) {
            for (NodePtr child : children) {
                if (child->name == child_name) {
                    return child;
                }
            }
            std::cerr << "Child with name " << child_name << " not found in getChildByName" << std::endl;
            return nullptr;
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
            printf("Drawing node %s (id=%d)\n", name.c_str(), id);

            Error::Check("scene::Node::draw start");
            transform::TransformStack& transform_stack = transform::stack();
            // Combina a transformação do pai com a transformação local dentro do push
            if (transform) transform_stack.push(transform->getMatrix());

            if (shader) shaderStack().push(shader);

            Error::Check("scene::Node::draw before drawing shape");
            // Desenha a forma associada a este nó, se existir
            if (shape) {
                
                // Envia a matriz de transformação para o shader
                unsigned int shader_program = shaderStack().top()->GetShaderID();
                unsigned int transformLoc = glGetUniformLocation(shader_program, "M");
                glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform_stack.top()));
                shape->Draw();
            }
            Error::Check("scene::Node::draw after drawing shape");
            // Desenha os filhos
            for (NodePtr child : children) {
                child->draw();
            }
            Error::Check("scene::Node::draw after drawing children");
            if (transform) transform_stack.pop();
            if (shader) shaderStack().pop();
            Error::Check("scene::Node::draw end");
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
            root = Node::MakeNode("root", nullptr, base, transform::Transform::Make());
            base_shader = base;
            currentNode = root;
            name_map["root"] = root;
            node_map[root->getId()] = root;
            view_transform = transform::Transform::Make();
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

        void clearGraph() {
            root = Node::MakeNode("root", nullptr, base_shader, transform::Transform::Make());
            currentNode = root;
            name_map.clear();
            node_map.clear();
            name_map["root"] = root;
            node_map[root->getId()] = root;
        }

        void draw() {
            // Aplica a transformação de visão
            transform::stack().push(view_transform->getMatrix());
            if (root) {
                root->draw();
            }
            transform::stack().pop();
            printf("\n--------------------------------\n\n");
        }
};
    
inline SceneGraphPtr graph() {
    static ShaderPtr default_shader = Shader::Make();
    static SceneGraphPtr instance = SceneGraphPtr(new SceneGraph(default_shader));
    return instance;
}

}

#endif