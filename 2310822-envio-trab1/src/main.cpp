#define GLAD_GL_IMPLEMENTATION // Necessary for headeronly version.
#include "gl_includes.h"
#include <iostream>

#include "error.h"
#include "input_handlers.h"
#include "shader.h"

#include "polygon.h"


static GLFWwindow* WindowSetup(int width, int height);
static void error (int code, const char* msg);

ShaderPtr shd;

float posicoes[] = {
  -0.6f, -0.6f,
   0.5f, -0.5f,
   0.5f,  0.5f,
  -0.6f,  0.6f,
   0.0f,  0.3f,
   0.0f, -0.3f,
};

float cores[] = {
  1.0f, 0.0f, 0.0f, // vermelho
  0.0f, 1.0f, 0.0f, // verde
  0.0f, 0.0f, 1.0f, // azul
  0.0f, 1.0f, 1.0f, // ciano
  1.0f, 0.0f, 1.0f, // magenta
  1.0f, 1.0f, 0.0f  // amarelo
};

unsigned int indices[] = {
  0, 1, 5,  // triângulo 1
  1, 2, 5,  // triângulo 2
  2, 4, 5,  // triângulo 3
  2, 3, 4   // triângulo 4
};

PolygonPtr polygon;

static void initialize()
{
  // config do OpenGL
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  // glFrontFace(GL_CCW);
  // glCullFace(GL_BACK);
  // glEnable(GL_CULL_FACE);
  // glPolygonMode(GL_FRONT, GL_FILL); // ERRADO

  Error::Check("setup");

  // CENTERPIECE
  // inicia Shader Program
  shd = Shader::Make();
  shd->AttachVertexShader("../shaders/vertex.glsl");
  shd->AttachFragmentShader("../shaders/fragment.glsl");
  shd->Link();
  
  Error::Check("shaders");
  // inicia geometria estática
  polygon = Polygon::Make(posicoes, cores, indices, 6, 12);
  Error::Check("polygon");
}

static void display(GLFWwindow * win)
{
  // CENTERPIECE
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glFlush();
  shd->UseProgram();

  // desenha geometria
  polygon->Draw();
  
  // errorcheck
  Error::Check("display");
}

int main(void) {

    GLFWwindow* win = WindowSetup(800, 600);

    setInputCallbacks(win);

    Error::Check("pre initialize"); 

    initialize();
    glFlush();
    Error::Check("initialize");

    while (!glfwWindowShouldClose(win)) {
      display(win);
      glfwSwapBuffers(win);
      glFlush();
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

  return win;
}