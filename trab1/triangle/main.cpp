#ifdef _WIN32
#include <windows.h>

// Use glad to initialize OpenGL context on Windows
#include <glad/glad.h>

#include <GLFW/glfw3.h>

#else

#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>

#endif


#include "error.h"
#include "triangle.h"
#include "shader.h"

#include <stdio.h>
#include <stdlib.h>

static TrianglePtr tri;
static ShaderPtr shd;

static void error (int code, const char* msg)
{
  printf("GLFW error %d: %s\n", code, msg);
  glfwTerminate();
  exit(1);
}

static void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_Q && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}

static void resize (GLFWwindow* win, int width, int height)
{
  glViewport(0,0,width,height);
}

static void initialize ()
{
  glClearColor(1.0f,1.0f,1.0f,1.0f);
  tri = Triangle::Make();
  shd = Shader::Make();
  shd->AttachVertexShader("shaders/vertex.glsl");
  shd->AttachFragmentShader("shaders/fragment.glsl");
  shd->Link();
  Error::Check("initialize");
}

static void display (GLFWwindow* win)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  shd->UseProgram();
  tri->Draw();
  Error::Check("display");
}

int main ()
{
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);
  glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);       // required for mac os
  //glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER,GLFW_TRUE);  // option for mac os

  glfwSetErrorCallback(error);

  GLFWwindow* win = glfwCreateWindow(600,400,"Triangle test",nullptr,nullptr);
  glfwSetFramebufferSizeCallback(win, resize);  // resize callback
  glfwSetKeyCallback(win, keyboard);            // keyboard callback
  
  glfwMakeContextCurrent(win);
 
#ifdef __glad_h_
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    printf("Failed to initialize GLAD OpenGL context\n");
    exit(1);
   }
#endif
#ifdef __glew_h__
  glewInit(); 
  if (glewInit() != GLEW_OK) {
    printf("Failed to initialize GLEW OpenGL context\n");
    exit(1);
  }
#endif

  printf("OpenGL version: %s\n", glGetString(GL_VERSION));

  initialize();

  while(!glfwWindowShouldClose(win)) {
    //idle(win);
    display(win);
    glfwSwapBuffers(win);
    glfwPollEvents();
  }
  glfwTerminate();
  return 0;
}
