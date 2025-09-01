#ifndef DRAWING_H
#define DRAWING_H
#pragma once

#include "gl_includes.h"
#include "Polygon.h" 
#include "shader.h"
#include "triangulate.h" // Inclui o seu novo header de triangulação
#include <vector>
#include <memory>
#include <cmath>
#include <iostream>

class Drawing {
    GLuint previewVAO = 0;
    GLuint previewVBO = 0;
public:
    std::vector<float> current_vertices_data; // Formato intercalado: [x, y, r, g, b, x, y, ...]
    bool is_drawing = false;
    void clear() {
        current_vertices_data.clear();
        is_drawing = false;
    }
    void start(float x, float y, const float* color) {
        is_drawing = true;
        current_vertices_data.clear();
        current_vertices_data.insert(current_vertices_data.end(), {x, y, color[0], color[1], color[2]});
    }
    void addVertex(float x, float y, const float* color) {
        current_vertices_data.insert(current_vertices_data.end(), {x, y, color[0], color[1], color[2]});
    }
    int numVertices() const {
        return current_vertices_data.size() / 5;
    }
    void getPositionsAndColors(std::vector<float>& positions, std::vector<float>& colors) const {
        int nverts = numVertices();
        positions.reserve(nverts * 2);
        colors.reserve(nverts * 3);
        for(int i = 0; i < nverts; ++i) {
            positions.push_back(current_vertices_data[i*5 + 0]);
            positions.push_back(current_vertices_data[i*5 + 1]);
            colors.push_back(current_vertices_data[i*5 + 2]);
            colors.push_back(current_vertices_data[i*5 + 3]);
            colors.push_back(current_vertices_data[i*5 + 4]);
        }
    }
    void getPositions(std::vector<float>& positions) const {
        int nverts = numVertices();
        positions.reserve(nverts * 2);
        for(int i = 0; i < nverts; ++i) {
            positions.push_back(current_vertices_data[i*5 + 0]);
            positions.push_back(current_vertices_data[i*5 + 1]);
        }
    }
    PolygonPtr finalize() {
        is_drawing = false;
        int num_verts = numVertices();
        if (num_verts < 3) {
            std::cout << "Desenho cancelado: polígono precisa de pelo menos 3 vértices." << std::endl;
            current_vertices_data.clear();
            return nullptr;
        }

        // 1. Prepara os dados para a função de triangulação (apenas posições)
        std::vector<float> positions_only;
        getPositions(positions_only);

        // 2. Chama a função de triangulação do header 'triangulate.h'
        std::shared_ptr<int> indices_ptr = TriangulateEarClipping(positions_only.data(), num_verts);
        
        if (!indices_ptr) { 
            std::cerr << "A triangulação falhou. O polígono não será adicionado." << std::endl; 
            current_vertices_data.clear();
            return nullptr; 
        }

        // 3. Converte os dados para criar o objeto Polygon
        int num_indices = (num_verts - 2) * 3;
        std::vector<unsigned int> final_indices(num_indices);
        int* raw_indices = indices_ptr.get();
        for(int i = 0; i < num_indices; ++i) {
            final_indices[i] = static_cast<unsigned int>(raw_indices[i]);
        }

        std::vector<float> positions, colors;
        getPositionsAndColors(positions, colors);
        
        PolygonPtr polygon = Polygon::Make(positions.data(), colors.data(), final_indices.data(), num_verts, final_indices.size());
        current_vertices_data.clear();
        return polygon;
    }
    void gl_initialize(ShaderPtr shader) {
        glGenVertexArrays(1, &previewVAO);
        glGenBuffers(1, &previewVBO);
        glBindVertexArray(previewVAO);
        glBindBuffer(GL_ARRAY_BUFFER, previewVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 5 * 100, nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }
    void gl_cleanup() {
        glDeleteBuffers(1, &previewVBO);
        glDeleteVertexArrays(1, &previewVAO);
    }
    void gl_drawPreview() {
        if (!is_drawing || current_vertices_data.empty()) return;
        int num_verts = numVertices();
        
        glBindBuffer(GL_ARRAY_BUFFER, previewVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * current_vertices_data.size(), current_vertices_data.data());
        
        glBindVertexArray(previewVAO);
        glPointSize(10.0f);
        glDrawArrays(GL_POINTS, 0, num_verts);
        glDrawArrays(GL_LINE_STRIP, 0, num_verts);
        glBindVertexArray(0);
    }

};

#endif