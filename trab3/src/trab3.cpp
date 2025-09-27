#define GLAD_GL_IMPLEMENTATION // Necessary for headeronly version.
#include "gl_includes.h"
#include <iostream>

#include "error.h"
#include "input_handlers.h"
// #include "shader.h"
#include "scene.h"
#include "circle.h"

#define BACKGROUND_COLOR 0.1f, 0.1f, 0.1f

// ShaderPtr shd;

double savedTime = 0;
double updateTimer = 0;
const double updateInterval = 1.0/60.0; // 60 fps

#define sg scene::graph()
float contador_rotacao_terra = 0;
float contador_rotacao_lua = 0;

int contador_debug = 0;

    #include <chrono>
    #include <thread>
    using namespace std::chrono_literals;
    auto delay = [](int ms) { std::this_thread::sleep_for(ms * 1ms); };

static void initialize()
{
  // config do OpenGL
  glClearColor(BACKGROUND_COLOR, 1.0f);
  // glFrontFace(GL_CCW);
  // glCullFace(GL_BACK);
  // glEnable(GL_CULL_FACE);
  // glPolygonMode(GL_FRONT, GL_FILL); // ERRADO

  // inicia Shader Program e SceneGraph
  sg->initializeBaseShader("../shaders/vertex.glsl","../shaders/fragment.glsl");
  // sg->setView(0,1,0,1,0,1);

  // CENTERPIECE
  // inicia geometria estática
  sg->addNode(
    "sol",
    Circle::Make(
      0.0f, 0.0f, // pos centro
      0.3f, // raio
      (float[]) { // cores
        0.8f, 0.5f, 0.0f, // centro
        BACKGROUND_COLOR, // borda
      },
      32,
      true
    )
  );

  std::cout << contador_debug++ << ": " << sg->getCurrentNode()->getName() << std::endl; // DEBUG

  sg->addNodeToCurrent(
    "terra",
    Circle::Make(
      0.0f, 0.0f, // pos centro
      0.1f, // raio
      (float[]) { // cores
        0.0f, 0.1f, 0.5f // centro
      },
      32,
      false
    )
  );

  std::cout << contador_debug++ << ": " << sg->getCurrentNode()->getName() << std::endl; // DEBUG
  
  sg->newNodeAbove("distancia_terra_sol");

  std::cout << contador_debug++ << ": " << sg->getCurrentNode()->getName() << std::endl; // DEBUG

  sg->translateCurrentNode(0.7f, 0.0f, 0.0f);

  sg->newNodeAbove("rotacao_terra");

  std::cout << contador_debug++ << ": " << sg->getCurrentNode()->getName() << std::endl; // DEBUG

  sg->lookAtNode("terra");

  std::cout << contador_debug++ << ": " << sg->getCurrentNode()->getName() << std::endl; // DEBUG

  sg->addNodeToCurrent(
    "lua",
    Circle::Make(
      0.0f, 0.0f, // pos centro
      0.03f, // raio
      (float[]) { // cores
        0.7f, 0.7f, 0.7f // centro
      },
      16,
      false
    )
  );

  std::cout << contador_debug++ << ": " << sg->getCurrentNode()->getName() << std::endl; // DEBUG

  sg->newNodeAbove("distancia_lua_terra");

  std::cout << contador_debug++ << ": " << sg->getCurrentNode()->getName() << std::endl; // DEBUG

  sg->translateCurrentNode(-0.2f, 0.0f, 0.0f);

  sg->newNodeAbove("rotacao_lua");

  std::cout << contador_debug++ << ": " << sg->getCurrentNode()->getName() << std::endl; // DEBUG

  Error::Check("initialize");
}

static void display(GLFWwindow * win)
{
  glFlush();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // CENTERPIECE
  // atualiza geometria dinâmica
  Error::Check("display - antes de atualizar geometria dinâmica");

  sg->lookAtNode("rotacao_terra");
  sg->rotateCurrentNode(0.1f, 0,0,1);
  // contador_rotacao_terra = (contador_rotacao_terra + 1.0f);
  // printf("\ncontador_rotacao_terra: %f\n", contador_rotacao_terra);

  sg->lookAtNode("rotacao_lua");
  sg->rotateCurrentNode(0.5f, 0,0,1);
  // contador_rotacao_lua = (contador_rotacao_lua + 5.0f);
  // printf("contador_rotacao_lua: %f\n", contador_rotacao_lua);

  Error::Check("display - depois de atualizar geometria dinâmica");
  // desenha geometria
  sg->draw();
  
  // errorcheck
  Error::Check("display");
}

static GLFWwindow* WindowSetup(int width, int height);

int main(void) {

    GLFWwindow* win = WindowSetup(800, 800);

    setInputCallbacks(win);

    initialize();
  
    while (!glfwWindowShouldClose(win)) {
      double currentTime =  glfwGetTime();
      double elapseTime = currentTime - savedTime;
      savedTime = currentTime;
        
      updateTimer += elapseTime;

      if(updateTimer >= updateInterval){
        display(win);
        glfwSwapBuffers(win);
        glfwPollEvents();
        updateTimer = 0;
      }
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

  if (!gladLoadGL(glfwGetProcAddress)) {
    printf("Failed to initialize GLAD OpenGL context\n");
    exit(1);
  }
  return win;
}