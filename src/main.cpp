#include <iostream>
#include <cstdlib>
#include <cmath>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "shader.hpp"
#include "util.hpp"

//-----------------------------------------------------------------------------
// settings
//-----------------------------------------------------------------------------
unsigned int win_w = 1280;
unsigned int win_h = 720;

float fovY = 90.f;
float zNear = 0.1f;
float zFar = 30.f;

glm::vec3 camPos = glm::vec3(1.2f, 0.75f, 1.f);

#define REQUIRED_OGL_VERSION_MAJOR 3
#define REQUIRED_OGL_VERSION_MINOR 3

//-----------------------------------------------------------------------------
// gui parameters
//-----------------------------------------------------------------------------
bool gui_frame = true;
bool gui_wireframe = false;
float gui_cam_zoom_speed = 0.1f;
float gui_cam_rot_speed = 0.2f;

//-----------------------------------------------------------------------------
// function prototypes
//-----------------------------------------------------------------------------
void cursor_position_cb(GLFWwindow *window, double xpos, double ypos);
void mouse_button_cb(GLFWwindow* window, int button, int action, int mods);
void scroll_cb(GLFWwindow* window, double xoffset, double yoffset);
void key_cb(GLFWwindow* window, int key, int scancode , int action, int mods);
void char_cb(GLFWwindow* window, unsigned int c);
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
    GLFWwindow* window = createWindow(win_w, win_h, "MVR");

    initializeGl3w();
    initializeImGui(window);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.f, 0.f, 0.f, 1.f);

    Shader shaderFrame("src/shader/frame.vert", "src/shader/frame.frag");
    Shader shaderVolume("src/shader/volume.vert", "src/shader/volume.frag");

    // bounding cube geometry
    // ----------------------
    float vertices[] = {
        -0.5f,  -0.5f,  -0.5f,  1.f,
        0.5f,   -0.5f,  -0.5f,  1.f,
        0.5f,   0.5f,   -0.5f,  1.f,
        -0.5f,  0.5f,   -0.5f,  1.f,
        -0.5f,  -0.5f,  0.5f,   1.f,
        0.5f,   -0.5f,  0.5f,   1.f,
        0.5f,   0.5f,   0.5f,   1.f,
        -0.5f,  0.5f,   0.5f,   1.f
    };
    float texCoords[] = {
        0.f, 0.f, 1.f,
        1.f, 0.f, 1.f,
        1.f, 1.f, 1.f,
        0.f, 1.f, 1.f,
        0.f, 0.f, 0.f,
        1.f, 0.f, 0.f,
        1.f, 1.f, 0.f,
        0.f, 1.f, 0.f
    };
    unsigned int volumeIndices[] = {
        2, 1, 0,
        0, 3, 2,
        6, 5, 1,
        1, 2, 6,
        7, 4, 5,
        5, 6, 7,
        3, 0, 4,
        4, 7, 3,
        3, 7, 6,
        6, 2, 3,
        4, 0, 1,
        1, 5, 4
    };
    unsigned int frameIndices[] = {
        0, 1,
        1, 2,
        2, 3,
        3, 0,
        4, 5,
        5, 6,
        6, 7,
        7, 4,
        0, 4,
        1, 5,
        2, 6,
        3, 7
    };

    // ---------------------------
    // create vertex array objects
    // ---------------------------
    GLuint frameVAO, volumeVAO;
    GLuint frameEBO, volumeEBO;
    GLuint VBO[2];

    // frame
    glGenVertexArrays(1, &frameVAO);
    glGenBuffers(2, VBO);
    glGenBuffers(1, &frameEBO);

    glBindVertexArray(frameVAO);

    // vertex coordinates
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(
        0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // vertex indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, frameEBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(frameIndices),
        frameIndices,
        GL_STATIC_DRAW);

    // texture coordinates
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(
        GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    //-------------------------------------------------------------------------
    // volume
    glGenVertexArrays(1, &volumeVAO);
    glGenBuffers(1, &volumeEBO);

    glBindVertexArray(volumeVAO);

    // vertex coordinates
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);  // reuse frame vertex buffer
    glVertexAttribPointer(
        0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // vertex indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, volumeEBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(volumeIndices),
        volumeIndices,
        GL_STATIC_DRAW);

    // texture coordinates
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]); // reuse frame vertex buffer
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // initialize model, view and projection matrices
    // ----------------------------------------------
    glm::mat4 modelMX = glm::scale(
        glm::mat4(1.f),
        glm::vec3(1.f));

    glm::vec3 right = glm::normalize(
        glm::cross(-camPos, glm::vec3(0.f, 1.f, 0.f)));
    glm::vec3 up = glm::normalize(glm::cross(right, -camPos));
    glm::mat4 viewMX = glm::lookAt(camPos, glm::vec3(0.f), up);

    glm::mat4 projMX = glm::perspective(
        glm::radians(fovY),
        static_cast<float>(win_w)/static_cast<float>(win_h),
        0.1f,
        50.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // update model, view and projection matrix
        right = glm::normalize(glm::cross(-camPos, glm::vec3(0.f, 1.f, 0.f)));
        up = glm::normalize(glm::cross(right, -camPos));
        viewMX = glm::lookAt(camPos, glm::vec3(0.f), up);
        projMX = glm::perspective(
            glm::radians(fovY),
            static_cast<float>(win_w)/static_cast<float>(win_h),
            zNear,
            zFar);

        // apply gui settings
        if(gui_wireframe)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // draw the bounding frame
        if(gui_frame)
        {
            shaderFrame.use();
            shaderFrame.setMat4("pvmMX", projMX * viewMX * modelMX);
            glBindVertexArray(frameVAO);
            glDrawElements(GL_LINES, 2*12, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        // draw the volume
        shaderVolume.use();
        shaderVolume.setMat4("modelMX", modelMX);
        shaderVolume.setMat4("viewMX", viewMX);
        shaderVolume.setMat4("projMX", projMX);
        shaderVolume.setMat4("pvMX", projMX * viewMX);
        shaderVolume.setMat4("pvmMX", projMX * viewMX * modelMX);
        glBindVertexArray(volumeVAO);
        glDrawElements(GL_TRIANGLES, 3*2*6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // draw ImGui windows
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            ImGui::InputFloat(
                "camera zoom speed", &gui_cam_zoom_speed, 0.01f, 0.1f, "%.3f");
            ImGui::InputFloat(
                "camera rotation speed",
                &gui_cam_rot_speed,
                0.01f,
                0.1f,
                "%.3f");

            glm::vec3 polar = util::cartesianToPolar<glm::vec3>(camPos);
            ImGui::Text("phi: %.3f", polar.y);
            ImGui::Text("theta: %.3f", polar.z);
            ImGui::Text("radius: %.3f", polar.x);
            ImGui::Text(
                "Camera position: x=%.3f, y=%.3f, z=%.3f",
                camPos.x, camPos.y, camPos.z);


            ImGui::Checkbox("draw frame", &gui_frame); ImGui::SameLine();
            ImGui::Checkbox("Wireframe", &gui_wireframe);

            ImGui::Text(
                "Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
        }
        //bool show_demo_window = true;
        //ImGui::ShowDemoWindow(&show_demo_window);
        glViewport(0, 0, win_w, win_h);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteVertexArrays(1, &frameVAO);
    glDeleteBuffers(2, VBO);    // also used for volume
    glDeleteBuffers(1, &frameEBO);
    glDeleteVertexArrays(1, &volumeVAO);
    glDeleteBuffers(1, &volumeEBO);

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

    // install callbacks
    glfwSetMouseButtonCallback(window, mouse_button_cb);
    glfwSetScrollCallback(window, scroll_cb);
    glfwSetKeyCallback(window, key_cb);
    glfwSetCharCallback(window, char_cb);
    glfwSetCursorPosCallback(window, cursor_position_cb);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_cb);

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

    ImGui_ImplGlfw_InitForOpenGL(window, false);
    ImGui_ImplOpenGL3_Init();

    ImGui::StyleColorsDark();
}

//-----------------------------------------------------------------------------
// GLFW callbacks and input processing
//-----------------------------------------------------------------------------
void cursor_position_cb(GLFWwindow *window, double xpos, double ypos)
{
    static double xpos_old = 0.0;
    static double ypos_old = 0.0;
    double dx, dy;

    dx = xpos - xpos_old; xpos_old = xpos;
    dy = ypos - ypos_old; ypos_old = ypos;

    if (GLFW_PRESS == glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE))
    {
		glm::vec3 polar = util::cartesianToPolar<glm::vec3>(camPos);
        float half_pi = glm::half_pi<float>();

        polar.y += glm::radians(dx) * gui_cam_rot_speed;
        polar.z += glm::radians(dy) * gui_cam_rot_speed;
        if (polar.z <= -0.999f * half_pi)
            polar.z = -0.999f * half_pi;
        else if (polar.z >= 0.999f * half_pi)
            polar.z = 0.999f * half_pi;

		camPos = util::polarToCartesian<glm::vec3>(polar);
    }
}

void mouse_button_cb(GLFWwindow* window, int button, int action, int mods)
{
    // chain ImGui callback
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
}

void scroll_cb(GLFWwindow *window, double xoffset, double yoffset)

{
    // y scrolling changes the distance of the camera from the origin
    camPos += static_cast<float>(yoffset) * gui_cam_zoom_speed * camPos;

    // chain ImGui callback
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
}

void key_cb(GLFWwindow* window, int key, int scancode , int action, int mods)
{
    if((key == GLFW_KEY_ESCAPE) && (action == GLFW_PRESS))
        glfwSetWindowShouldClose(window, true);

    // chain ImGui callback
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
}

void char_cb(GLFWwindow* window, unsigned int c)
{
    // chain ImGui callback
    ImGui_ImplGlfw_CharCallback(window, c);
}

void framebuffer_size_cb(
    __attribute__((unused)) GLFWwindow* window,
    int width,
    int height)
{
    win_w = width;
    win_h = height;
}

void error_cb(int error, const char* description)
{
    std::cerr << "Glfw error " << error << ": " << description << std::endl;
}

