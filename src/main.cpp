#include <iostream>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <vector>
#include <string>
#include <exception>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

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
enum class Gradient : int
{
    central_differences = 0,
    sobel_operators
};

//-----------------------------------------------------------------------------
// global parameters
//-----------------------------------------------------------------------------
unsigned int win_w = 1600;
unsigned int win_h = 900;

float fovY = 90.f;
float zNear = 0.000001f;
float zFar = 30.f;

glm::vec3 camPos = glm::vec3(1.2f, 0.75f, 1.f);

#define REQUIRED_OGL_VERSION_MAJOR 3
#define REQUIRED_OGL_VERSION_MINOR 3

#define MAX_FILEPATH_LENGTH 200

#define DEFAULT_VOLUME_JSON_FILE "/mnt/data/steffen/jet.json"

//-----------------------------------------------------------------------------
// gui parameters
//-----------------------------------------------------------------------------
int gui_mode = static_cast<int>(Mode::line_of_sight);

char gui_volume_desc_file[MAX_FILEPATH_LENGTH]; // init from program options

int gui_timestep = 0;

float gui_step_size = 0.25f;

int gui_grad_method = static_cast<int>(Gradient::sobel_operators);

float gui_brightness = 1.f;

float gui_cam_zoom_speed = 0.1f;
float gui_cam_rot_speed = 0.2f;

float gui_isovalue = 0.1f;
bool gui_iso_denoise = true;
float gui_iso_denoise_r = 0.1f;

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
int gui_y_limit = 100000;
float gui_x_min = 0.f;
float gui_x_max = 255.f;

bool gui_show_tf_window = true;

bool gui_show_demo_window = false;
bool gui_frame = true;
bool gui_wireframe = false;
bool gui_invert_colors = false;
bool gui_invert_alpha = false;

bool gui_slice_volume = false;
float gui_slice_plane_normal[3] = {0.f, 0.f, 1.f};
float gui_slice_plane_base[3] = {0.f, 0.f, 0.f};

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
void applyProgramOptions(int argc, char *argv[]);
void initializeGl3w();
void initializeImGui(GLFWwindow* window);
static void showHelpMarker(const char* desc);
static void showSettingsWindow(
    cr::VolumeConfig &vConf,
    void *&volumeData,
    GLuint &volumeTex,
    glm::mat4 &modelMX,
    std::vector<util::bin_t> *&histogramBins);
static void showHistogramWindow(
    std::vector<util::bin_t> *&histogramBins,
    cr::VolumeConfig vConf,
    void* volumeData);
static void showTransferFunctionWindow();
std::vector<util::bin_t> *loadHistogramBins(
    cr::VolumeConfig vConf, void* values, size_t numBins, float min, float max);
GLuint loadScalarVolumeTex(cr::VolumeConfig vConf, void* volumeData);
static void reloadStuff(
    cr::VolumeConfig vConf,
    void *&volumeData,
    GLuint &volumeTex,
    unsigned int timestep,
    glm::mat4 &modelMX,
    std::vector<util::bin_t> *&histogramBins,
    size_t num_bins,
    float x_min,
    float x_max);
//-----------------------------------------------------------------------------
// internals
//-----------------------------------------------------------------------------
static bool _flag_reload_shaders = false;

