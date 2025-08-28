#define GLAD_GL_IMPLEMENTATION // Necessary for headeronly version.
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include<memory>

// BACALHAU
#include <string>
#include <vector>
#include <fstream>
#include <sstream>


#include "quad.h"

GLuint createShaderProgram(const std::string& vertexPath, const std::string& fragmentPath);

// BACALHAU
std::string readShaderFile(const std::string& filePath) {
    std::ifstream shaderFile;
    // Garante que ifstream pode lançar exceções em caso de erro
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        shaderFile.open(filePath);
        std::stringstream shaderStream;
        shaderStream << shaderFile.rdbuf(); // Lê o buffer do ficheiro para um stream
        shaderFile.close();
        return shaderStream.str(); // Converte o stream para string
    } catch (const std::ifstream::failure& e) {
        std::cerr << "ERRO::SHADER::FICHEIRO_NAO_LIDO: " << filePath << "\n" << e.what() << std::endl;
        return "";
    }
}


// BACALHAU
GLuint createShaderProgram(const std::string& vertexPath, const std::string& fragmentPath) {
    // 1. Obter o código fonte dos ficheiros
    std::string vertexCode = readShaderFile(vertexPath);
    std::string fragmentCode = readShaderFile(fragmentPath);

    if (vertexCode.empty() || fragmentCode.empty()) {
        return 0; // Retorna 0 para indicar falha
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    // 2. Compilar os shaders
    GLuint vertexShader, fragmentShader;
    int success;
    char infoLog[512];

    // Compila o Vertex Shader
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vShaderCode, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERRO::SHADER::VERTEX::COMPILACAO_FALHOU\n" << infoLog << std::endl;
    }

    // Compila o Fragment Shader
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fShaderCode, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERRO::SHADER::FRAGMENT::COMPILACAO_FALHOU\n" << infoLog << std::endl;
    }

    // 3. Linka os shaders num Shader Program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERRO::SHADER::PROGRAMA::LINKAGEM_FALHOU\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}



GLuint shaderProgram;
QuadPtr myQuad;


static void initialize()
{
  // config do OpenGL
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);
  glPolygonMode(GL_FRONT, GL_FILL);

  // definindo shaders
  shaderProgram = createShaderProgram("../shaders/vertex.glsl", "../shaders/fragment.glsl");

  // CENTERPIECE: inicialização dos objetos

  float vertices[] = {
      -0.5f, -0.5f, // Inferior esquerdo
       0.5f, -0.5f, // Inferior direito
       0.5f,  0.5f, // Superior direito
      -0.5f,  0.5f  // Superior esquerdo
  };

  unsigned int indices[] = {
      0, 1, 2, // Primeiro triângulo
      2, 3, 0  // Segundo triângulo
  };

  myQuad = Quad::Make(vertices, indices);

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

static void cleanup() 
{
    glDeleteProgram(shaderProgram);
}

static void display(GLFWwindow * win)
{
 glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // CENTERPIECE: desenho dos objetos
  glUseProgram(shaderProgram);
  myQuad->Draw();
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
        glfwTerminate();
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

    cleanup();

    glfwTerminate();
    return 0;
}