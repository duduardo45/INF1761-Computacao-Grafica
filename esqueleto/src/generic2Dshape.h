#ifndef GENERIC2DSHAPE_H
#define GENERIC2DSHAPE_H

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "shape.h"

#include <memory>
class Generic2dShape;
using Generic2dShapePtr = std::shared_ptr<Generic2dShape>;


class Generic2dShape : public Shape {
    unsigned int mode = GL_TRIANGLES;
    unsigned int nverts;
    unsigned int type = GL_UNSIGNED_INT;
    unsigned int offset = 0;
    unsigned int m_vao;
    unsigned int m_vbo;
    unsigned int m_ebo; 
    int n_indices;

protected:
    // Construtor agora armazena vbo e ebo
    Generic2dShape(float* dados_vertices, unsigned int* indices, int nverts, int n_indices) : 
    nverts(nverts),
    n_indices(n_indices)
    {
        // Define os parâmetros para o Draw()
        mode = GL_TRIANGLES;
        type = GL_UNSIGNED_INT;
        // Cada vértice tem 2 floats para posição e 3 floats para cor
        int vertex_stride_in_bytes = (2 + 3) * sizeof(float); // 5 floats por vértice

        // 1. Geração e bind do VAO
        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        // 2. Geração, bind e envio de dados para o VBO
        glGenBuffers(1, &m_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, 2 * nverts * sizeof(float), dados_vertices, GL_STATIC_DRAW);

        // 3. Configuração dos atributos dos vértices
        // Diz ao OpenGL como interpretar os dados de posição do VBO
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, vertex_stride_in_bytes, (void*)0);
        glEnableVertexAttribArray(0);

        // Diz ao OpenGL como interpretar os dados de cor do VBO
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertex_stride_in_bytes, (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // 4. Geração, bind e envio de dados para o EBO
        glGenBuffers(1, &m_ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, n_indices * sizeof(unsigned int), indices, GL_STATIC_DRAW);
        
        // 5. Unbind de tudo para evitar modificações acidentais
        // O VAO "lembra" dos binds do VBO e EBO, então podemos desvinculá-los.
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

public:
    static Generic2dShapePtr Make(float* dados_vertices, unsigned int* indices, int nverts, int n_indices) {
        // é trabalho do caller interpolar as posições e cores
        return Generic2dShapePtr(new Generic2dShape(dados_vertices, indices, nverts, n_indices));
    }

    // Destrutor que libera os buffers da GPU
    virtual ~Generic2dShape() {
        glDeleteBuffers(1, &m_vbo);
        glDeleteBuffers(1, &m_ebo);
        glDeleteVertexArrays(1, &m_vao);
    }

    // Função de desenho
    virtual void Draw() {
        glBindVertexArray(m_vao);
        // Desenha índices (3*(nverts-2))
        glDrawElements(mode, n_indices, type, (void*)0);
        glBindVertexArray(0);
    }
};

#endif