//-----------------------------------------------------------------------------
// main program
//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    applyProgramOptions(argc, argv);

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
    glm::mat4 modelMX = glm::mat4(1.f);

    glm::vec3 right = glm::normalize(
        glm::cross(-camPos, glm::vec3(0.f, 1.f, 0.f)));
    glm::vec3 up = glm::normalize(glm::cross(right, -camPos));
    glm::mat4 viewMX = glm::lookAt(camPos, glm::vec3(0.f), up);

    glm::mat4 projMX = glm::perspective(
        glm::radians(fovY),
        static_cast<float>(win_w)/static_cast<float>(win_h),
        0.1f,
        50.0f);

    // Diagonal of a voxel
    float voxel_diag = 1.f;

    // load the data, create a model matrix, bins and a texture
    std::vector<util::bin_t> *histogramBins = nullptr;
    void *volumeData = nullptr;
    GLuint volumeTex = 0;

    cr::VolumeConfig vConf = cr::VolumeConfig(gui_volume_desc_file);
    if(vConf.isValid())
    {
        reloadStuff(
            vConf,
            volumeData,
            volumeTex,
            gui_timestep,
            modelMX,
            histogramBins,
            gui_num_bins,
            gui_x_min,
            gui_x_max);
    }
    else
    {
        modelMX = glm::mat4(1.f);
        histogramBins = nullptr;
        volumeData = nullptr;
        volumeTex = 0;
    }

    // temporary variables
    glm::vec3 tempVec3 = glm::vec3(0.f);

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

        // calculate the diagonal of a voxel
        voxel_diag = glm::length(glm::vec3(
                (modelMX *
                    glm::vec4(
                        1.f / static_cast<float>(vConf.getVolumeDim()[0]),
                        1.f / static_cast<float>(vConf.getVolumeDim()[1]),
                        1.f / static_cast<float>(vConf.getVolumeDim()[2]),
                        1.f)).xyz));

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
        shaderVolume.setInt("gradMethod", gui_grad_method);
        shaderVolume.setFloat("step_size", voxel_diag * gui_step_size);
        shaderVolume.setFloat("brightness", gui_brightness);
        shaderVolume.setFloat("isovalue", gui_isovalue);
        shaderVolume.setBool("iso_denoise", gui_iso_denoise);
        shaderVolume.setFloat("iso_denoise_r", voxel_diag * gui_iso_denoise_r);
        tempVec3 = glm::normalize(
                glm::vec3(
                    gui_light_dir[0], gui_light_dir[1], gui_light_dir[2]));
        shaderVolume.setVec3(
            "lightDir", tempVec3[0], tempVec3[1], tempVec3[2]);
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
        shaderVolume.setBool("invert_colors", gui_invert_colors);
        shaderVolume.setBool("invert_alpha", gui_invert_alpha);
        shaderVolume.setBool("slice_volume", gui_slice_volume);
        tempVec3 = glm::normalize(
                glm::vec3(
                    gui_slice_plane_normal[0],
                    gui_slice_plane_normal[1],
                    gui_slice_plane_normal[2]));
        shaderVolume.setVec3(
            "slice_plane_normal", tempVec3[0], tempVec3[1], tempVec3[2]);
        tempVec3 = (modelMX *
                glm::vec4(
                    gui_slice_plane_base[0] / 2.f,
                    gui_slice_plane_base[1] / 2.f,
                    gui_slice_plane_base[2] / 2.f,
                    1.f)).xyz;
        shaderVolume.setVec3(
            "slice_plane_base", tempVec3[0], tempVec3[1], tempVec3[2]);

        glBindVertexArray(volumeVAO);
        glDrawElements(GL_TRIANGLES, 3*2*6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // draw ImGui windows
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        showSettingsWindow(
            vConf,
            volumeData,
            volumeTex,
            modelMX,
            histogramBins);
        if(gui_show_demo_window) ImGui::ShowDemoWindow(&gui_show_demo_window);
        if(gui_mode == static_cast<int>(Mode::transfer_function))
        {
            if(gui_show_tf_window) showTransferFunctionWindow();
            if(gui_show_histogram_window)
                showHistogramWindow(
                    histogramBins,
                    vConf,
                    volumeData);
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

    if (volumeData != nullptr) cr::deleteVolumeData(vConf, volumeData);
    if (histogramBins != nullptr) delete histogramBins;

    return 0;
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

    if((key == GLFW_KEY_F5) && (action == GLFW_PRESS))
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

//-----------------------------------------------------------------------------
// subroutines
//-----------------------------------------------------------------------------
void applyProgramOptions(int argc, char *argv[])
{
    // Declare the supporded options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("volume", po::value<std::string>(), "volume description file")
    ;

    try
    {
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            std::cout << desc << std::endl;
            exit(EXIT_SUCCESS);
        }

        std::string desc_file = "";
        if (vm.count("volume"))
            desc_file = vm["volume"].as<std::string>();
        else
            desc_file = DEFAULT_VOLUME_JSON_FILE;

        cr::VolumeConfig tempConf = cr::VolumeConfig(desc_file);
        if(tempConf.isValid())
            strncpy(
                gui_volume_desc_file, desc_file.c_str(), MAX_FILEPATH_LENGTH);
        else
        {
            std::cout << "Invalid volume description!" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    catch(std::exception &e)
    {
        std::cout << "Invalid program options!" << std::endl;
        exit(EXIT_FAILURE);
    }

}

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
static void showHelpMarker(const char* desc)
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

static void showSettingsWindow(
    cr::VolumeConfig &vConf,
    void *&volumeData,
    GLuint &volumeTex,
    glm::mat4 &modelMX,
    std::vector<util::bin_t> *&histogramBins)
{
    cr::VolumeConfig tempConf;

    ImGui::Begin("Settings");
    {
        if(ImGui::InputText(
                "volume",
                gui_volume_desc_file,
                MAX_FILEPATH_LENGTH,
                ImGuiInputTextFlags_CharsNoBlank |
                    ImGuiInputTextFlags_EnterReturnsTrue))
        {
            tempConf = cr::VolumeConfig(gui_volume_desc_file);
            if(tempConf.isValid())
            {
                vConf = tempConf;
                reloadStuff(
                    vConf,
                    volumeData,
                    volumeTex,
                    gui_timestep,
                    modelMX,
                    histogramBins,
                    gui_num_bins,
                    gui_x_min,
                    gui_x_max);
            }
        }
        ImGui::SameLine();
        showHelpMarker("Path to the volume description file");
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

        if(ImGui::InputInt(
            "timestep",
            &gui_timestep,
            1,
            1,
            ImGuiInputTextFlags_CharsNoBlank |
                ImGuiInputTextFlags_EnterReturnsTrue))
        {
            if (gui_timestep < 0) gui_timestep = 0;
            else if(gui_timestep >
                    static_cast<int>(vConf.getNumTimesteps() - 1))
                gui_timestep = vConf.getNumTimesteps() - 1;

            reloadStuff(
                vConf,
                volumeData,
                volumeTex,
                gui_timestep,
                modelMX,
                histogramBins,
                gui_num_bins,
                gui_x_min,
                gui_x_max);
        }

        ImGui::Spacing();

        ImGui::SliderFloat(
            "step size", &gui_step_size, 0.05f, 2.f, "%.3f");

        ImGui::Spacing();

        ImGui::InputFloat("brightness", &gui_brightness, 0.01f, 0.1f);

        ImGui::Spacing();

        ImGui::Text("Gradient Calculation Method:");
        ImGui::RadioButton(
            "central differences",
            &gui_grad_method,
            static_cast<int>(Gradient::central_differences));
        ImGui::RadioButton(
            "sobel operators",
            &gui_grad_method,
            static_cast<int>(Gradient::sobel_operators));

        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Isosurface"))
        {
            ImGui::InputFloat(
                "isovalue",
                &gui_isovalue,
                0.01f,
                0.1f,
                "%.3f");
            ImGui::Checkbox("denoise", &gui_iso_denoise);
            ImGui::SliderFloat(
                "denoise radius", &gui_iso_denoise_r, 0.001f, 5.f, "%.3f");
            if (ImGui::TreeNode("Lighting"))
            {
                ImGui::SliderFloat3(
                    "light direction", gui_light_dir, -1.f, 1.f);
                ImGui::ColorEdit3("ambient", gui_ambient);
                ImGui::ColorEdit3("diffuse", gui_diffuse);
                ImGui::ColorEdit3("specular", gui_specular);

                ImGui::Spacing();

                ImGui::SliderFloat("k_amb", &gui_k_amb, 0.f, 1.f);
                ImGui::SliderFloat("k_diff", &gui_k_diff, 0.f, 1.f);
                ImGui::SliderFloat("k_spec", &gui_k_spec, 0.f, 1.f);
                ImGui::SliderFloat("k_exp", &gui_k_exp, 0.f, 50.f);
                ImGui::TreePop();
            }
        }

        if (ImGui::CollapsingHeader("Transfer Function"))
        {
            ImGui::Checkbox("show histogram", &gui_show_histogram_window);
            ImGui::SameLine();
            showHelpMarker(
                "Only visible in transfer function mode.");

            ImGui::Spacing();

            ImGui::Checkbox(
                "show transfer function editor", &gui_show_tf_window);
            ImGui::SameLine();
            showHelpMarker(
                "Only visible in transfer function mode.");
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
            showHelpMarker(
                "Scroll up or down while holding CTRL to zoom.");
            ImGui::InputFloat(
                "camera rotation speed",
                &gui_cam_rot_speed,
                0.01f,
                0.1f,
                "%.3f");
            ImGui::SameLine();
            showHelpMarker(
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

        if (ImGui::CollapsingHeader("General"))
        {
            ImGui::Checkbox("draw frame", &gui_frame); ImGui::SameLine();
            ImGui::Checkbox("wireframe", &gui_wireframe);
            ImGui::Checkbox(
                "show ImGui demo window", &gui_show_demo_window);
            ImGui::Checkbox("invert colors", &gui_invert_colors);
            ImGui::Checkbox("invert alpha", &gui_invert_alpha);

            ImGui::Spacing();

            ImGui::Checkbox("Slice volume", &gui_slice_volume);
            ImGui::SliderFloat3(
                "slicing plane normal", gui_slice_plane_normal, -1.f, 1.f);
            ImGui::SliderFloat3(
                "slicing plane base", gui_slice_plane_base, -1.f, 1.f);
        }

        ImGui::Separator();

        ImGui::Text(
            "Application average %.3f ms/frame (%.1f FPS)",
            1000.0f / ImGui::GetIO().Framerate,
            ImGui::GetIO().Framerate);
    }
    ImGui::End();
}

static void showHistogramWindow(
    std::vector<util::bin_t> *&histogramBins,
    cr::VolumeConfig vConf,
    void* volumeData)
{
    float values[(*histogramBins).size()];

    for(size_t i = 0; i < (*histogramBins).size(); i++)
    {
        if (gui_hist_semilog)
            values[i] = log10(std::get<2>((*histogramBins)[i]));
        else
            values[i] = static_cast<float>(std::get<2>((*histogramBins)[i]));
    }

    ImGui::Begin("Histogram", &gui_show_histogram_window);
    ImGui::PushItemWidth(-1);
    ImGui::PlotHistogram(
        "",
        values,
        (*histogramBins).size(),
        0,
        nullptr,
        0.f,
        gui_hist_semilog ? static_cast<int>(log10(gui_y_limit)) : gui_y_limit,
        ImVec2(0, 160));
    ImGui::PopItemWidth();
    ImGui::InputInt("y limit", &gui_y_limit, 1, 100);
    ImGui::SameLine();
    ImGui::Checkbox("semi-logarithmic", &gui_hist_semilog);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::InputInt("# bins", &gui_num_bins);
    ImGui::InputFloat("x min", &gui_x_min, 0.1f, 1.f);
    ImGui::InputFloat("x max", &gui_x_max, 0.1f, 1.f);
    if (gui_x_max < gui_x_min) gui_x_max = gui_x_min;
    if(ImGui::Button("Regenerate Histogram"))
    {
        delete histogramBins;
        histogramBins = loadHistogramBins(
            vConf, volumeData, gui_num_bins, gui_x_min, gui_x_max);
    }
    ImGui::End();
}

/**
 * \brief Shows and handles the ImGui Window for the transfer function editor
*/
static void showTransferFunctionWindow()
{
    ImGui::Begin("Transfer Function Editor", &gui_show_tf_window);
    ImGui::Text("Here comes the transfer function editor");
    ImGui::End();
}

/**
 * \brief Bins the volume data according to its data type
 *
 * \param vConf configuration object for the dataset
 * \param values pointer to the volume data
 * \param numBins number of histogram bins
 * \param min lower histogram x axis limit
 * \param max upper histogram x axis limit
 *
 * \returns an vector of bin objects
 *
 * Note: vector of bins has to be deleted by the calling function
*/
std::vector<util::bin_t > *loadHistogramBins(
    cr::VolumeConfig vConf, void* values, size_t numBins, float min, float max)
{
    std::vector<util::bin_t> *bins = nullptr;

    switch(vConf.getVoxelType())
    {
        case cr::Datatype::unsigned_byte:
            bins = util::binData<unsigned_byte_t>(
                numBins,
                static_cast<unsigned_byte_t>(min),
                static_cast<unsigned_byte_t>(max),
                reinterpret_cast<unsigned_byte_t*>(values),
                vConf.getVoxelCount());
            break;

        case cr::Datatype::signed_byte:
            bins = util::binData<signed_byte_t>(
                numBins,
                static_cast<signed_byte_t>(min),
                static_cast<signed_byte_t>(max),
                reinterpret_cast<signed_byte_t*>(values),
                vConf.getVoxelCount());
            break;

        case cr::Datatype::unsigned_halfword:
            bins = util::binData<unsigned_halfword_t>(
                numBins,
                static_cast<unsigned_halfword_t>(min),
                static_cast<unsigned_halfword_t>(max),
                reinterpret_cast<unsigned_halfword_t*>(values),
                vConf.getVoxelCount());
            break;

        case cr::Datatype::signed_halfword:
            bins = util::binData<signed_halfword_t>(
                numBins,
                static_cast<signed_halfword_t>(min),
                static_cast<signed_halfword_t>(max),
                reinterpret_cast<signed_halfword_t*>(values),
                vConf.getVoxelCount());
            break;

        case cr::Datatype::unsigned_word:
            bins = util::binData<unsigned_word_t>(
                numBins,
                static_cast<unsigned_word_t>(min),
                static_cast<unsigned_word_t>(max),
                reinterpret_cast<unsigned_word_t*>(values),
                vConf.getVoxelCount());
            break;

        case cr::Datatype::signed_word:
            bins = util::binData<signed_word_t>(
                numBins,
                static_cast<signed_word_t>(min),
                static_cast<signed_word_t>(max),
                reinterpret_cast<signed_word_t*>(values),
                vConf.getVoxelCount());
            break;

        case cr::Datatype::unsigned_longword:
            bins = util::binData<unsigned_longword_t>(
                numBins,
                static_cast<unsigned_longword_t>(min),
                static_cast<unsigned_longword_t>(max),
                reinterpret_cast<unsigned_longword_t*>(values),
                vConf.getVoxelCount());
            break;

        case cr::Datatype::signed_longword:
            bins = util::binData<signed_longword_t>(
                numBins,
                static_cast<signed_longword_t>(min),
                static_cast<signed_longword_t>(max),
                reinterpret_cast<signed_longword_t*>(values),
                vConf.getVoxelCount());
            break;

        case cr::Datatype::single_precision_float:
            bins = util::binData<single_precision_float_t>(
                numBins,
                static_cast<single_precision_float_t>(min),
                static_cast<single_precision_float_t>(max),
                reinterpret_cast<single_precision_float_t*>(values),
                vConf.getVoxelCount());
            break;

        case cr::Datatype::double_precision_float:
            bins = util::binData<double_precision_float_t>(
                numBins,
                static_cast<double_precision_float_t>(min),
                static_cast<double_precision_float_t>(max),
                reinterpret_cast<double_precision_float_t*>(values),
                vConf.getVoxelCount());
            break;

        default:
            break;
    }

    return bins;
}

GLuint loadScalarVolumeTex(cr::VolumeConfig vConf, void* volumeData)
{
    GLuint tex = 0;

    switch(vConf.getVoxelType())
    {
        case cr::Datatype::unsigned_byte:
            tex = util::create3dTexFromScalar(
                volumeData,
                GL_UNSIGNED_BYTE,
                vConf.getVolumeDim()[0],
                vConf.getVolumeDim()[1],
                vConf.getVolumeDim()[2]);
            break;

        case cr::Datatype::signed_byte:
            tex = util::create3dTexFromScalar(
                volumeData,
                GL_BYTE,
                vConf.getVolumeDim()[0],
                vConf.getVolumeDim()[1],
                vConf.getVolumeDim()[2]);
            break;

        case cr::Datatype::unsigned_halfword:
            tex = util::create3dTexFromScalar(
                volumeData,
                GL_UNSIGNED_SHORT,
                vConf.getVolumeDim()[0],
                vConf.getVolumeDim()[1],
                vConf.getVolumeDim()[2]);
            break;

        case cr::Datatype::signed_halfword:
            tex = util::create3dTexFromScalar(
                volumeData,
                GL_SHORT,
                vConf.getVolumeDim()[0],
                vConf.getVolumeDim()[1],
                vConf.getVolumeDim()[2]);
            break;

        case cr::Datatype::unsigned_word:
            tex = util::create3dTexFromScalar(
                volumeData,
                GL_UNSIGNED_INT,
                vConf.getVolumeDim()[0],
                vConf.getVolumeDim()[1],
                vConf.getVolumeDim()[2]);
            break;

        case cr::Datatype::signed_word:
            tex = util::create3dTexFromScalar(
                volumeData,
                GL_INT,
                vConf.getVolumeDim()[0],
                vConf.getVolumeDim()[1],
                vConf.getVolumeDim()[2]);
            break;

        case cr::Datatype::single_precision_float:
            tex = util::create3dTexFromScalar(
                volumeData,
                GL_FLOAT,
                vConf.getVolumeDim()[0],
                vConf.getVolumeDim()[1],
                vConf.getVolumeDim()[2]);
            break;

        case cr::Datatype::double_precision_float:
        case cr::Datatype::unsigned_longword:
        case cr::Datatype::signed_longword:
        default:
            break;
    }

    return tex;
}

/**
 * \brief updates the volume data, texture, histogram information...
*/
static void reloadStuff(
    cr::VolumeConfig vConf,
    void *&volumeData,
    GLuint &volumeTex,
    unsigned int timestep,
    glm::mat4 &modelMX,
    std::vector<util::bin_t> *&histogramBins,
    size_t num_bins,
    float x_min,
    float x_max)
{
    glDeleteTextures(1, &volumeTex);
    if (volumeData != nullptr) cr::deleteVolumeData(vConf, volumeData);
    if (histogramBins != nullptr) delete histogramBins;

    volumeData = cr::loadScalarVolumeDataTimestep(vConf, timestep, false);
    modelMX = glm::scale(
        glm::mat4(1.f),
        glm::normalize(glm::vec3(
            static_cast<float>(vConf.getVolumeDim()[0]),
            static_cast<float>(vConf.getVolumeDim()[1]),
            static_cast<float>(vConf.getVolumeDim()[2]))));
    histogramBins = loadHistogramBins(
        vConf, volumeData, num_bins, x_min, x_max);
    volumeTex = loadScalarVolumeTex(vConf, volumeData);
}
