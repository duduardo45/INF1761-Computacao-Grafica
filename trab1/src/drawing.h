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

// Encapsulamos toda a lógica de desenho em um namespace para organização.
namespace drawing {

// --- Estado da Aplicação e Dados de Desenho ---
static std::vector<float> current_vertices_data; // Formato intercalado: [x, y, r, g, b, x, y, ...]
static std::vector<PolygonPtr> scene_polygons;
static bool is_drawing = false;

// --- Recursos do OpenGL para o Preview ---
static GLuint previewVAO = 0;
static GLuint previewVBO = 0;

// Cores predefinidas para os vértices
const float PRESET_COLORS[][3] = {
    {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.0f}
};
const int NUM_PRESET_COLORS = 6;

// --- Funções de Manipulação de Desenho ---
void startPolygon(float x, float y) {
    is_drawing = true;
    current_vertices_data.clear();
    current_vertices_data.insert(current_vertices_data.end(), {x, y, PRESET_COLORS[0][0], PRESET_COLORS[0][1], PRESET_COLORS[0][2]});
    std::cout << "Polígono iniciado no ponto (" << x << ", " << y << ")" << std::endl;
}

void addVertexToPolygon(float x, float y) {
    int color_index = (current_vertices_data.size() / 5) % NUM_PRESET_COLORS;
    const float* color = PRESET_COLORS[color_index];
    current_vertices_data.insert(current_vertices_data.end(), {x, y, color[0], color[1], color[2]});
    std::cout << "Vértice adicionado no ponto (" << x << ", " << y << ")" << std::endl;
}

void finishPolygon() {
    is_drawing = false;
    int num_verts = current_vertices_data.size() / 5;
    if (num_verts < 3) {
        std::cout << "Desenho cancelado: polígono precisa de pelo menos 3 vértices." << std::endl;
        current_vertices_data.clear();
        return;
    }

    // 1. Prepara os dados para a função de triangulação (apenas posições)
    std::vector<float> positions_only;
    positions_only.reserve(num_verts * 2);
    for(int i = 0; i < num_verts; ++i) {
        positions_only.push_back(current_vertices_data[i * 5 + 0]);
        positions_only.push_back(current_vertices_data[i * 5 + 1]);
    }

    // 2. Chama a função de triangulação do header 'triangulate.h'
    std::shared_ptr<int> indices_ptr = TriangulateEarClipping(positions_only.data(), num_verts);
    
    if (!indices_ptr) { 
        std::cerr << "A triangulação falhou. O polígono não será adicionado." << std::endl; 
        current_vertices_data.clear();
        return; 
    }

    // 3. Converte os dados para criar o objeto Polygon
    int num_indices = (num_verts - 2) * 3;
    std::vector<unsigned int> final_indices(num_indices);
    int* raw_indices = indices_ptr.get();
    for(int i = 0; i < num_indices; ++i) {
        final_indices[i] = static_cast<unsigned int>(raw_indices[i]);
    }

    std::vector<float> positions, colors;
    positions.reserve(num_verts * 2);
    colors.reserve(num_verts * 3);
    for(int i = 0; i < num_verts; ++i) {
        positions.push_back(current_vertices_data[i*5 + 0]);
        positions.push_back(current_vertices_data[i*5 + 1]);
        colors.push_back(current_vertices_data[i*5 + 2]);
        colors.push_back(current_vertices_data[i*5 + 3]);
        colors.push_back(current_vertices_data[i*5 + 4]);
    }
    
    scene_polygons.push_back(Polygon::Make(positions.data(), colors.data(), final_indices.data(), num_verts, final_indices.size()));
    current_vertices_data.clear();
}

void handleMouseClick(float x, float y, int button) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        is_drawing ? finishPolygon() : startPolygon(x, y);
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && is_drawing) {
        addVertexToPolygon(x, y);
    }
}

// --- Funções de Setup e Renderização ---
void initialize(ShaderPtr shader) {
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

void cleanup() {
    glDeleteBuffers(1, &previewVBO);
    glDeleteVertexArrays(1, &previewVAO);
}

void drawPreview() {
    if (!is_drawing || current_vertices_data.empty()) return;
    int num_verts = current_vertices_data.size() / 5;
    
    glBindBuffer(GL_ARRAY_BUFFER, previewVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * current_vertices_data.size(), current_vertices_data.data());
    
    glBindVertexArray(previewVAO);
    glPointSize(10.0f);
    glDrawArrays(GL_POINTS, 0, num_verts);
    glDrawArrays(GL_LINE_STRIP, 0, num_verts);
    glBindVertexArray(0);
}

void drawScene() {
    for (const auto& polygon : scene_polygons) {
        polygon->Draw();
    }
}

void clearScene() {
    scene_polygons.clear();
    current_vertices_data.clear();
    is_drawing = false;
    std::cout << "Cena limpa." << std::endl;
}

} // namespace drawing
#endif

