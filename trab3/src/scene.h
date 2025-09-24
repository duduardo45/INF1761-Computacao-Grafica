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

using ParentPtr = std::weak_ptr<Node>; // ponteiro fraco para evitar ciclos de referência

class SceneGraph;
using SceneGraphPtr = std::shared_ptr<SceneGraph>;

class Node : public std::enable_shared_from_this<Node>{ // BACALHAU falta fazer as subclasses de nó
private:
    int id;
    inline static int next_id = 0;
    std::string name;
    ShapePtr shape;
    ShaderPtr shader;
    std::vector<NodePtr> children;
    int child_count = 0;
    transform::TransformPtr transform; // Transformação local deste nó
    ParentPtr parent; // pode ser nulo
    bool visibility = true;
    bool applicability = true;
    bool local_applicability = true;
    bool local_visibility = true;

    
    Node(std::string name, ShapePtr shape, ShaderPtr shader, transform::TransformPtr transform) : 
    name(name), shape(shape), shader(shader), transform(transform)
    {
        id = next_id++;
    }

    void setName(const std::string& new_name) {
        name = new_name;
    }

    void setParent(NodePtr new_parent) {
        parent = new_parent;
    }

    void setShader(ShaderPtr new_shader) {
        shader = new_shader;
    }

    void setTransform(transform::TransformPtr new_transform) {
        transform = new_transform;
    }

    void setShape(ShapePtr new_shape) {
        shape = new_shape;
    }

    void setVisibility(bool new_visibility) {
        visibility = new_visibility;
    }

    void setApplicability(bool new_applicability) {
        applicability = new_applicability;
    }

    friend class SceneGraph;

public:

    static NodePtr Make(std::string name, ShapePtr shape, ShaderPtr shader, transform::TransformPtr transform) {
        return NodePtr(new Node(name, shape, shader, transform));
    }

    static NodePtr Make(std::string name, ShapePtr shape, ShaderPtr shader, transform::TransformPtr transform, NodePtr parent) {
        NodePtr n = NodePtr(new Node(name, shape, shader, transform));
        n->parent = parent;
        return n;
    }

    static NodePtr Make(std::string name) {
        return NodePtr(new Node(name, nullptr, nullptr, transform::Transform::Make()));
    }

    static NodePtr Make(std::string name, NodePtr parent) {
        NodePtr n = NodePtr(new Node(name, nullptr, nullptr, transform::Transform::Make()));
        n->parent = parent;
        return n;
    }

    int getId() const {
        return id;
    }

    const std::string& getName() const {
        return name;
    }

    transform::TransformPtr getTransform() const {
        return transform;
    }

    ShapePtr getShape() const {
        return shape;
    }

    NodePtr getParent() const {
        return parent.lock();
    }

    bool getVisibility() const {
        return visibility;
    }

    bool getApplicability() const {
        return applicability;
    }

    bool getLocalVisibility() const {
        return local_visibility;
    }

    bool getLocalApplicability() const {
        return local_applicability;
    }
    
    ShaderPtr getShader() const {
        return shader;
    }

    int getChildCount() const {
        return child_count;
    }

    void updateChildCount() {
        child_count = children.size();
    }

    NodePtr getChild(int index) const {
        if (index < 0 || index >= children.size()) {
            std::cerr << "Index out of bounds in getChild" << std::endl;
            return nullptr;
        }
        return children[index];
    }

    NodePtr getChildByName(const std::string& child_name) const {
        for (NodePtr child : children) {
            if (child->name == child_name) {
                return child;
            }
        }
        std::cerr << "Child with name " << child_name << " not found in getChildByName" << std::endl;
        return nullptr;
    }

    int getChildIndex(std::string& child) const {
        for (int i = 0; i < children.size(); i++) {
            if (children[i]->name == child) {
                return i;
            }
        }
        std::cerr << "Child with name " << child << " not found in getChildIndex" << std::endl;
        return -1;
    }

    int getChildIndex(NodePtr child) const {
        for (int i = 0; i < children.size(); i++) {
            if (children[i] == child) {
                return i;
            }
        }
        std::cerr << "Child not found in getChildIndex" << std::endl;
        return -1;
    }

    void addChild(NodePtr child) {
        children.push_back(child);
        child->setParent(shared_from_this());
    }

    void addChild(NodePtr child, int index) {
        if (index < 0 || index > children.size()) {
            std::cerr << "Index out of bounds in addChild" << std::endl;
            return;
        }
        children.insert(children.begin() + index, child);
        child->setParent(shared_from_this());
    }

    void addChildFront(NodePtr child) {
        children.insert(children.begin(), child);
        child->setParent(shared_from_this());
    }

