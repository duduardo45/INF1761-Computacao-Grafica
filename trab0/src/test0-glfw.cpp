// Simple example to use glfw

#ifdef _WIN32

#define GLAD_GL_IMPLEMENTATION // Necessary for headeronly version.
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#elif __APPLE__

#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>

#endif

#include <iostream>

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

static void display(GLFWwindow * win)
{
 glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

int main()
{
 glfwSetErrorCallback(error);
 if (glfwInit() != GLFW_TRUE) {
    std::cerr << "Could not initialize GLFW" << std::endl;
    return 0;
  }
 glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
 glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
 glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
 glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);     // required for mac os
#if __APPLE__
 glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);  // option for mac os
#endif
 
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

 std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

 glfwSetFramebufferSizeCallback(win, resize);  // resize callback
 glfwSetKeyCallback(win, keyboard);   // keyboard callback
 glfwSetMouseButtonCallback(win, mousebutton); // mouse button callback

 initialize();
 while (!glfwWindowShouldClose(win)) {
  display(win);
  glfwSwapBuffers(win);
  glfwPollEvents();
 }
 glfwTerminate();
 return 0;
}

