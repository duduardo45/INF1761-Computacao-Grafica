#define GLAD_GL_IMPLEMENTATION // Coloque isto no seu .cpp antes de incluir glad.h
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <vector>

#include "quad.h"

// --- Shaders básicos como strings ---
// Para um esqueleto, é mais fácil ter os shaders aqui do que em ficheiros separados.

// Vertex Shader: Responsável por calcular a posição final de cada vértice.
const char* vertexShaderSource = R"(
    #version 410 core
    layout (location = 0) in vec2 aPos;
    void main()
    {
        gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    }
)";

// Fragment Shader: Responsável por definir a cor de cada píxel.
const char* fragmentShaderSource = R"(
    #version 410 core
    out vec4 FragColor;
    void main()
    {
        FragColor = vec4(0.2f, 0.3f, 0.8f, 1.0f); // Cor azul
    }
)";


// --- Função para compilar e linkar os shaders ---
GLuint createShaderProgram() {
    // Compila o Vertex Shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // Verifica erros de compilação
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERRO::SHADER::VERTEX::COMPILACAO_FALHOU\n" << infoLog << std::endl;
    }

    // Compila o Fragment Shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // Verifica erros de compilação
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERRO::SHADER::FRAGMENT::COMPILACAO_FALHOU\n" << infoLog << std::endl;
    }

    // Linka os shaders num Shader Program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // Verifica erros de linkagem
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERRO::SHADER::PROGRAMA::LINKAGEM_FALHOU\n" << infoLog << std::endl;
    }

    // Os shaders já estão no programa, podemos deletá-los
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}


// Variáveis globais para o nosso programa
GLuint shaderProgram;


// Função de inicialização
static void initialize()
{
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Cor de fundo cinza escuro
    glEnable(GL_DEPTH_TEST);

    // --- CENTERPIECE: INICIALIZAÇÃO ---
    // Crie o seu shader program
    shaderProgram = createShaderProgram();
    
    // Crie e inicialize os seus objetos (ex: myQuad = Quad::Make();) aqui
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

static void mousebutton(GLFWwindow* window, int button, int action, int mods)
{
    // Callback para eventos de mouse (atualmente vazia)
}

static void resize(GLFWwindow * win, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Função de display (renderização)
static void display(GLFWwindow * win)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // --- CENTERPIECE: DESENHO ---
    // Ative o seu shader program
    glUseProgram(shaderProgram);
    
    // Chame as funções de desenho dos seus objetos (ex: myQuad->Draw();) aqui
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
    QuadPtr myQuad = Quad::Make(vertices, indices);

    myQuad->Draw();
}

// Função para limpar os recursos da OpenGL
static void cleanup() {
    glDeleteProgram(shaderProgram);
    // Os seus objetos (como shared_ptrs) serão limpos automaticamente
}

int main(void) {
    glfwSetErrorCallback(error);
    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "Could not initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    GLFWwindow* win = glfwCreateWindow(800, 600, "Esqueleto OpenGL", nullptr, nullptr);
    if (!win) {
        std::cerr << "Could not create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(win);

    // Carrega o GLAD
    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        printf("Failed to initialize GLAD OpenGL context\n");
        return -1;
    }
    printf("Loaded OpenGL %d.%d\n", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));


    glfwSetFramebufferSizeCallback(win, resize);
    glfwSetKeyCallback(win, keyboard);
    glfwSetMouseButtonCallback(win, mousebutton);

    // Chama a nossa função de inicialização
    initialize();

    // Loop principal de renderização
    while (!glfwWindowShouldClose(win)) {
        display(win);
        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    // Limpa os recursos antes de sair
    cleanup();

    glfwTerminate();
    return 0;
}
