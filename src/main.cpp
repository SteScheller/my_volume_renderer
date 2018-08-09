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
#include "configraw.hpp"
#include "texture.hpp"


//-----------------------------------------------------------------------------
// type definitions
//-----------------------------------------------------------------------------
enum class Mode : int
{
    line_of_sight = 0,
    maximum_intensity_projection,
    isosurface,
    transfer_function
};

//-----------------------------------------------------------------------------
// settings
//-----------------------------------------------------------------------------
unsigned int win_w = 1280;
unsigned int win_h = 720;

float fovY = 90.f;
float zNear = 0.000001f;
float zFar = 30.f;

glm::vec3 camPos = glm::vec3(1.2f, 0.75f, 1.f);

#define REQUIRED_OGL_VERSION_MAJOR 3
#define REQUIRED_OGL_VERSION_MINOR 3

//-----------------------------------------------------------------------------
// gui parameters
//-----------------------------------------------------------------------------
int gui_mode = static_cast<int>(Mode::line_of_sight);

float gui_step_size = 0.01f;

float gui_brightness = 1.f;

float gui_cam_zoom_speed = 0.1f;
float gui_cam_rot_speed = 0.2f;

float gui_isovalue = 0.1f;

float gui_light_dir[3] = {0.3f, 1.f, -0.3f};
float gui_ambient[3] = {0.2f, 0.2f, 0.2f};
float gui_diffuse[3] = {1.f, 1.f, 1.f};
float gui_specular[3] = {1.f, 1.f, 1.f};
float gui_k_amb = 0.2f;
float gui_k_diff = 0.3f;
float gui_k_spec = 0.5f;
float gui_k_exp = 10.0f;


bool gui_show_histogram_window = true;
bool gui_hist_semilog = false;
int gui_num_bins = 255;
int gui_y_limit = 10000;

bool gui_show_tf_window = true;

bool gui_show_demo_window = false;
bool gui_frame = true;
bool gui_wireframe = false;
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
static void ShowHelpMarker(const char* desc);
static void showSettingsWindow();
static void showHistogramWindow(util::bin_t bins[], size_t num_bins);
static void showTransferFunctionWindow();
//-----------------------------------------------------------------------------
// internals
//-----------------------------------------------------------------------------
static bool _flag_reload_shaders = false;

