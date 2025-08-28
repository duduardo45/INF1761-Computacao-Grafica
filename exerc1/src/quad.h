// --- No seu arquivo Quad.h ---

class Quad;
using QuadPtr = std::shared_ptr<Quad>;

#ifndef QUAD_H
#define QUAD_H

#include "shape.h" // Supondo que Shape defina nverts, mode, type
#include <memory>  // Para std::shared_ptr

class Quad : public Shape {
    unsigned int m_vao;
    unsigned int m_vbo;
    unsigned int m_ebo; 
    const int n_indices = 6;

protected:
    // Construtor agora armazena vbo e ebo
    Quad(float* vertices, unsigned int* indices) {
        // Define os parâmetros para o Draw()
        mode = GL_TRIANGLES;
        type = GL_UNSIGNED_INT;
        nverts = 4; // Total de vértices na geometria

        // 1. Geração e bind do VAO
        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        // 2. Geração, bind e envio de dados para o VBO
        glGenBuffers(1, &m_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, 2 * nverts * sizeof(float), vertices, GL_STATIC_DRAW);

        // 3. Configuração dos atributos dos vértices
        // Diz ao OpenGL como interpretar os dados do VBO
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

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
    // Factory que cria um quadrado e retorna um shared_ptr
    static QuadPtr Make(float* vertices, unsigned int* indices) {
        // O construtor é protegido, então usamos 'new' e o colocamos em um shared_ptr
        return QuadPtr(new Quad(vertices, indices));
    }

    // Destrutor que libera os buffers da GPU
    virtual ~Quad() {
        glDeleteBuffers(1, &m_vbo);
        glDeleteBuffers(1, &m_ebo);
        glDeleteVertexArrays(1, &m_vao);
    }

    // Função de desenho
    virtual void Draw() const {
        glBindVertexArray(m_vao);
        // Desenha 6 índices (3*(4-2) = 6)
        glDrawElements(mode, n_indices, type, (void*)0);
        glBindVertexArray(0);
    }
};

#endif
