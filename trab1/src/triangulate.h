#ifndef TRIANGULATE_H
#define TRIANGULATE_H
#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <cmath>
#include <cassert>

// Estrutura auxiliar para facilitar o manuseio de pontos 2D
struct Point {
    float x, y;
};

// Nó da lista duplamente ligada que representa o polígono ativo
struct Node {
    int index;          // Índice do vértice no array original `pontos`
    bool is_convex;     // O vértice é convexo no polígono atual?
    Node* prev = nullptr;
    Node* next = nullptr;
};

// --- Funções Auxiliares ---

// Calcula o produto vetorial 2D (na verdade, a componente Z do resultado 3D)
// para o giro do vetor P1->P2 para P2->P3.
// Sinal > 0: Giro para a esquerda (anti-horário)
// Sinal < 0: Giro para a direita (horário)
// Sinal = 0: Colinear
float CrossProduct(Point p1, Point p2, Point p3) {
    return (p2.x - p1.x) * (p3.y - p2.y) - (p2.y - p1.y) * (p3.x - p2.x);
}

// Verifica se um ponto P está dentro do triângulo ABC
// Assume que A, B, C estão em ordem anti-horária (CCW).
bool IsPointInTriangle(Point p, Point a, Point b, Point c) {
    // Usando a técnica "same-side" com produto vetorial.
    // O ponto deve estar à esquerda de todas as três arestas (AB, BC, CA).
    float cross_ab = CrossProduct(a, b, p);
    float cross_bc = CrossProduct(b, c, p);
    float cross_ca = CrossProduct(c, a, p);
    
    // Se todos os sinais forem positivos (ou zero), o ponto está dentro ou na borda.
    return (cross_ab >= 0.0f) && (cross_bc >= 0.0f) && (cross_ca >= 0.0f);
}

/**
 * @brief Triangula um polígono simples usando o algoritmo Ear Clipping.
 * * @param pontos Ponteiro para um array de floats [x0, y0, x1, y1, ...].
 * @param n_vertices O número de vértices no polígono.
 * @return Um shared_ptr para um novo array de ints contendo os vértices dos triângulos.
 * O tamanho do array será (n_vertices - 2) * 3 (vértices) * 2 (coords).
 * Retorna nullptr se a triangulação não for possível (n_vertices < 3).
 */