//-----------------------------------------------------------------------------
// main program
//-----------------------------------------------------------------------------
int main(
    __attribute__((unused)) int argc,
    __attribute__((unused)) char *argv[])
{
    GLFWwindow* window = createWindow(win_w, win_h, "MVR");

    initializeGl3w();
    initializeImGui(window);

    glEnable(GL_DEPTH_TEST);
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
    glm::vec4 bbMin = glm::vec4(
            vertices[0], vertices[1],vertices[2], vertices[3]);
    glm::vec4 bbMax = glm::vec4(
            vertices[24], vertices[25],vertices[26], vertices[27]);

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

    // TODO: Actually read and parse the config file
    /*unsigned char *volumeData = new unsigned char[480 * 720 * 120];
    cr::loadRaw<unsigned char>(
        "/mnt/data/steffen/jet/jet_00042",
        volumeData,
        static_cast<size_t>(480 * 720 * 120),
        false);
    modelMX = glm::scale(glm::mat4(1.f), glm::vec3(2.f/3.f, 1.f, 1.f/6.f));
    GLuint volumeTex = util::create3dTexFromScalar(
        volumeData, GL_BYTE, 480, 720, 120);
    util::bin_t histogram_bins = util::binData(
        255,
        static_cast<unsigned char> -127,
        static_cast<unsigned char> 127,
        volumeData,
        static_cast<size_t>(480 * 720 * 120));*/
    unsigned char *volumeData = new unsigned char[256 * 256 * 128];
    cr::loadRaw<unsigned char>(
        "/mnt/data/testData/engine.raw",
        volumeData,
        static_cast<size_t>(256 * 256 * 128),
        false);
    modelMX = glm::scale(glm::mat4(1.f), glm::vec3(1.f, 2.f, 1.f));
    GLuint volumeTex = util::create3dTexFromScalar(
        volumeData, GL_UNSIGNED_BYTE, 256, 256, 128);
    util::bin_t *histogram_bins = util::binData(
        255,
        static_cast<unsigned char>(0),
        static_cast<unsigned char> (255),
        volumeData,
        static_cast<size_t>(256 * 256 * 128));

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // check if shader programs shall be reloaded
        if (_flag_reload_shaders)
        {
            _flag_reload_shaders = false;
            std::cout << "reloading shaders..." << std::endl;

            glDeleteProgram(shaderVolume.ID);
            glDeleteProgram(shaderFrame.ID);

            shaderFrame = Shader(
                "src/shader/frame.vert", "src/shader/frame.frag");
            shaderVolume = Shader(
                "src/shader/volume.vert", "src/shader/volume.frag");
        }

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

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, volumeTex);
        shaderVolume.setInt("volumeTex", 0);
        shaderVolume.setMat4("modelMX", modelMX);
        shaderVolume.setMat4("pvmMX", projMX * viewMX * modelMX);
        shaderVolume.setVec3("eyePos", camPos);
        shaderVolume.setVec3("bbMin", (modelMX * bbMin).xyz);
        shaderVolume.setVec3("bbMax", (modelMX * bbMax).xyz);
        shaderVolume.setInt("mode", gui_mode);
        shaderVolume.setFloat("step_size", gui_step_size);
        shaderVolume.setFloat("brightness", gui_brightness);
        shaderVolume.setFloat("isovalue", gui_isovalue);
        shaderVolume.setVec3(
            "lightDir", gui_light_dir[0], gui_light_dir[1], gui_light_dir[2]);
        shaderVolume.setVec3(
            "ambient", gui_ambient[0], gui_ambient[1], gui_ambient[2]);
        shaderVolume.setVec3(
            "diffuse", gui_diffuse[0], gui_diffuse[1], gui_diffuse[2]);
        shaderVolume.setVec3(
            "specular", gui_specular[0], gui_specular[1], gui_specular[2]);
        shaderVolume.setFloat("k_amb", gui_k_amb);
        shaderVolume.setFloat("k_diff", gui_k_diff);
        shaderVolume.setFloat("k_spec", gui_k_spec);
        shaderVolume.setFloat("k_exp", gui_k_exp);

        glBindVertexArray(volumeVAO);
        glDrawElements(GL_TRIANGLES, 3*2*6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // draw ImGui windows
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        showSettingsWindow();
        if(gui_show_demo_window) ImGui::ShowDemoWindow(&gui_show_demo_window);
        if(gui_mode == static_cast<int>(Mode::transfer_function))
        {
            if(gui_show_tf_window) showTransferFunctionWindow();
            if(gui_show_histogram_window)
                showHistogramWindow(histogram_bins, 255);
        }

        glViewport(0, 0, win_w, win_h);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteTextures(1, &volumeTex);

    glDeleteProgram(shaderVolume.ID);
    glDeleteProgram(shaderFrame.ID);

    glDeleteVertexArrays(1, &frameVAO);
    glDeleteBuffers(2, VBO);    // also used for volume
    glDeleteBuffers(1, &frameEBO);
    glDeleteVertexArrays(1, &volumeVAO);
    glDeleteBuffers(1, &volumeEBO);

    glfwDestroyWindow(window);
    glfwTerminate();

    delete[] volumeData;
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
// from imgui_demo.cpp
static void ShowHelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

static void showSettingsWindow()
{
    ImGui::Begin("Settings");
    {
        ImGui::Text("Mode");
        ImGui::RadioButton(
            "line of sight",
            &gui_mode,
            static_cast<int>(Mode::line_of_sight));
        ImGui::RadioButton(
            "maximum intensity projection",
            &gui_mode,
            static_cast<int>(Mode::maximum_intensity_projection));
        ImGui::RadioButton(
            "isosurface",
            &gui_mode,
            static_cast<int>(Mode::isosurface));
        ImGui::RadioButton(
            "transfer function",
            &gui_mode,
            static_cast<int>(Mode::transfer_function));

        ImGui::Spacing();

        ImGui::InputFloat(
            "step size", &gui_step_size, 0.0001f, 0.01f, "%.4f");

        ImGui::Spacing();

        ImGui::InputFloat("brightness", &gui_brightness, 0.01f, 0.1f);

        if (ImGui::CollapsingHeader("General"))
        {
            ImGui::Checkbox("draw frame", &gui_frame); ImGui::SameLine();
            ImGui::Checkbox("wireframe", &gui_wireframe);
            ImGui::Checkbox(
                "show ImGui demo window", &gui_show_demo_window);
        }

        if (ImGui::CollapsingHeader("Camera"))
        {
            ImGui::InputFloat(
                "camera zoom speed",
                &gui_cam_zoom_speed,
                0.01f,
                0.1f,
                "%.3f");
            ImGui::SameLine();
            ShowHelpMarker(
                "Scroll up or down while holding CTRL to zoom.");
            ImGui::InputFloat(
                "camera rotation speed",
                &gui_cam_rot_speed,
                0.01f,
                0.1f,
                "%.3f");
            ImGui::SameLine();
            ShowHelpMarker(
                "Hold the middle mouse button and move the mouse to pan "
                "the camera");
           glm::vec3 polar = util::cartesianToPolar<glm::vec3>(camPos);
            ImGui::Text("phi: %.3f", polar.y);
            ImGui::Text("theta: %.3f", polar.z);
            ImGui::Text("radius: %.3f", polar.x);
            ImGui::Text(
                "Camera position: x=%.3f, y=%.3f, z=%.3f",
                camPos.x, camPos.y, camPos.z);

        }

        if (ImGui::CollapsingHeader("Isosurface"))
        {
            ImGui::InputFloat(
                "isovalue",
                &gui_isovalue,
                0.01f,
                0.1f,
                "%.3f");
            if (ImGui::TreeNode("Lighting"))
            {
                ImGui::SliderFloat3(
                    "light direction", gui_light_dir, 0.f, 1.f);
                ImGui::ColorEdit3("ambient", gui_ambient);
                ImGui::ColorEdit3("diffuse", gui_diffuse);
                ImGui::ColorEdit3("specular", gui_specular);

                ImGui::Spacing();

                ImGui::SliderFloat("k_amb", &gui_k_amb, 0.f, 1.f);
                ImGui::SliderFloat("k_diff", &gui_k_diff, 0.f, 1.f);
                ImGui::SliderFloat("k_spec", &gui_k_spec, 0.f, 1.f);
                ImGui::SliderFloat("k_exp", &gui_k_spec, 0.f, 50.f);
                ImGui::TreePop();
            }
        }

        if (ImGui::CollapsingHeader("Transfer Function"))
        {
            ImGui::Checkbox("show histogram", &gui_show_histogram_window);
            ImGui::SameLine();
            ShowHelpMarker(
                "Only visible in transfer function mode.");

            ImGui::Spacing();

            ImGui::Checkbox(
                "show transfer function editor", &gui_show_tf_window);
            ImGui::SameLine();
            ShowHelpMarker(
                "Only visible in transfer function mode.");
        }

        ImGui::Separator();

        ImGui::Text(
            "Application average %.3f ms/frame (%.1f FPS)",
            1000.0f / ImGui::GetIO().Framerate,
            ImGui::GetIO().Framerate);
    }
    ImGui::End();
}

static void showHistogramWindow(util::bin_t bins[], size_t num_bins)
{
    float values[num_bins];

    for(size_t i = 0; i < num_bins; i++)
    {
        if (gui_hist_semilog)
            values[i] = log10(std::get<2>(bins[i]));
        else
            values[i] = static_cast<float>(std::get<2>(bins[i]));
    }

    ImGui::Begin("Histogram", &gui_show_histogram_window);
    ImGui::PushItemWidth(-1);
    ImGui::PlotHistogram(
        "",
        values,
        num_bins,
        0,
        nullptr,
        0.f,
        gui_hist_semilog ? static_cast<int>(log10(gui_y_limit)) : gui_y_limit,
        ImVec2(0, 160));
    ImGui::PopItemWidth();
    ImGui::InputInt("y limit", &gui_y_limit, 1, 100);
    ImGui::SameLine();
    ImGui::Checkbox("semi-logarithmic", &gui_hist_semilog);
    ImGui::InputInt("# bins", &gui_num_bins);
    ImGui::SameLine();
    if(ImGui::Button("Regenerate Histogram"))
    {
        //TODO: Regenerate Histrogram
    }
    ImGui::End();
}

static void showTransferFunctionWindow()
{
    ImGui::Begin("Transfer Function Editor", &gui_show_tf_window);
    ImGui::Text("Here comes the transfer function editor");
    ImGui::End();
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

    if ((GLFW_PRESS == glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)) ||
        (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL)))
    {
        // y scrolling changes the distance of the camera from the origin
        camPos +=
            static_cast<float>(-yoffset) *
            gui_cam_zoom_speed *
            camPos;
    }

    // chain ImGui callback
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
}

void key_cb(GLFWwindow* window, int key, int scancode , int action, int mods)
{
    if((key == GLFW_KEY_ESCAPE) && (action == GLFW_PRESS))
        glfwSetWindowShouldClose(window, true);

    if((key == GLFW_KEY_R) && (action == GLFW_PRESS))
        _flag_reload_shaders = true;

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