    void addChildAfter(NodePtr child, NodePtr after) {
        auto it = std::find(children.begin(), children.end(), after);
        if (it != children.end()) {
            children.insert(it + 1, child);
        } else {
            std::cerr << "Reference child not found in addChildAfter" << std::endl;
        }
        child->setParent(shared_from_this());
    }

    void moveChild(int from_idx, int to_idx) {
        if (from_idx < 0 || from_idx >= children.size() || to_idx < 0 || to_idx >= children.size()) {
            std::cerr << "Index out of bounds in moveChild" << std::endl;
            return;
        }
        NodePtr child = children[from_idx];
        children.erase(children.begin() + from_idx);
        children.insert(children.begin() + to_idx, child);
    }

    void swapChildren(int idx1, int idx2) {
        if (idx1 < 0 || idx1 >= children.size() || idx2 < 0 || idx2 >= children.size()) {
            std::cerr << "Index out of bounds in swapChildren" << std::endl;
            return;
        }
        std::swap(children[idx1], children[idx2]);
    }

    void removeChild(NodePtr child) {
        auto it = std::find(children.begin(), children.end(), child);
        if (it != children.end()) {
            (*it)->setParent(nullptr);
            children.erase(it);
        } else {
            std::cerr << "Child not found in removeChild" << std::endl;
        }
    }

    void draw(bool print = false) {
        if (!applicability) return;
        if (!visibility) return;
        if (!local_applicability) {
            // Desenha os filhos
            for (NodePtr child : children) {
                child->draw();
            }
            Error::Check("scene::Node::draw after drawing children");
            return;
        }
        if (print) printf("Drawing node %s (id=%d) (parent=%s)\n", name.c_str(), id, parent.lock()? parent.lock()->getName().c_str() : "none");

        Error::Check("scene::Node::draw start");
        transform::TransformStack& transform_stack = transform::stack();
        // Combina a transformação do pai com a transformação local dentro do push
        if (transform) transform_stack.push(transform->getMatrix());

        if (shader) shaderStack().push(shader);

        if (!local_visibility) {
            // Desenha os filhos
            for (NodePtr child : children) {
                child->draw();
            }
            Error::Check("scene::Node::draw after drawing children");
            if (transform) transform_stack.pop();
            if (shader) shaderStack().pop();
            Error::Check("scene::Node::draw end");
            return;
        }

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
            child->draw(print);
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
        root = Node::Make("root", nullptr, base, transform::Transform::Make());
        base_shader = base;
        currentNode = root;
        name_map["root"] = root;
        node_map[root->getId()] = root;
        view_transform = transform::Transform::Make();
        view_transform->orthographic(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f); // Inicializa com ortográfica padrão
    }

    friend SceneGraphPtr graph();

    void registerNode(NodePtr node) {
        name_map[node->getName()] = node;
        node_map[node->getId()] = node;
        currentNode = node;
    }

protected:

    // BACALHAU
    NodePtr getRoot() const {
        return root;
    }

    // NodePtr getCurrentNode() const {
    //     return currentNode;
    // }

    NodePtr getNodeByName(const std::string& name) {
        auto it = name_map.find(name);
        if (it != name_map.end()) {
            currentNode = it->second;
            return currentNode;
        }
        return nullptr;
    }

    NodePtr getNodeById(int id) {
        auto it = node_map.find(id);
        if (it != node_map.end()) {
            currentNode = it->second;
            return currentNode;
        }
        return nullptr;
    }

public:

    NodePtr getCurrentNode() const {
        return currentNode;
    }

    void addNode(NodePtr node, NodePtr parent = nullptr) {
        if (!parent) {
            parent = root;
        }
        parent->addChild(node);
        registerNode(node);
    }

    void addNode(const std::string& name, ShapePtr shape = nullptr, ShaderPtr shader = nullptr, transform::TransformPtr transform = nullptr, NodePtr parent = nullptr) {
        if (name_map.find(name) != name_map.end()) {
            std::cerr << "Node with name " << name << " already exists!" << std::endl;
            return;
        }
        if (!transform) {
            transform = transform::Transform::Make();
        }
        NodePtr new_node = Node::Make(name, shape, shader, transform);
        addNode(new_node, parent);
    }

    void addNodeToCurrent(const std::string& name, ShapePtr shape = nullptr, ShaderPtr shader = nullptr, transform::TransformPtr transform = nullptr) {
        addNode(name, shape, shader, transform, currentNode);
    }

    void lookAtNode(const std::string& name) {
        NodePtr node = getNodeByName(name);
        if (node) {
            currentNode = node;
        } else {
            std::cerr << "Node with name " << name << " not found!" << std::endl;
        }
    }

