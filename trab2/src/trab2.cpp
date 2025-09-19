#define GLAD_GL_IMPLEMENTATION // Necessary for headeronly version.
#include "gl_includes.h"
#include <iostream>

#include "error.h"
#include "input_handlers.h"
// #include "shader.h"
#include "scene.h"
#include "circle.h"

// ShaderPtr shd;

static void initialize()
{
  // config do OpenGL
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  // glFrontFace(GL_CCW);
  // glCullFace(GL_BACK);
  // glEnable(GL_CULL_FACE);
  // glPolygonMode(GL_FRONT, GL_FILL); // ERRADO

  // inicia Shader Program e SceneGraph
  scene::graph()->initializeBaseShader("../shaders/vertex.glsl","../shaders/fragment.glsl");
  // scene::graph()->setView(0,1,0,1,0,1);

  // CENTERPIECE
  // inicia geometria estática
  CirclePtr face_relogio = Circle::Make(
    0.0f, // x_center
    0.0f, // y_center
    0.95f, // radius
    (float[]){1.0f, 1.0f, 1.0f}, // corRGB
    64, // pontos para discretização
    false // se tem ponto no centro ou não
  );
  scene::graph()
  ->getRoot() // não tive tempo ainda de fazer uma interface que abstrai a necessidade de pegar a raiz
  ->addChild(scene::Node::MakeNode("face_relogio", face_relogio, nullptr, transform::Transform::Make()));

  PolygonPtr triangulo_ponteiro = Polygon::Make(
    (float[]){ // vertices
      1.0f, 0.0f, 
      -1.0f, 0.0f, 
      0.0f, 1.0f,
    }, 
    (float[]){ // cores
      1.0f, 0.0f, 0.0f, 
      1.0f, 0.0f, 0.0f, 
      1.0f, 0.0f, 0.0f,
    }, 
    (unsigned int[]){ // incidencia
      0,1,2, 
      2,3,4
    }, 
    3, // numero de vertices
    3 // numero de indices
  );

  transform::TransformPtr segundo_agora = transform::Transform::Make();
  scene::graph()
  ->getRoot() // adiciona a rotação antes da geometria
  ->addChild(scene::Node::MakeNode("segundo_agora", nullptr, nullptr, segundo_agora));

    // transforma o triangulo base para o formato de um ponteiro
  transform::TransformPtr ponteiro_segundos = transform::Transform::Make();
  ponteiro_segundos->translate(0.0f, 0.01f, 0.1f);
  ponteiro_segundos->scale(0.02f, 0.7f, 1.0f);

  scene::graph()
  ->getRoot()
  ->getChildByName("segundo_agora")
  ->addChild(scene::Node::MakeNode("ponteiro_horas", triangulo_ponteiro, nullptr, ponteiro_segundos));
  
  // minuto agora
  transform::TransformPtr minuto_agora = transform::Transform::Make();
  scene::graph()
  ->getRoot() // adiciona a rotação antes da geometria
  ->addChild(scene::Node::MakeNode("minuto_agora", nullptr, nullptr, minuto_agora));

  // adiciona o ponteiro dos minutos
  transform::TransformPtr ponteiro_minutos = transform::Transform::Make();
  ponteiro_minutos->translate(0.0f, 0.01f, 0.1f);
  ponteiro_minutos->scale(0.04f, 0.9f, 1.0f);
  scene::graph()
  ->getRoot()
  ->getChildByName("minuto_agora")
  ->addChild(scene::Node::MakeNode("minuto_agora", triangulo_ponteiro, nullptr, ponteiro_minutos));

  // hora agora
  transform::TransformPtr hora_agora = transform::Transform::Make();
  scene::graph()
  ->getRoot() // adiciona a rotação antes da geometria
  ->addChild(scene::Node::MakeNode("hora_agora", nullptr, nullptr, hora_agora));

  // adiciona o ponteiro das horas
  transform::TransformPtr ponteiro_horas = transform::Transform::Make();
  ponteiro_horas->translate(0.0f, 0.01f, 0.1f);
  ponteiro_horas->scale(0.04f, 0.4f, 1.0f);
  scene::graph()
  ->getRoot()
  ->getChildByName("hora_agora")
  ->addChild(scene::Node::MakeNode("hora_agora", triangulo_ponteiro, nullptr, ponteiro_horas));
}

static void display(GLFWwindow * win)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // CENTERPIECE
  // atualiza geometria dinâmica

  // pega a hora atual
  time_t now = time(0);
  tm *ltm = localtime(&now);
  int segundos = ltm->tm_sec;
  int minutos = ltm->tm_min;
  int horas = ltm->tm_hour;
  // calcula o ângulo dos ponteiros
  float angulo_segundos = -(360.0f * segundos / 60.0f); // 0 graus é o "topo" do relógio
  float angulo_minutos = -(360.0f * minutos / 60.0f + 6.0f * segundos / 60.0f); // cada minuto são 6 graus, mais o avanço dos segundos
  float angulo_horas = -(360.0f * (horas % 12) / 12.0f + 30.0f * minutos / 60.0f); // cada hora são 30 graus, mais o avanço dos minutos

  scene::graph()
  ->getRoot()
  ->getChildByName("segundo_agora")
  ->getTransform()
  ->setRotate(angulo_segundos, 0.0f, 0.0f, 1.0f);

  scene::graph()
  ->getRoot()
  ->getChildByName("minuto_agora")
  ->getTransform()
  ->setRotate(angulo_minutos, 0.0f, 0.0f, 1.0f);

  scene::graph()
  ->getRoot()
  ->getChildByName("hora_agora")
  ->getTransform()
  ->setRotate(angulo_horas, 0.0f, 0.0f, 1.0f);

  // desenha geometria
  scene::graph()->draw();
  
  // errorcheck
  Error::Check("display");
}

static GLFWwindow* WindowSetup(int width, int height);

int main(void) {

    GLFWwindow* win = WindowSetup(1000, 1000);

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

  if (!gladLoadGL(glfwGetProcAddress)) {
    printf("Failed to initialize GLAD OpenGL context\n");
    exit(1);
  }
  return win;
}