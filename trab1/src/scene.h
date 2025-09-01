#ifndef SCENE_H
#define SCENE_H
#pragma once

#include "gl_includes.h"
#include "Polygon.h"
#include "drawing.h"
#include <vector>
#include <memory>
#include <cmath>
#include <iostream>

// Encapsulamos toda a lógica de desenho em um namespace para organização.
namespace scene {

class Collection {
public:
    std::vector<PolygonPtr> polygons;
    void addPolygon(PolygonPtr poly) {
        polygons.push_back(poly);
    }
    void clear() {
        polygons.clear();
    }
    void Draw() {
        for (const auto& poly : polygons) {
            poly->Draw();
        }
    }
};

Collection scene;
Drawing current_drawing;

// Cores predefinidas para os vértices
const float PRESET_COLORS[][3] = {
    {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.0f}
};
const int NUM_PRESET_COLORS = 6;

// --- Funções de Manipulação de Desenho ---
void startPolygon(float x, float y) {
    int color_index = 0; // Sempre começa com a primeira cor
    const float* color = PRESET_COLORS[color_index];
    current_drawing.start(x, y, color);
    std::cout << "Polígono iniciado no ponto (" << x << ", " << y << ")" << std::endl;
}

void addVertexToPolygon(float x, float y) {
    int color_index = (current_drawing.numVertices()) % NUM_PRESET_COLORS;
    const float* color = PRESET_COLORS[color_index];
    current_drawing.addVertex(x, y, color);
    std::cout << "Vértice adicionado no ponto (" << x << ", " << y << ")" << std::endl;
}

void finishPolygon() {
    int num_verts = current_drawing.numVertices();
    PolygonPtr polygon = current_drawing.finalize();
    if (polygon) {
        scene.addPolygon(polygon);
        std::cout << "Polígono finalizado com " << num_verts << " vértices." << std::endl;
    }
}

void handleMouseClick(float x, float y, int button) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        current_drawing.is_drawing ? finishPolygon() : startPolygon(x, y);
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && current_drawing.is_drawing) {
        addVertexToPolygon(x, y);
    }
}

// --- Funções de Setup e Renderização ---
void initialize(ShaderPtr shader) {
    current_drawing.gl_initialize(shader);
}

void cleanup() {
    current_drawing.gl_cleanup();
}

void drawPreview() {
    current_drawing.gl_drawPreview();
}

void drawScene() {
    scene.Draw();
}

void clearScene() {
    scene.clear();
    current_drawing.clear();
    std::cout << "Cena limpa." << std::endl;
}

} // namespace drawing
#endif