    void lookAtNode(int id) {
        NodePtr node = getNodeById(id);
        if (node) {
            currentNode = node;
        } else {
            std::cerr << "Node with id " << id << " not found!" << std::endl;
        }
    }

    void setCurrentNodeShader(ShaderPtr shader) {
        currentNode->setShader(shader);
    }

    void setCurrentNodeShape(ShapePtr shape) {
        currentNode->setShape(shape);
    }

    void setCurrentNodeTransform(transform::TransformPtr transform) {
        currentNode->setTransform(transform);
    }

    void moveCurrentNodeTo(NodePtr new_parent) {
        if (new_parent) {
            NodePtr old_parent = currentNode->getParent();
            if (old_parent) {
                old_parent->removeChild(currentNode);
            }
            new_parent->addChild(currentNode);
        } else {
            std::cerr << "New parent is null in moveCurrentNodeTo" << std::endl;
        }
    }

    void moveCurrentNodeTo(const std::string& new_parent_name) {
        NodePtr new_parent = getNodeByName(new_parent_name);
        if (new_parent) {
            NodePtr old_parent = currentNode->getParent();
            if (old_parent) {
                old_parent->removeChild(currentNode);
            }
            new_parent->addChild(currentNode);
        } else {
            std::cerr << "New parent with name " << new_parent_name << " not found in moveCurrentNodeTo" << std::endl;
        }
    }

    void moveToPositionUnderParent(const int position) {
        NodePtr parent = currentNode->getParent();
        if (parent) {
            if (position < 0 || position >= parent->getChildCount()) {
                std::cerr << "Position out of bounds in moveToPositionUnderParent" << std::endl;
                return;
            }
            parent->moveChild(parent->getChildIndex(currentNode), position);
        } else {
            std::cerr << "Current node has no parent in moveToPositionUnderParent" << std::endl;
        }
    }

    void moveChild(const int from_idx, const int to_idx) {
        currentNode->moveChild(from_idx, to_idx);
    }

    void swapChildren(const int idx1, const int idx2) {
        currentNode->swapChildren(idx1, idx2);
    }

    void renameCurrentNode(const std::string& new_name) {
        if (name_map.find(new_name) != name_map.end()) {
            std::cerr << "Node with name " << new_name << " already exists!" << std::endl;
            return;
        }
        name_map.erase(currentNode->getName());
        currentNode->setName(new_name);
        name_map[new_name] = currentNode;
    }

    void removeCurrentNode() {
        NodePtr parent = currentNode->getParent();
        if (parent) {
            parent->removeChild(currentNode);
        }
        name_map.erase(currentNode->getName());
        node_map.erase(currentNode->getId());
    }

    void duplicateNode(const std::string& name, const std::string& new_name) {
        NodePtr node = getNodeByName(name);
        if (node) {
            NodePtr new_node = Node::Make(
                new_name, 
                node->getShape(), 
                node->getShader(), 
                transform::Transform::Make(node->getTransform()->getMatrix()), 
                node->getParent()
            );
            node->getParent()->addChild(new_node);
            name_map[new_name] = new_node;
            node_map[new_node->getId()] = new_node;
            currentNode = new_node;
        } else {
            std::cerr << "Node with name " << name << " not found!" << std::endl;
        }
    }

    void addSibling(const std::string& name, ShapePtr shape = nullptr, ShaderPtr shader = nullptr, transform::TransformPtr transform = nullptr) {
        NodePtr parent = currentNode->getParent();
        if (parent) {
            addNode(name, shape, shader, transform, parent);
        } else {
            std::cerr << "Current node has no parent!" << std::endl;
        }
    }

    void addSiblingAfter(NodePtr new_sibling, const std::string& node_to_add_after = "") {
        if (!node_to_add_after.empty()) {
            NodePtr after = getNodeByName(node_to_add_after);
            if (after) {
                NodePtr parent = after->getParent();
                if (parent) {
                    parent->addChildAfter(new_sibling, after);
                    registerNode(new_sibling);
                } else {
                    std::cerr << "Node to add after has no parent!" << std::endl;
                }
            } else {
                std::cerr << "Node to add after not found!" << std::endl;
            }
            return;
        }
        
        NodePtr parent = currentNode->getParent();
        if (parent) {
            NodePtr after = currentNode;
            parent->addChildAfter(new_sibling, after);
            registerNode(new_sibling);
        } else {
            std::cerr << "Current node has no parent!" << std::endl;
        }
    }