std::shared_ptr<int> TriangulateEarClipping(const float* pontos, int n_vertices) {
    // --- Validação de Entrada ---
    if (n_vertices < 3) {
        return nullptr;
    }

    // --- Fase 1: Configuração Inicial e Classificação ---
    
    // Copia os pontos para uma estrutura mais conveniente
    std::vector<Point> vertex_coords(n_vertices);
    for (int i = 0; i < n_vertices; ++i) {
        vertex_coords[i] = {pontos[2 * i], pontos[2 * i + 1]};
    }

    // Cria a lista duplamente ligada de nós
    std::vector<Node*> polygon_nodes(n_vertices);
    for (int i = 0; i < n_vertices; ++i) {
        polygon_nodes[i] = new Node{i};
    }

    // Conecta os nós para formar um anel
    for (int i = 0; i < n_vertices; ++i) {
        polygon_nodes[i]->prev = polygon_nodes[(i + n_vertices - 1) % n_vertices];
        polygon_nodes[i]->next = polygon_nodes[(i + 1) % n_vertices];
    }

    // Calcula a convexidade inicial e a orientação do polígono
    float polygon_orientation_sum = 0.0f;
    for (int i = 0; i < n_vertices; ++i) {
        Node* current = polygon_nodes[i];
        float cross_val = CrossProduct(
            vertex_coords[current->prev->index],
            vertex_coords[current->index],
            vertex_coords[current->next->index]
        );
        current->is_convex = (cross_val > 0.0f);
        polygon_orientation_sum += cross_val;
    }

    // Se a soma é negativa, o polígono está em ordem horária.
    // Nossa definição de "convexo" (giro à esquerda) estava invertida. Corrigimos.
    if (polygon_orientation_sum < 0.0f) {
        for (int i = 0; i < n_vertices; ++i) {
            polygon_nodes[i]->is_convex = !polygon_nodes[i]->is_convex;
        }
    }

    // --- Fase 2: Loop de Ear Clipping ---

    std::vector<int> result_indices;
    result_indices.reserve((n_vertices - 2) * 3);
    
    int vertices_left = n_vertices;
    Node* current_node = polygon_nodes[0];
    int watchdog = n_vertices * n_vertices*3; // Prevenção contra loops infinitos

    while (vertices_left > 3 && watchdog-- > 0) {
        bool ear_found = false;
        
        // 1. O nó atual é um candidato a orelha? (Precisa ser convexo)
        if (current_node->is_convex) {
            Node* prev_node = current_node->prev;
            Node* next_node = current_node->next;

            bool is_valid_ear = true;

            // 2. Validar a orelha: verificar se nenhum vértice reflexo está dentro dela
            Node* test_node = next_node->next;
            while (test_node != prev_node) {
                if (!test_node->is_convex) { // Testar apenas contra vértices reflexos
                    if (IsPointInTriangle(
                        vertex_coords[test_node->index],
                        vertex_coords[prev_node->index],
                        vertex_coords[current_node->index],
                        vertex_coords[next_node->index])) 
                    {
                        is_valid_ear = false;
                        break;
                    }
                }
                test_node = test_node->next;
            }

            // 3. Se a orelha for válida, processe-a
            if (is_valid_ear) {
                // Adiciona o triângulo à lista de resultados
                result_indices.push_back(prev_node->index);
                result_indices.push_back(current_node->index);
                result_indices.push_back(next_node->index);

                // Remove a orelha da lista ligada
                prev_node->next = next_node;
                next_node->prev = prev_node;
                
                delete current_node; // Libera a memória do nó removido
                vertices_left--;

                // Reavalia a convexidade dos vizinhos que foram afetados
                float cross_prev = CrossProduct(vertex_coords[prev_node->prev->index], vertex_coords[prev_node->index], vertex_coords[next_node->index]);
                prev_node->is_convex = (polygon_orientation_sum > 0.0f) ? (cross_prev > 0.0f) : (cross_prev < 0.0f);

                float cross_next = CrossProduct(vertex_coords[prev_node->index], vertex_coords[next_node->index], vertex_coords[next_node->next->index]);
                next_node->is_convex = (polygon_orientation_sum > 0.0f) ? (cross_next > 0.0f) : (cross_next < 0.0f);
                
                current_node = prev_node; // Continua a busca a partir do próximo nó
                ear_found = true;
            }
        }

        if (!ear_found) {
            current_node = current_node->next;
        }
    }
    if (watchdog <= 0) {
       // O polígono pode ser inválido (ex: com auto-intersecção)
       // Limpando memória antes de sair
       Node* node_to_delete = current_node;
       for(int i = 0; i < vertices_left; ++i) {
           Node* next = node_to_delete->next;
           delete node_to_delete;
           node_to_delete = next;
       }
       return nullptr;
    }


    // --- Fase 3: Finalização ---

    // Adiciona o último triângulo restante
    result_indices.push_back(current_node->prev->index);
    result_indices.push_back(current_node->index);
    result_indices.push_back(current_node->next->index);

    // Libera a memória dos 3 nós restantes
    delete current_node->prev;
    delete current_node->next;
    delete current_node;

    // Constrói o array final de índices
    const size_t result_size = result_indices.size();
    int* final_indices = new int[result_size];
    
    // Copia os índices do vetor para o array final
    for (size_t i = 0; i < result_size; ++i) {
        final_indices[i] = result_indices[i];
    }
    
    // Retorna o array de índices gerenciado por um shared_ptr
    return std::shared_ptr<int>(final_indices, std::default_delete<int[]>());
}
#endif