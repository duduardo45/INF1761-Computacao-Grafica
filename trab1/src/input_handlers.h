#ifndef INPUT_HANDLERS_H
#define INPUT_HANDLERS_H
#pragma once
#include "gl_includes.h"
#include "drawing.h" // Inclui a nossa nova lógica de desenho
#include <iostream>

static void keyboard(GLFWwindow * window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_Q && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

static void cursorpos(GLFWwindow * win, double xpos, double ypos)
{
 // convert screen pos (upside down) to framebuffer pos (e.g., retina displays)
 int wn_w, wn_h, fb_w, fb_h;
 glfwGetWindowSize(win, &wn_w, &wn_h);
 glfwGetFramebufferSize(win, &fb_w, &fb_h);
 double x = xpos * fb_w / wn_w;
 double y = (wn_h - ypos) * fb_h / wn_h;
 std::cout << "(x,y): " << x << ", " << y << std::endl;
}

static void mousebutton(GLFWwindow * win, int button, int action, int mods)
{
    if (action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(win, &xpos, &ypos);

        // Converte a posição da tela para a posição do framebuffer (considerando displays retina)
        // E inverte o eixo Y para corresponder ao sistema de coordenadas do OpenGL
        int win_w, win_h, fb_w, fb_h;
        glfwGetWindowSize(win, &win_w, &win_h);
        glfwGetFramebufferSize(win, &fb_w, &fb_h);
        
        float x_world = (float)(xpos * fb_w / win_w);
        float y_world = (float)((win_h - ypos) * fb_h / win_h);

        // Despacha o evento para o nosso manipulador de desenho
        drawing::handleMouseClick(x_world, y_world, button);
    }
}

static void resize(GLFWwindow * win, int width, int height)
{
    glViewport(0, 0, width, height);
}

static void setInputCallbacks(GLFWwindow * win) {
    glfwSetFramebufferSizeCallback(win, resize);   // Callback de redimensionamento
    glfwSetKeyCallback(win, keyboard);           // Callback de teclado
    glfwSetMouseButtonCallback(win, mousebutton); // Callback de botões do mouse
}

#endif

#ifndef INPUT_HANDLERS_H
#define INPUT_HANDLERS_H
#pragma once
#include "gl_includes.h"
#include <iostream>

static void keyboard(GLFWwindow * window, int key, int scancode, int action, int mods)
{
 if (key == GLFW_KEY_Q && action == GLFW_PRESS)
  glfwSetWindowShouldClose(window, GLFW_TRUE);
}

static void cursorpos(GLFWwindow * win, double xpos, double ypos)
{
 // convert screen pos (upside down) to framebuffer pos (e.g., retina displays)
 int wn_w, wn_h, fb_w, fb_h;
 glfwGetWindowSize(win, &wn_w, &wn_h);
 glfwGetFramebufferSize(win, &fb_w, &fb_h);
 double x = xpos * fb_w / wn_w;
 double y = (wn_h - ypos) * fb_h / wn_h;
 std::cout << "(x,y): " << x << ", " << y << std::endl;
}

static void mousebutton(GLFWwindow * win, int button, int action, int mods)
{
 if (action == GLFW_PRESS) {
  switch (button) {
  case GLFW_MOUSE_BUTTON_1:
   std::cout << "button 1" << std::endl;
   break;
  case GLFW_MOUSE_BUTTON_2:
   std::cout << "button 2" << std::endl;
   break;
  case GLFW_MOUSE_BUTTON_3:
   std::cout << "button 3" << std::endl;
   break;
  }
 }
 if (action == GLFW_PRESS)
  glfwSetCursorPosCallback(win, cursorpos);  // cursor position callback
 else // GLFW_RELEASE 
  glfwSetCursorPosCallback(win, nullptr);   // callback disabled
}

static void resize(GLFWwindow * win, int width, int height)
{
 glViewport(0, 0, width, height);
}

static void setInputCallbacks(GLFWwindow * win) {
    glfwSetFramebufferSizeCallback(win, resize);  // resize callback
    glfwSetKeyCallback(win, keyboard);   // keyboard callback
    glfwSetMouseButtonCallback(win, mousebutton); // mouse button callback
}

#endif