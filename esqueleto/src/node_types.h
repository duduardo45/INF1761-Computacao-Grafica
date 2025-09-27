#ifndef NODE_TYPES_H
#define NODE_TYPES_H
#pragma once

#include "shader.h"
#include "shape.h"
#include "transform.h"
#include "generic_node.h"
#include <glm/gtc/type_ptr.hpp> // Para glm::value_ptr


namespace node {

class TransformNode;
class ShaderNode;
class ShapeNode;

using ShapeNodePtr = std::shared_ptr<ShapeNode>;
using ShaderNodePtr = std::shared_ptr<ShaderNode>;
using TransformNodePtr = std::shared_ptr<TransformNode>;
using MultiUseNodePtr = std::shared_ptr<MultiUseNode>;

class TransformNode : virtual public Node {
private:
    transform::TransformPtr transform;
protected:
    TransformNode(std::string name, transform::TransformPtr transform) : 
    Node(name),
    transform(transform) 
    {}
public:

    static TransformNodePtr Make(std::string name, transform::TransformPtr transform = nullptr) {
        return std::make_shared<TransformNode>(name, transform);
    }

    transform::TransformPtr getTransform() const {
        return transform;
    }

    void setTransform(transform::TransformPtr new_transform) {
        transform = new_transform;
    }

    // falta adicionar uma interface mais direta para as transformações

    void apply() override {
        // Aplica a transformação associada a este nó, se existir
        if (transform) transform::stack()->push(transform->getMatrix());
        Error::Check("node::TransformNode::apply");
    }

    void unapply() override {
        // Remove a transformação associada a este nó, se existir
        if (transform) transform::stack()->pop();
    }
};

class ShaderNode : virtual public Node {
private:
    shader::ShaderPtr shader;
protected:
    ShaderNode(std::string name, shader::ShaderPtr shader) : 
    Node(name),
    shader(shader) 
    {}
public:

    static ShaderNodePtr Make(std::string name, shader::ShaderPtr shader = nullptr) {
        return std::make_shared<ShaderNode>(name, shader);
    }

    shader::ShaderPtr getShader() const {
        return shader;
    }

    void setShader(shader::ShaderPtr new_shader) {
        shader = new_shader;
    }

    void apply() override {
        // Aplica o shader associado a este nó, se existir
        if (shader) shader::stack()->push(shader);
        Error::Check("node::ShaderNode::apply");
    }

    void unapply() override {
        // Remove o shader associado a este nó, se existir
        if (shader) shader::stack()->pop();
    }
};

class ShapeNode : virtual public Node {
private:
    ShapePtr shape;
protected:
    ShapeNode(std::string name, ShapePtr shape) : 
    Node(name),
    shape(shape) 
    {}
public:

    static ShapeNodePtr Make(std::string name, ShapePtr shape = nullptr) {
        return std::make_shared<ShapeNode>(name, shape);
    }

    ShapePtr getShape() const {
        return shape;
    }

    void setShape(ShapePtr new_shape) {
        shape = new_shape;
    }

    void apply() override {
        // Desenha a forma associada a este nó, se existir
        if (shape) shape->Draw();
        Error::Check("node::ShapeNode::apply");
    }

    void unapply() override {
        // Nada a fazer aqui, pois não há transformação associada a este nó
    }


};

class MultiUseNode : public TransformNode, public ShaderNode, public ShapeNode {
private:
    MultiUseNode(std::string name, transform::TransformPtr transform, shader::ShaderPtr shader, ShapePtr shape) :
    Node(name),
    TransformNode("",transform), 
    ShaderNode("",shader), 
    ShapeNode("",shape) 
    {}
public:
    static MultiUseNodePtr Make(std::string name, transform::TransformPtr transform = nullptr, shader::ShaderPtr shader = nullptr, ShapePtr shape = nullptr) {
        return std::make_shared<MultiUseNode>(name, transform, shader, shape);
    }

    void apply() override {
        TransformNode::apply();
        ShaderNode::apply();
        ShapeNode::apply();
    }

    void unapply() override {
        ShapeNode::unapply();
        ShaderNode::unapply();
        TransformNode::unapply();
    }
};

}

#endif