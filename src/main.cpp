#include <iostream>
#include <cstdlib>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "shader.h"

//-----------------------------------------------------------------------------
// settings
//-----------------------------------------------------------------------------
unsigned int win_w = 1280;
unsigned int win_h = 720;

float fovY = 90.f;
float zNear = 0.1f;
float zFar = 30.f;

#define REQUIRED_OGL_VERSION_MAJOR 3
#define REQUIRED_OGL_VERSION_MINOR 3

//-----------------------------------------------------------------------------
// function prototypes
//-----------------------------------------------------------------------------
void process_input(GLFWwindow *window);
void framebuffer_size_cb(GLFWwindow* window, int width, int height);
void error_cb(int error, const char* description);
GLFWwindow* createWindow(
    unsigned int win_w, unsigned int win_h, const char* title);
void initializeGl3w();
void initializeImGui(GLFWwindow* window);

//-----------------------------------------------------------------------------
// main program
//-----------------------------------------------------------------------------
int main(int, char**)
{
    GLFWwindow* window = createWindow( win_w, win_h, "Hello world!");

    initializeGl3w();
    initializeImGui(window);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.f, 0.f, 0.f, 1.f);

    Shader shaderProg("src/shader/simple.vert", "src/shader/simple.frag");

    // geometry
    // --------
    float vertices[] = {
        0.f,    0.5f,   0.f,    1.f,
        -0.5f,  -0.5f,  0.f,    1.f,
        0.5f,   -0.5f,  0.f,    1.f
    };
    float texCoords[] = {
        1.f, 0.f, 0.f,
        0.f, 1.f, 0.f,
        0.f, 0.f, 1.f
    };
    unsigned int indices[] = {
        0, 1, 2
    };

    unsigned int VBO[2], VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(2, VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);


    // vertex coordinates
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // vertex indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // texture coordinates
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // render loop
    // -----------
    bool show_demo_window = false;

    glm::mat4 modelMX = glm::scale(
        glm::mat4(1.f),
        glm::vec3(1.f));

    glm::mat4 viewMX = glm::lookAt(
        glm::vec3(0.f, 0.f, 1.f),
        glm::vec3(0.f),
        glm::vec3(0.f, 1.f, 0.f));

    glm::mat4 projMX = glm::perspective(
        glm::radians(fovY),
        static_cast<float>(win_w)/static_cast<float>(win_h),
        0.1f,
        50.0f);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        process_input(window);

        glClear(GL_COLOR_BUFFER_BIT);

        // draw the triangle
        projMX = glm::perspective(
            glm::radians(fovY),
            static_cast<float>(win_w)/static_cast<float>(win_h),
            zNear,
            zFar);
        shaderProg.use();
        shaderProg.setMat4("modelMX", modelMX);
        shaderProg.setMat4("viewMX", viewMX);
        shaderProg.setMat4("projMX", projMX);
        shaderProg.setMat4("pvMX", projMX * viewMX);
        shaderProg.setMat4("pvmMX", projMX * viewMX * modelMX);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // draw ImGui windows
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            ImGui::Text("Hello, world!");
            ImGui::Checkbox("Demo Window", &show_demo_window);


            ImGui::Text(
                "Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
        }
        if (show_demo_window)
        {
            ImGui::ShowDemoWindow(&show_demo_window);
        }
        glViewport(0, 0, win_w, win_h);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

//-----------------------------------------------------------------------------
// subroutines
//-----------------------------------------------------------------------------
GLFWwindow* createWindow(
    unsigned int win_w, unsigned int win_h, const char* title)
{
    glfwSetErrorCallback(error_cb);
    if (!glfwInit()) exit(EXIT_FAILURE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(
        win_w, win_h, title, nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    gl3wInit();

    return window;
}

void initializeGl3w()
{
    if (gl3wInit())
    {
        std::cerr << "Failed to initialize OpenGL" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    if (!gl3wIsSupported(
            REQUIRED_OGL_VERSION_MAJOR, REQUIRED_OGL_VERSION_MINOR))
    {
        std::cerr << "OpenGL " << REQUIRED_OGL_VERSION_MAJOR << "." <<
            REQUIRED_OGL_VERSION_MINOR << " not supported" << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << "OpenGL " << glGetString(GL_VERSION) << ", GLSL " <<
        glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
}

void initializeImGui(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    ImGui::StyleColorsDark();
}

//-----------------------------------------------------------------------------
// GLFW callbacks and input processing
//-----------------------------------------------------------------------------
void process_input(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_cb(
    __attribute__((unused)) GLFWwindow* window,
    int width,
    int height)
{
    win_w = width;
    win_h = height;
    std::cout << win_w << win_w;
}

void error_cb(int error, const char* description)
{
    std::cerr << "Glfw error " << error << ": " << description << std::endl;
}

