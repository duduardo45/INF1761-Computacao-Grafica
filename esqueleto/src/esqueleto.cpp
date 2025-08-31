#define GLAD_GL_IMPLEMENTATION // Necessary for headeronly version.
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include "error.h"
#include "input_handlers.h"
#include "shader.h"

ShaderPtr shd;

static void initialize()
{
  // config do OpenGL
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  // glFrontFace(GL_CCW);
  // glCullFace(GL_BACK);
  // glEnable(GL_CULL_FACE);
  // glPolygonMode(GL_FRONT, GL_FILL); // ERRADO

  // CENTERPIECE
  // inicia Shader Program
  shd = Shader::Make();
  shd->AttachVertexShader("../shaders/vertex.glsl");
  shd->AttachFragmentShader("../shaders/fragment.glsl");
  shd->Link();

  // inicia geometria estática

}

static void display(GLFWwindow * win)
{
  // CENTERPIECE
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glFlush();
  shd->UseProgram();

  // desenha geometria

  
  // errorcheck
  Error::Check("display");
}

int main(void) {

    GLFWwindow* win = WindowSetup(800, 600);

    setInputCallbacks(win);

    initialize();

    while (!glfwWindowShouldClose(win)) {
      display(win);
      glfwSwapBuffers(win);
      glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}





static void error (int code, const char* msg)
{
  printf("GLFW error %d: %s\n", code, msg);
  glfwTerminate();
  exit(0);
}

static GLFWwindow* WindowSetup(int width, int height) {
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
  GLFWwindow* win = glfwCreateWindow(width, height, "Window title", nullptr, nullptr);
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
}