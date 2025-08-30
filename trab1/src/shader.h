#ifndef SHADER_H
#define SHADER_H
#pragma once
#include <memory>
class Shader;
using ShaderPtr = std::shared_ptr<Shader>;

#include <glad/gl.h>
#include <fstream>
#include <iostream>
#include <sstream> 
#include <cstdlib>

static GLuint MakeShader(GLenum shadertype, const std::string& filename) {

    GLuint id = glCreateShader(shadertype);

    // errorcheck
    if (id == 0) {
        std::cerr << "Could not create shader object";
        exit(1);
    }

    // open the shader file
    std::ifstream fp;
    fp.open(filename); 


    // errorcheck
    if (!fp.is_open()) {
        std::cerr << "Could not open file: " << filename << std::endl;
        exit(1);
    } 

    // read the shader file content
    std::stringstream strStream;
    strStream << fp.rdbuf();

    // pass the source string to OpenGL
    std::string source = strStream.str();
    const char* csource = source.c_str();
    glShaderSource(id, 1, &csource, 0);

    // tell OpenGL to compile the shader
    GLint status;
    glCompileShader(id);
    glGetShaderiv(id, GL_COMPILE_STATUS, &status);

    // errorcheck
    if (!status) {
        GLint len;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &len);
        char* message = new char[len];
        glGetShaderInfoLog(id, len, 0, message);
        std::cerr << filename << ":" << std::endl << message << std::endl;
        delete [] message;
        exit(1);
    }

    return id;
}

class Shader {
    unsigned int m_pid;
protected:
    Shader() {
        m_pid - glCreateProgram();
        if (m_pid == 0) {
            std::cerr << "Could not create program object";
            exit(1);
        }
    }
public:
    static ShaderPtr Make() {
        return ShaderPtr(new Shader());
    }
    
    virtual ~Shader(){
        glDeleteProgram(m_pid);
    }

    void AttachVertexShader(const std::string& filename) {
        GLuint sid = MakeShader(GL_VERTEX_SHADER, filename);
        glAttachShader(m_pid, sid);
    }

    void AttachFragmentShader(const std::string& filename) {
        GLuint sid = MakeShader(GL_FRAGMENT_SHADER, filename);
        glAttachShader(m_pid, sid);
    }

    void Link() {
        glLinkProgram(m_pid);
        GLint status;
        glGetProgramiv(m_pid, GL_LINK_STATUS, &status);
        if (status == GL_FALSE) {
            GLint len;
            glGetProgramiv(m_pid, GL_INFO_LOG_LENGTH, &len);
            char* message = new char[len];
            glGetProgramInfoLog(m_pid, len, 0, message);
            std::cerr << "Shader linking failed: " << message << std::endl;
            delete[] message;
            exit(1);
        }
    }
    void UseProgram() const {
        glUseProgram(m_pid);
    }
};

static GLuint educationalMakeShader(GLenum shadertype, const std::string& filename) {

    GLuint id = glCreateShader(shadertype);

    // open the shader file
    std::ifstream fp;
    fp.open(filename); 

    // read the shader file content
    std::stringstream strStream;
    strStream << fp.rdbuf();

    // pass the source string to OpenGL
    std::string source = strStream.str();
    const char* csource = source.c_str();
    glShaderSource(id, 1, &csource, 0);

    // tell OpenGL to compile the shader
    GLint status;
    glCompileShader(id);
    glGetShaderiv(id, GL_COMPILE_STATUS, &status);

    return id;
}

#endif