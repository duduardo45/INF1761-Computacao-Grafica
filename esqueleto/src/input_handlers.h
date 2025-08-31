#ifndef INPUT_HANDLERS_H
#define INPUT_HANDLERS_H

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>

static void setInputCallbacks(GLFWwindow * win) {
    glfwSetFramebufferSizeCallback(win, resize);  // resize callback
    glfwSetKeyCallback(win, keyboard);   // keyboard callback
    glfwSetMouseButtonCallback(win, mousebutton); // mouse button callback
}

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

#endif