    void addSiblingAfter(const std::string& name, ShapePtr shape = nullptr, ShaderPtr shader = nullptr, transform::TransformPtr transform = nullptr, const std::string& node_to_add_after = "") {
        if (transform == nullptr) {
            transform = transform::Transform::Make();
        }
        addSiblingAfter(Node::Make(name, shape, shader, transform), node_to_add_after);
    }

    // encapsula as funções do transform para o current node

    void translateCurrentNode(float delta_x, float delta_y, float delta_z) {
        currentNode->getTransform()->translate(delta_x, delta_y, delta_z);
    }

    void rotateCurrentNode(float angle, float axis_x, float axis_y, float axis_z) {
        currentNode->getTransform()->rotate(angle, axis_x, axis_y, axis_z);
    }

    void rotateCurrentNode(float angle) {
        rotateCurrentNode(angle, 0.0f, 0.0f, 1.0f);
    }

    void scaleCurrentNode(float x, float y, float z) {
        currentNode->getTransform()->scale(x, y, z);
    }

    void setTranslateCurrentNode(float x, float y, float z) {
        currentNode->getTransform()->setTranslate(x, y, z);
    }

    void setRotateCurrentNode(float angle, float axis_x, float axis_y, float axis_z) {
        currentNode->getTransform()->setRotate(angle, axis_x, axis_y, axis_z);
    }

    void setRotateCurrentNode(float angle) {
        setRotateCurrentNode(angle, 0.0f, 0.0f, 1.0f);
    }

    void setScaleCurrentNode(float x, float y, float z) {
        currentNode->getTransform()->setScale(x, y, z);
    }

    void resetTransformCurrentNode() {
        currentNode->getTransform()->reset();
    }

    void setTransformCurrentNode(glm::mat4 transform) {
        currentNode->getTransform()->setMatrix(transform);
    }

    void setTransformCurrentNode(transform::TransformPtr transform) {
        currentNode->setTransform(transform);
    }

    void newNodeAbove(const std::string& new_name) {
        NodePtr current = currentNode, old_parent = currentNode->getParent();
        addSiblingAfter(Node::Make(new_name));
        currentNode->addChild(current);
        old_parent->removeChild(current);
        current->setParent(currentNode);

        // alternative logic:
        // goes to parent finds the current node stores the current node 
        // then adds a new node where the current one was relative to the parent
        // and adds the previous current node as a child to the created node
        // NodePtr parent = currentNode->getParent();
        // if (parent) {
            
        //     int idx = -1;
        //     NodePtr new_node = Node::Make(name, nullptr, nullptr, transform::Transform::Make());
        //     idx = parent->getChildIndex(currentNode);
        //     if (idx != -1) {
        //         parent->removeChild(currentNode);
        //         parent->addChild(new_node, idx);
        //         new_node->addChild(currentNode);
        //         currentNode = new_node;
        //     } else {
        //         std::cerr << "Current node not found in parent's children!" << std::endl;
        //     }
        // } else {
        //     std::cerr << "Current node has no parent!" << std::endl;
        // }
    }


    void newNodeAbove() {
        std::string new_name = currentNode->getName() + "_parent";
        newNodeAbove(new_name);
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
        root = Node::Make("root", nullptr, base_shader, transform::Transform::Make());
        currentNode = root;
        name_map.clear();
        node_map.clear();
        name_map["root"] = root;
        node_map[root->getId()] = root;
    }

    

    void draw(bool print = false) {
        // Aplica a transformação de visão
        transform::stack().push(view_transform->getMatrix());
        if (root) {
            root->draw(print);
        }
        transform::stack().pop();
        if (print) printf("\n--------------------------------\n\n");
    }

    void drawSubtree(NodePtr node, bool print = false) {
        // Aplica a transformação de visão
        transform::stack().push(view_transform->getMatrix());
        if (node) {
            node->draw(print);
        }
        transform::stack().pop();
        if (print) printf("\n--------------------------------\n\n");
    }

    void drawSubtree(const std::string& node_name) {
        NodePtr node = getNodeByName(node_name);
        if (node) {
            drawSubtree(node);
        } else {
            std::cerr << "Node with name " << node_name << " not found!" << std::endl;
        }
    }

    void drawSubtree(const int node_id) {
        NodePtr node = getNodeById(node_id);
        if (node) {
            drawSubtree(node);
        } else {
            std::cerr << "Node with id " << node_id << " not found!" << std::endl;
        }
    }
};
    
inline SceneGraphPtr graph() {
    static ShaderPtr default_shader = Shader::Make();
    static SceneGraphPtr instance = SceneGraphPtr(new SceneGraph(default_shader));
    return instance;
}

}

#endif