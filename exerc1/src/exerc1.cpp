#define GLAD_GL_IMPLEMENTATION // Necessary for headeronly version.
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include<memory>


#include "quad.h"


static void initialize()
{
 glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
}

static void error (int code, const char* msg)
{
  printf("GLFW error %d: %s\n", code, msg);
  glfwTerminate();
  exit(0);
}

static void keyboard(GLFWwindow * window, int key, int scancode, int action, int mods)
{
 if (key == GLFW_KEY_Q && action == GLFW_PRESS)
  glfwSetWindowShouldClose(window, GLFW_TRUE);
}

static void resize(GLFWwindow * win, int width, int height)
{
 glViewport(0, 0, width, height);
}

static void display(GLFWwindow * win)
{
 glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

int main(void) {

    glfwSetErrorCallback(error);
    if (glfwInit() != GLFW_TRUE) {
      std::cerr << "Could not initialize GLFW" << std::endl;
      return 0;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    // DIMENSOES
    GLFWwindow* win = glfwCreateWindow(600, 400, "Window title", nullptr, nullptr);
    if (!win) {
        std::cerr << "Could not create GLFW window" << std::endl;
        return 0;
      }
    glfwMakeContextCurrent(win);

    #ifdef GLAD_GL_H_
      if (!gladLoadGL(glfwGetProcAddress)) {
        printf("Failed to initialize GLAD OpenGL context\n");
        exit(1);
      }
    #endif

    glfwSetFramebufferSizeCallback(win, resize);  // resize callback
    glfwSetKeyCallback(win, keyboard);   // keyboard callback
    glfwSetMouseButtonCallback(win, mousebutton); // mouse button callback

    initialize();


    // CENTERPIECE
    while (!glfwWindowShouldClose(win)) {
      display(win);
      glfwSwapBuffers(win);
      glfwPollEvents();
    }



    glfwTerminate();
    return 0;
}