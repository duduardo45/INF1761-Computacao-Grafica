#define GLAD_GL_IMPLEMENTATION // Coloque isto no seu .cpp antes de incluir glad.h
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>



// Função para ler o conteúdo de um ficheiro de shader
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


// --- Função para compilar e linkar os shaders ---
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

// Variáveis globais para o nosso programa
GLuint shaderProgram;


// Função de inicialização
static void initialize()
{
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Cor de fundo cinza escuro
    glEnable(GL_DEPTH_TEST);

    // --- CENTERPIECE: INICIALIZAÇÃO ---
    // Crie o seu shader program
    shaderProgram = createShaderProgram("shaders/vertex.glsl", "shaders/fragment.glsl");
    
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
