#define GLAD_GL_IMPLEMENTATION // Necessary for headeronly version.
#include "gl_includes.h"
#include <iostream>

#include "error.h"
#include "input_handlers.h"
#include "shader.h"

#include "polygon.h"
#include "drawing.h" // Incluído para a nova funcionalidade

static GLFWwindow* WindowSetup(int width, int height);
static void error (int code, const char* msg);

ShaderPtr shd;

static void initialize()
{
    // config do OpenGL
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_PROGRAM_POINT_SIZE); // Permite que o shader controle o tamanho do ponto
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    Error::Check("setup");

  // CENTERPIECE
    // inicia Shader Program
    shd = Shader::Make();
    shd->AttachVertexShader("../shaders/vertex.glsl");
    shd->AttachFragmentShader("../shaders/fragment.glsl");
    shd->Link();
    
    Error::Check("shaders");
    
    // Inicializa os recursos necessários para o desenho interativo
    drawing::initialize(shd);
    Error::Check("drawing::initialize");
}

static void display(GLFWwindow * win)
{
  // CENTERPIECE
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shd->UseProgram();

    // Desenha os polígonos já finalizados
    drawing::drawScene();
    
    // Desenha o preview (pontos e linhas) do polígono em construção
    drawing::drawPreview();
    
    Error::Check("display");
}

int main(void) {
    GLFWwindow* win = WindowSetup(800, 600);
    setInputCallbacks(win);
    Error::Check("pre initialize"); 
    initialize();
    Error::Check("initialize");

    while (!glfwWindowShouldClose(win)) {
        display(win);
        glfwSwapBuffers(win);
        glFlush();
        glfwPollEvents();
    }
    
    // Libera os recursos do OpenGL criados em drawing.h
    drawing::cleanup();

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