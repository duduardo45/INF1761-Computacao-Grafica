#ifndef INPUT_HANDLERS_H
#define INPUT_HANDLERS_H
#pragma once
#include "gl_includes.h"
#include "drawing.h"
#include <iostream>

static void keyboard(GLFWwindow * window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_Q && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    else if (key == GLFW_KEY_C && action == GLFW_PRESS)
        drawing::clearScene();
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

        int fb_w, fb_h;
        glfwGetFramebufferSize(win, &fb_w, &fb_h);
        
        // Converte coordenadas de tela (pixels) para Coordenadas de Dispositivo Normalizadas (NDC) [-1, 1]
        float x_ndc = ((float)xpos / (float)fb_w) * 2.0f - 1.0f;
        float y_ndc = (1.0f - ((float)ypos / (float)fb_h)) * 2.0f - 1.0f;

        drawing::handleMouseClick(x_ndc, y_ndc, button);
    }
}

static void resize(GLFWwindow * win, int width, int height)
{
    glViewport(0, 0, width, height);
}

static void setInputCallbacks(GLFWwindow * win) {
    glfwSetFramebufferSizeCallback(win, resize);
    glfwSetKeyCallback(win, keyboard);
    glfwSetMouseButtonCallback(win, mousebutton);
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