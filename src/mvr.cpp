#include "mvr.hpp"

#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <vector>
#include <string>
#include <exception>
#include <algorithm>
#include <ctime>
#include <utility>
#include <memory>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_stl.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <FreeImage.h>

#include <json.hpp>
using json = nlohmann::json;

#include "shader.hpp"
#include "util/util.hpp"
#include "configraw.hpp"

//-----------------------------------------------------------------------------
// definition of static member variables
//-----------------------------------------------------------------------------
const std::string mvr::Renderer::DEFAULT_VOLUME_FILE("exampleData/bucky.json");
const glm::vec3 mvr::Renderer::DEFAULT_CAMERA_POSITION(1.2f, 0.75f, 1.f);
const glm::vec3 mvr::Renderer::DEFAULT_CAMERA_LOOKAT(0.f);

//-----------------------------------------------------------------------------
// public member implementations
//-----------------------------------------------------------------------------
mvr::Renderer::Renderer() :
    // window and gui size
    m_windowDimensions{ {1600, 900} },
    m_renderingDimensions{ {1920, 1080} },
    m_tfFuncWidgetDimensions{ {384, 96} },
    m_tfColorWidgetDimensions{ {384, 16} },
    // output mode
    m_renderMode(mvr::Mode::line_of_sight),
    m_outputSelect(mvr::Output::volume_rendering),
    // detailled mode and output settings
    m_showVolumeFrame(true),
    m_showWireframe(false),
    m_showDemoWindow(false),
    m_showTfWindow(true),
    m_showHistogramWindow(true),
    m_semilogHistogram(false),
    m_binNumberHistogram(255),
    m_yLimitHistogramMax(100000),
    m_xLimitsMin(0.f),
    m_xLimitsMax(255.f),
    m_invertColors(false),
    m_invertAlpha(false),
    m_clearColor{ {0.f} },
    // data selection
    m_volumeDescriptionFile(mvr::Renderer::DEFAULT_VOLUME_FILE),
    m_timestep(0),
    m_outputDataZSlice(0.f),
    // ray casting
    m_stepSize(0.25f),
    m_gradientMethod(mvr::Gradient::sobel_operators),
    // camera settings
    m_fovY(80.f),
    m_zNear(0.000001f),
    m_zFar(30.f),
    m_cameraPosition(mvr::Renderer::DEFAULT_CAMERA_POSITION),
    m_cameraLookAt(mvr::Renderer::DEFAULT_CAMERA_LOOKAT),
    m_cameraZoomSpeed(0.1f),
    m_cameraRotationSpeed(0.2f),
    m_cameraTranslationSpeed(0.002f),
    m_projection(mvr::Projection::perspective),
    // isosurface mode
    m_isovalue(0.1f),
    m_isovalueDenoising(true),
    m_isovalueDenoisingRadius(0.1f),
    // lighting
    m_brightness(1.f),
    m_lightDirection{ {0.3f, 1.f, -0.3f} },
    m_ambientColor{ {0.2f, 0.2f, 0.2f} },
    m_diffuseColor{ {1.f, 1.f, 1.f} },
    m_specularColor{ {1.f, 1.f, 1.f} },
    m_ambientFactor(0.2f),
    m_diffuseFactor(0.3f),
    m_specularFactor(0.5f),
    m_specularExponent(10.0f),
    // slicing plane functionality
    m_slicingPlane(false),
    m_slicingPlaneNormal{ {0.f, 0.f, 1.f} },
    m_slicingPlaneBase{ {0.f, 0.f, 0.f} },
    // ambient occlussion functionality
    m_ambientOcclusion(false),
    m_ambientOcclusionRadius(0.2f),
    m_ambientOcclusionProportion(0.5f),
    m_ambientOcclusionNumSamples(10),
    // internal member variables
    m_isInitialized(false),
    m_window(nullptr),
    m_shaderQuad(),
    m_shaderFrame(),
    m_shaderVolume(),
    m_shaderTfColor(),
    m_shaderTfFunc(),
    m_shaderTfPoint(),
    m_framebuffers(),
    m_tfColorWidgetFBO(),
    m_tfFuncWidgetFBO(),
    m_volumeFrame(false),
    m_volumeCube(false),
    m_windowQuad(false),
    m_tfPoint(false),
    m_boundingBoxMin(glm::vec3(-0.5f), 1.f),
    m_boundingBoxMax(glm::vec3(0.5f), 1.f),
    m_volumeModelMx(1.f),
    m_volumeViewMx(1.f),
    m_volumeProjMx(1.f),
    m_quadProjMx(glm::ortho(-0.5f, 0.5f, -0.5f, 0.5f)),
    m_histogramBins(0),
    m_transferFunction(),
    m_volumeData(nullptr),
    m_volumeTex(),
    m_randomSeedTex(),
    m_voxelDiagonal(1.f),
    m_showMenues(true),
    m_showControlPointList(false),
    m_tfScreenPosition{ {0, 0} },
    m_selectedTfControlPointPos(0.f)
{
    // nothing to see here
}

mvr::Renderer::~Renderer()
{
    if (nullptr != m_window)
        glfwDestroyWindow(m_window);

    glfwTerminate();
}

int mvr::Renderer::initialize()
{
    int ret = EXIT_SUCCESS;

    //-------------------------------------------------------------------------
    // window and context creation
    //-------------------------------------------------------------------------
    m_window = createWindow(
        m_windowDimensions[0], m_windowDimensions[1], "MVR");

    ret = initializeGl3w();
    if (EXIT_SUCCESS != ret) return ret;

    ret = initializeImGui();
    if (EXIT_SUCCESS != ret) return ret;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPointSize(13.f);

    //-------------------------------------------------------------------------
    // shader setup
    //-------------------------------------------------------------------------
    m_shaderQuad = Shader("src/shader/quad.vert", "src/shader/quad.frag");
    m_shaderFrame = Shader("src/shader/frame.vert", "src/shader/frame.frag");
    m_shaderVolume =
        Shader("src/shader/volume.vert", "src/shader/volume.frag");
    m_shaderTfColor =
        Shader("src/shader/tfColor.vert", "src/shader/tfColor.frag");
    m_shaderTfFunc =
        Shader("src/shader/tfFunc.vert", "src/shader/tfFunc.frag");
    m_shaderTfPoint =
        Shader("src/shader/tfPoint.vert", "src/shader/tfPoint.frag");

    //-------------------------------------------------------------------------
    // ping pong framebuffers and rendering targets
    //-------------------------------------------------------------------------
    updatePingPongFramebufferObjects();

    // ------------------------------------------------------------------------
    // geometry
    // ------------------------------------------------------------------------
    m_volumeFrame = util::geometry::CubeFrame(true);
    m_volumeCube = util::geometry::Cube(true);
    m_windowQuad = util::geometry::Quad(true);
    m_tfPoint = util::geometry::Point2D(true);

    //-------------------------------------------------------------------------
    // utility textures
    //-------------------------------------------------------------------------
    // seed texture for fragment shader random number generator
    m_randomSeedTex = util::texture::create2dHybridTausTexture(
        m_renderingDimensions[0], m_renderingDimensions[1]);

    //-------------------------------------------------------------------------
    // volume data and transfer function loading
    //-------------------------------------------------------------------------
    // load the data, adjust the model matrix, prepare a histogram of the data
    // and a texture object
    cr::VolumeConfig volumeConfig = cr::VolumeConfig(m_volumeDescriptionFile);
    if(volumeConfig.isValid())
        loadVolume(volumeConfig, m_timestep);
    else
    {
        ret = EXIT_FAILURE;
        return ret;
    }

    // create a transfer function object
    m_transferFunction = util::tf::TransferFuncRGBA1D();

    //-------------------------------------------------------------------------
    // transformation matrices
    //-------------------------------------------------------------------------
    // view and projection matrices for the volume (model matrix is set during
    // loading of the volume)
    glm::vec3 right = glm::normalize(
        glm::cross(-m_cameraPosition, glm::vec3(0.f, 1.f, 0.f)));
    glm::vec3 up = glm::normalize(glm::cross(right, -m_cameraPosition));
    m_volumeViewMx = glm::lookAt(m_cameraPosition, m_cameraLookAt, up);

    m_volumeProjMx= glm::perspective(
        glm::radians(m_fovY),
        static_cast<float>(m_renderingDimensions[0]) /
            static_cast<float>(m_renderingDimensions[1]),
        0.1f,
        50.0f);

    if (EXIT_SUCCESS == ret)
        m_isInitialized = true;

    return ret;
}

int mvr::Renderer::run()
{
    int ret = EXIT_SUCCESS;

    if (false == m_isInitialized)
    {
        std::cerr << "Error: Renderer::initialize() must be called "
            "successfully before Renderer::run() can be used!" <<
            std::endl;
        return EXIT_FAILURE;
    }

    // ------------------------------------------------------------------------
    // local variables
    // ------------------------------------------------------------------------
    static unsigned int ping = 0, pong = 1;

    // ------------------------------------------------------------------------
    // gui widget framebuffer objects
    // ------------------------------------------------------------------------
    // render target for widget that shows resulting transfer function colors
    {
        std::vector<util::texture::Texture2D> tfColorWidgetFboTextures;
        tfColorWidgetFboTextures.emplace_back(
            GL_RGBA,
            GL_RGBA,
            0,
            GL_FLOAT,
            GL_LINEAR,
            GL_CLAMP_TO_EDGE,
            m_tfColorWidgetDimensions[0],
            m_tfColorWidgetDimensions[1]);
        const std::vector<GLenum> tfColorWidgetFboAttachments = {
            GL_COLOR_ATTACHMENT0 };
        m_tfColorWidgetFBO = util::FramebufferObject(
            std::move(tfColorWidgetFboTextures), tfColorWidgetFboAttachments);
    }

    // render target that shows transfer function line plot
    {
        std::vector<util::texture::Texture2D> tfFuncWidgetTextures;
        tfFuncWidgetTextures.emplace_back(
            GL_RGBA,
            GL_RGBA,
            0,
            GL_FLOAT,
            GL_LINEAR,
            GL_CLAMP_TO_EDGE,
            m_tfFuncWidgetDimensions[0],
            m_tfFuncWidgetDimensions[1]);
        tfFuncWidgetTextures.emplace_back(
            GL_RG32F,
            GL_RG,
            0,
            GL_FLOAT,
            GL_NEAREST,
            GL_CLAMP_TO_BORDER,
            m_tfFuncWidgetDimensions[0],
            m_tfFuncWidgetDimensions[1]);
        const std::vector<GLenum> tfFuncWidgetFboAttachments =
            { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        m_tfFuncWidgetFBO = util::FramebufferObject(
            std::move(tfFuncWidgetTextures), tfFuncWidgetFboAttachments);
    }


    // ------------------------------------------------------------------------
    // render loop
    // ------------------------------------------------------------------------
    while (!glfwWindowShouldClose(m_window))
    {
        // swap fbo objects in each render pass
        std::swap(ping, pong);
        glfwPollEvents();

        // --------------------------------------------------------------------
        // draw the volume, frame etc. into a frame buffer object
        // --------------------------------------------------------------------
        // activate one of the framebuffer objects as rendering target
        glViewport(0, 0, m_renderingDimensions[0], m_renderingDimensions[1]);
        m_framebuffers[ping].bind();

        // clear old buffer content
        glClearColor(m_clearColor[0], m_clearColor[1], m_clearColor[2], 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        drawVolume(m_framebuffers[pong].accessTextures()[1]);

        // --------------------------------------------------------------------
        // show the rendering result as window filling quad in the default
        // framebuffer
        // --------------------------------------------------------------------
        glViewport(0, 0, m_windowDimensions[0], m_windowDimensions[1]);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        m_shaderQuad.use();

        glActiveTexture(GL_TEXTURE0);
        m_framebuffers[ping].accessTextures()[0].bind();
        m_shaderQuad.setInt("renderTex", 0);

        glActiveTexture(GL_TEXTURE1);
        m_framebuffers[ping].accessTextures()[1].bind();
        m_shaderQuad.setInt("rngTex", 1);

        glActiveTexture(GL_TEXTURE2);
        m_volumeTex.bind();
        m_shaderQuad.setInt("volumeTex", 2);
        m_shaderQuad.setFloat("volumeZ", m_outputDataZSlice);

        m_shaderQuad.setMat4("projMX", m_quadProjMx);
        m_shaderQuad.setInt("texSelect", static_cast<int>(m_outputSelect));

        m_windowQuad.draw();

        // draw ImGui windows
        if(m_showMenues)
        {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            drawSettingsWindow();
            if(m_showDemoWindow)
                ImGui::ShowDemoWindow(&m_showDemoWindow);
            if(m_showTfWindow)
                drawTransferFunctionWindow();
            if(m_showHistogramWindow)
                drawHistogramWindow();
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        glfwSwapBuffers(m_window);

        if (printOpenGLError())
            ret = EXIT_FAILURE;
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    return ret;
}

int mvr::Renderer::renderToFile(std::string path)
{
    int ret = EXIT_SUCCESS;

    if (false == m_isInitialized)
    {
        std::cerr << "Error: Renderer::initialize() must be called "
            "successfully before Renderer::renderToFile(...) can be used!" <<
            std::endl;
        return EXIT_FAILURE;
    }

    // ------------------------------------------------------------------------
    // local variables
    // ------------------------------------------------------------------------
    static unsigned int ping = 0, pong = 1;

    // swap fbo objects in each render pass
    std::swap(ping, pong);
    glfwPollEvents();

    // --------------------------------------------------------------------
    // draw the volume, frame etc. into a frame buffer object
    // --------------------------------------------------------------------
    // activate one of the framebuffer objects as rendering target
    glViewport(0, 0, m_renderingDimensions[0], m_renderingDimensions[1]);
    m_framebuffers[ping].bind();

    // clear old buffer content
    glClearColor(m_clearColor[0], m_clearColor[1], m_clearColor[2], 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawVolume(m_framebuffers[pong].accessTextures()[1]);

    if (printOpenGLError())
        ret = EXIT_FAILURE;

    util::makeScreenshot(
        m_framebuffers[ping],
        m_renderingDimensions[0],
        m_renderingDimensions[1],
        path,
        FIF_TIFF);

    return ret;
}

int mvr::Renderer::saveConfigToFile(std::string path)
{
    int ret = EXIT_SUCCESS;

    try
    {
        std::ofstream ofs(path, std::ofstream::out);
        json conf;

        conf["volumeDescriptionFile"] = m_volumeDescriptionFile;
        conf["timestep"] = m_timestep;

        conf["renderingDimensions"] = m_renderingDimensions;

        conf["renderMode"] = m_renderMode;
        conf["outputSelect"] = m_outputSelect;

        conf["showVolumeFrame"] = m_showVolumeFrame;
        conf["showWireframe"] = m_showWireframe;
        conf["showDemoWindow"] = m_showDemoWindow;
        conf["showTfWindow"] = m_showTfWindow;
        conf["showHistogramWindow"] = m_showHistogramWindow;
        conf["semilogHistogram"] = m_semilogHistogram;
        conf["binNumberHistogram"] = m_binNumberHistogram;
        conf["yLimitHistogramMax"] = m_yLimitHistogramMax;
        conf["xLimitsMin"] = m_xLimitsMin;
        conf["xLimitsMax"] = m_xLimitsMax;
        conf["invertColors"] = m_invertColors;
        conf["invertAlpha"] = m_invertAlpha;
        conf["clearColor"] = m_clearColor;
        conf["outputDataZSlice"] = m_outputDataZSlice;

        conf["stepSize"] = m_stepSize;
        conf["gradientMethod"] = m_gradientMethod;

        conf["fovY"] = m_fovY;
        conf["zNear"] = m_zNear;
        conf["zFar"] = m_zFar;
        conf["cameraPosition"] = std::array<float, 3>{
                m_cameraPosition.x, m_cameraPosition.y, m_cameraPosition.z};
        conf["cameraLookAt"] = std::array<float, 3>{
                m_cameraLookAt.x, m_cameraLookAt.y, m_cameraLookAt.z};
        conf["cameraZoomSpeed"] = m_cameraZoomSpeed;
        conf["cameraRotationSpeed"] = m_cameraRotationSpeed;
        conf["cameraTranslationSpeed"] = m_cameraTranslationSpeed;
        conf["projection"] = m_projection;

        conf["isovalue"] = m_isovalue;
        conf["isovalueDenoising"] = m_isovalueDenoising;
        conf["isovalueDenoisingRadius"] = m_isovalueDenoisingRadius;

        conf["brightness"] = m_brightness;
        conf["lightDirection"] = m_lightDirection;
        conf["ambientColor"] = m_ambientColor;
        conf["diffuseColor"] = m_diffuseColor;
        conf["specularColor"] = m_specularColor;
        conf["ambientFactor"] = m_ambientFactor;
        conf["diffuseFactor"] = m_diffuseFactor;
        conf["specularFactor"] = m_specularFactor;
        conf["specularExponent"] = m_specularExponent;

        conf["slicingPlane"] = m_slicingPlane;
        conf["slicingPlaneNormal"] = m_slicingPlaneNormal;
        conf["slicingPlaneBase"] = m_slicingPlaneBase;

        conf["ambientOcclusion"] = m_ambientOcclusion;
        conf["ambientOcclusionRadius"] = m_ambientOcclusionRadius;
        conf["ambientOcclusionProportion"] = m_ambientOcclusionProportion;
        conf["ambientOcclusionNumSamples"] = m_ambientOcclusionNumSamples;

        std::vector<json> transferFunction;
        json tfPoint;
        for (
            auto it = m_transferFunction.accessControlPoints()->cbegin();
            it != m_transferFunction.accessControlPoints()->cend();
            it++)
        {
            tfPoint["position"] = it->pos;
            tfPoint["color"] =
                std::array<float, 3>{ it->color.r, it->color.g, it->color.b };
            tfPoint["alpha"] = it->color.a;
            tfPoint["slope"] = it->fderiv;

            transferFunction.push_back(tfPoint);
        }
        conf["transferFunction"] = transferFunction;

        ofs << std::setw(4) << conf << std::endl;
        ofs.close();
    }
    catch(json::exception &e)
    {
        std::cout << "Error saving renderer configuration to file: " <<
            path << std::endl;
        std::cout << "JSON exception: " << e.what() << std::endl;
        ret = EXIT_FAILURE;
    }
    catch(std::exception &e)
    {
        std::cout << "Error saving renderer configuration to file: " <<
            path << std::endl;
        std::cout << "General exception: " << e.what() << std::endl;
        ret = EXIT_FAILURE;
    }

    return ret;
}

int mvr::Renderer::saveTransferFunctionToFile(std::string path)
{
    util::tf::discreteTf1D_t discreteTf =
            m_transferFunction.getDiscretized(m_xLimitsMin, m_xLimitsMax, 256);
    std::ofstream out(path);

    out << "index, red, green, blue, alpha" << std::endl;
    for (auto it = discreteTf.cbegin(); it != discreteTf.cend(); ++it)
    {
        out << it - discreteTf.cbegin() << "," <<
               (*it)[0] << "," <<
               (*it)[1] << "," <<
               (*it)[2] << "," <<
               (*it)[3] << std::endl;
    }

    out.close();

    return EXIT_SUCCESS;
}

//-----------------------------------------------------------------------------
// public functions for setting the renderer configuration
//-----------------------------------------------------------------------------
/**
* \brief applies settings from a json configuration file to the renderer
*
* \param path file path to the json settings configuration file
*
* \return exit code
*
* This function reads a json configuration file and applies the found settings
* to the state of the renderer. This functionality can be used for
* programatically settings the renderer.
*/
int mvr::Renderer::loadConfigFromFile(std::string path)
{
    int ret = EXIT_SUCCESS;
    std::ifstream fs;

    if (false == m_isInitialized)
    {
        std::cerr << "Error: Renderer::initialize() must be called "
            "successfully before Renderer::loadConfigFromFile() can be "
            "used!" <<
            std::endl;
        return EXIT_FAILURE;
    }

    fs.open(path.c_str(), std::ofstream::in);
    try
    {
        bool rebucket = false;
        json conf;

        fs >> conf;

        if (!conf["renderMode"].is_null())
            m_renderMode = conf["renderMode"].get<Mode>();
        if (!conf["outputSelect"].is_null())
        m_outputSelect = conf["outputSelect"].get<Output>();

        if (!conf["showVolumeFrame"].is_null())
            m_showVolumeFrame = conf["showVolumeFrame"].get<bool>();
        if (!conf["m_showWireframe"].is_null())
            m_showWireframe = conf["showWireframe"].get<bool>();
        if (!conf["showDemoWindow"].is_null())
            m_showDemoWindow = conf["showDemoWindow"].get<bool>();
        if (!conf["showTfWindow"].is_null())
            m_showTfWindow = conf["showTfWindow"].get<bool>();
        if (!conf["showHistogramWindow"].is_null())
            m_showHistogramWindow = conf["showHistogramWindow"].get<bool>();
        if (!conf["semilogHistogram"].is_null())
            m_semilogHistogram = conf["semilogHistogram"].get<bool>();
        if (!conf["binNumberHistogram"].is_null())
        {
            m_binNumberHistogram = conf["binNumberHistogram"].get<int>();
            rebucket = true;
        }
        if (!conf["yLimitHistogramMax"].is_null())
            m_yLimitHistogramMax = conf["yLimitHistogramMax"].get<int>();
        if (!conf["xLimitsMin"].is_null())
        {
            m_xLimitsMin = conf["xLimitsMin"].get<float>();
            rebucket = true;
        }
        if (!conf["xLimitsMax"].is_null())
        {
            m_xLimitsMax = conf["xLimitsMax"].get<float>();
            rebucket = true;
        }
        if (!conf["invertColors"].is_null())
            m_invertColors = conf["invertColors"].get<bool>();
        if (!conf["invertAlpha"].is_null())
            m_invertAlpha = conf["invertAlpha"].get<bool>();
        if (!conf["clearColor"].is_null())
            m_clearColor = conf["clearColor"].get<std::array<float, 3>>();

        if (!conf["outputDataZSlice"].is_null())
            m_outputDataZSlice = conf["outputDataZSlice"].get<float>();

        if (!conf["stepSize"].is_null())
            m_stepSize = conf["stepSize"].get<float>();
        if (!conf["gradientMethod"].is_null())
            m_gradientMethod = conf["gradientMethod"].get<Gradient>();

        if (!conf["fovY"].is_null())
            m_fovY = conf["fovY"].get<float>();
        if (!conf["zNear"].is_null())
            m_zNear = conf["zNear"].get<float>();
        if (!conf["zFar"].is_null())
            m_zFar = conf["zFar"].get<float>();
        if (!conf["cameraPosition"].is_null())
            m_cameraPosition = glm::vec3(
                conf["cameraPosition"].get<std::array<float, 3>>()[0],
                conf["cameraPosition"].get<std::array<float, 3>>()[1],
                conf["cameraPosition"].get<std::array<float, 3>>()[2]);
        if (!conf["cameraLookAt"].is_null())
            m_cameraLookAt = glm::vec3(
                conf["cameraLookAt"].get<std::array<float, 3>>()[0],
                conf["cameraLookAt"].get<std::array<float, 3>>()[1],
                conf["cameraLookAt"].get<std::array<float, 3>>()[2]);
        if (!conf["cameraZoomSpeed"].is_null())
            m_cameraZoomSpeed = conf["cameraZoomSpeed"].get<float>();
        if (!conf["cameraRotationSpeed"].is_null())
            m_cameraRotationSpeed = conf["cameraRotationSpeed"].get<float>();
        if (!conf["cameraTranslationSpeed"].is_null())
            m_cameraTranslationSpeed =
                conf["cameraTranslationSpeed"].get<float>();
        if (!conf["projection"].is_null())
            m_projection = conf["projection"].get<Projection>();

        if (!conf["isovalue"].is_null())
            m_isovalue = conf["isovalue"].get<float>();
        if (!conf["isovalueDenoising"].is_null())
            m_isovalueDenoising = conf["isovalueDenoising"].get<bool>();
        if (!conf["isovalueDenoisingRadius"].is_null())
            m_isovalueDenoisingRadius =
                conf["isovalueDenoisingRadius"].get<float>();

        if (!conf["brightness"].is_null())
            m_brightness = conf["brightness"].get<float>();
        if (!conf["lightDirection"].is_null())
            m_lightDirection =
                conf["lightDirection"].get<std::array<float, 3>>();
        if (!conf["ambientColor"].is_null())
            m_ambientColor =
                conf["ambientColor"].get<std::array<float, 3>>();
        if (!conf["diffuseColor"].is_null())
            m_diffuseColor =
                conf["diffuseColor"].get<std::array<float, 3>>();
        if (!conf["specularColor"].is_null())
            m_specularColor =
                conf["specularColor"].get<std::array<float, 3>>();
        if (!conf["ambientFactor"].is_null())
            m_ambientFactor = conf["ambientFactor"].get<float>();
        if (!conf["diffuseFactor"].is_null())
            m_diffuseFactor = conf["diffuseFactor"].get<float>();
        if (!conf["specularFactor"].is_null())
            m_specularFactor = conf["specularFactor"].get<float>();
        if (!conf["specularExponent"].is_null())
            m_specularExponent = conf["specularExponent"].get<float>();

        if (!conf["slicingPlane"].is_null())
            m_slicingPlane = conf["slicingPlane"].get<bool>();
        if (!conf["slicingPlaneNormal"].is_null())
            m_slicingPlaneNormal =
                conf["slicingPlaneNormal"].get<std::array<float, 3>>();
        if (!conf["slicingPlaneBase"].is_null())
            m_slicingPlaneBase =
                conf["slicingPlaneBase"].get<std::array<float, 3>>();

        if (!conf["ambientOcclusion"].is_null())
            m_ambientOcclusion = conf["ambientOcclusion"].get<bool>();
        if (!conf["ambientOcclusionRadius"].is_null())
            m_ambientOcclusionRadius =
                conf["ambientOcclusionRadius"].get<float>();
        if (!conf["ambientOcclusionProportion"].is_null())
            m_ambientOcclusionProportion =
                conf["ambientOcclusionProportion"].get<float>();
        if (!conf["ambientOcclusionNumSamples"].is_null())
            m_ambientOcclusionNumSamples =
                conf["ambientOcclusionNumSamples"].get<int>();

        // create a the transfer function
        if (!conf["transferFunction"].is_null())
        {
            util::tf::TransferFuncRGBA1D tf;
            glm::vec3 color;
            for (
                json::const_iterator it = conf["transferFunction"].cbegin();
                it != conf["transferFunction"].cend();
                it++)
            {
                color = glm::vec3(
                        (*it)["color"].get<std::array<float, 3>>()[0],
                        (*it)["color"].get<std::array<float, 3>>()[1],
                        (*it)["color"].get<std::array<float, 3>>()[2]);

                tf.insertControlPoint(
                        (*it)["position"].get<float>(),
                        (*it)["slope"].get<float>(),
                        color,
                        (*it)["alpha"].get<float>());
            }
            tf.updateTexture();
            tf.updateTexture(
                std::max(
                    m_xLimitsMin,
                    tf.accessControlPoints()->begin()->pos),
                std::min(
                    m_xLimitsMax,
                    tf.accessControlPoints()->rbegin()->pos));
            m_transferFunction = std::move(tf);
        }

        // set size of the render image
        if (!conf["renderingDimensions"].is_null())
        {
            resizeRendering(
                conf["renderingDimensions"].get<
                    std::array<unsigned int, 2>>()[0],
                conf["renderingDimensions"].get<
                    std::array<unsigned int, 2>>()[1]);
        }

        // load volume
        if (!conf["timestep"].is_null())
            m_timestep = conf["timestep"].get<unsigned int>();
        if (!conf["volumeDescriptionFile"].is_null())
        {
            loadVolumeFromFile(
                conf["volumeDescriptionFile"].get<std::string>());
        }

        // bucket volume data if one of the limits was changed
        if (rebucket)
            m_histogramBins = bucketVolumeData(
             *m_volumeData, m_binNumberHistogram, m_xLimitsMin, m_xLimitsMax);
    }
    catch(json::exception &e)
    {
        std::cout << "Error loading renderer configuration from file: " <<
            path << std::endl;
        std::cout << "JSON exception: " << e.what() << std::endl;
        ret = EXIT_FAILURE;
    }
    catch(std::exception &e)
    {
        std::cout << "Error loading renderer configuration from file: " <<
            path << std::endl;
        std::cout << "General exception: " << e.what() << std::endl;
        ret = EXIT_FAILURE;
    }

    return ret;
}

int mvr::Renderer::loadVolumeFromFile(
        std::string path, unsigned int timestep)
{
    cr::VolumeConfig volumeConfig = cr::VolumeConfig(path);

    if (false == m_isInitialized)
    {
        std::cerr << "Error: Renderer::initialize() must be called "
            "successfully before Renderer::loadVolumeFromFile() can be used!"
            << std::endl;
        return EXIT_FAILURE;
    }

    if(volumeConfig.isValid())
        loadVolume(volumeConfig, timestep);
    else
        return EXIT_FAILURE;

    m_volumeDescriptionFile = path;
    return EXIT_SUCCESS;
}

//-----------------------------------------------------------------------------
// subroutines
//-----------------------------------------------------------------------------
void mvr::Renderer::drawVolume(const util::texture::Texture2D& stateInTexture)
{
    // ------------------------------------------------------------------------
    // local variables
    // ------------------------------------------------------------------------
    glm::vec3 tempVec3 = glm::vec3(0.f);
    glm::vec3 right(0.f), up(0.f);

    // ------------------------------------------------------------------------
    // draw the volume
    // ------------------------------------------------------------------------
    // first update model, view and projection matrix
    right = glm::normalize(
        glm::cross(-m_cameraPosition, glm::vec3(0.f, 1.f, 0.f)));
    up = glm::normalize(glm::cross(right, -m_cameraPosition));
    m_volumeViewMx = glm::lookAt(m_cameraPosition, m_cameraLookAt, up);
    if (m_projection == Projection::perspective)
    {
        m_volumeProjMx = glm::perspective(
            glm::radians(m_fovY),
            static_cast<float>(m_renderingDimensions[0]) /
                static_cast<float>(m_renderingDimensions[1]),
            m_zNear,
            m_zFar);
    }
    else
    {
        m_volumeProjMx = glm::orthoRH(
            -0.5f, 0.5f, -0.5f, 0.5f, m_zNear, m_zFar);
    }

    // apply gui settings
    if(m_showWireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // draw the bounding frame
    if(m_showVolumeFrame)
    {
        m_shaderFrame.use();
        m_shaderFrame.setMat4(
            "pvmMX", m_volumeProjMx * m_volumeViewMx * m_volumeModelMx);
        m_volumeFrame.draw();
    }

    // draw the volume
    m_shaderVolume.use();

    glActiveTexture(GL_TEXTURE0);
    m_volumeTex.bind();
    m_shaderVolume.setInt("volumeTex", 0);

    glActiveTexture(GL_TEXTURE1);
    m_transferFunction.accessTexture().bind();
    m_shaderVolume.setInt("transferfunctionTex", 1);

    glActiveTexture(GL_TEXTURE2);
    m_randomSeedTex.bind();
    m_shaderVolume.setInt("seed", 2);
    m_shaderVolume.setBool("useSeed", true);

    glActiveTexture(GL_TEXTURE3);
    stateInTexture.bind();
    m_shaderVolume.setInt("stateIn", 3);

    m_shaderVolume.setInt("winWidth", m_renderingDimensions[0]);
    m_shaderVolume.setInt("winHeight", m_renderingDimensions[1]);
    m_shaderVolume.setMat4("modelMX", m_volumeModelMx);
    m_shaderVolume.setMat4(
        "pvmMX", m_volumeProjMx * m_volumeViewMx * m_volumeModelMx);
    m_shaderVolume.setVec3("eyePos", m_cameraPosition);
    m_shaderVolume.setVec3("bbMin", m_boundingBoxMin.xyz);
    m_shaderVolume.setVec3("bbMax", m_boundingBoxMax.xyz);
    m_shaderVolume.setInt("mode", static_cast<int>(m_renderMode));
    m_shaderVolume.setInt(
        "gradMethod", static_cast<int>(m_gradientMethod));
    m_shaderVolume.setFloat("stepSize", m_voxelDiagonal * m_stepSize);
    m_shaderVolume.setFloat("stepSizeVoxel", m_stepSize);
    m_shaderVolume.setFloat("brightness", m_brightness);
    m_shaderVolume.setBool("ambientOcclusion", m_ambientOcclusion);
    m_shaderVolume.setInt("aoSamples", m_ambientOcclusionNumSamples);
    m_shaderVolume.setFloat(
        "aoRadius", m_voxelDiagonal * m_ambientOcclusionRadius);
    m_shaderVolume.setFloat("aoProportion", m_ambientOcclusionProportion);
    m_shaderVolume.setFloat("isovalue", m_isovalue);
    m_shaderVolume.setBool("isoDenoise", m_isovalueDenoising);
    m_shaderVolume.setFloat("isoDenoiseR",
        m_voxelDiagonal * m_isovalueDenoisingRadius);
    tempVec3 = glm::normalize( glm::vec3(
        m_lightDirection[0], m_lightDirection[1], m_lightDirection[2]));
    m_shaderVolume.setVec3(
        "lightDir", tempVec3[0], tempVec3[1], tempVec3[2]);
    m_shaderVolume.setVec3(
        "ambient",
        m_ambientColor[0], m_ambientColor[1], m_ambientColor[2]);
    m_shaderVolume.setVec3(
        "diffuse",
        m_diffuseColor[0], m_diffuseColor[1], m_diffuseColor[2]);
    m_shaderVolume.setVec3(
        "specular",
        m_specularColor[0], m_specularColor[1], m_specularColor[2]);
    m_shaderVolume.setFloat("kAmb", m_ambientFactor);
    m_shaderVolume.setFloat("kDiff", m_diffuseFactor);
    m_shaderVolume.setFloat("kSpec", m_specularFactor);
    m_shaderVolume.setFloat("kExp", m_specularExponent);
    m_shaderVolume.setBool("invertColors", m_invertColors);
    m_shaderVolume.setBool("invertAlpha", m_invertAlpha);
    m_shaderVolume.setBool("sliceVolume", m_slicingPlane);
    tempVec3 = glm::normalize( glm::vec3(
        m_slicingPlaneNormal[0],
        m_slicingPlaneNormal[1],
        m_slicingPlaneNormal[2]));
    m_shaderVolume.setVec3(
        "slicePlaneNormal", tempVec3[0], tempVec3[1], tempVec3[2]);
    tempVec3 = (m_volumeModelMx *
        glm::vec4(
            m_slicingPlaneBase[0] / 2.f,
            m_slicingPlaneBase[1] / 2.f,
            m_slicingPlaneBase[2] / 2.f,
            1.f)).xyz;
    m_shaderVolume.setVec3(
        "slicePlaneBase", tempVec3[0], tempVec3[1], tempVec3[2]);

    m_volumeCube.draw();

}

void mvr::Renderer::drawSettingsWindow()
{
    std::string volumeDescription(m_volumeDescriptionFile);
    cr::VolumeConfig tempConf;
    static int renderMode = static_cast<int>(m_renderMode);
    static int gradientMethod = static_cast<int>(m_gradientMethod);
    static int projection = static_cast<int>(m_projection);
    static int timestep = m_timestep;
    static int renderingDimensions[2] = {
        static_cast<int>(m_renderingDimensions[0]),
        static_cast<int>(m_renderingDimensions[1])};
    static int outputSelect = static_cast<int>(m_outputSelect);
    static time_t timer = std::time(nullptr);
    static char filename[200] = {};

    ImGui::Begin("Settings");
    {
        if(ImGui::InputText(
                "volume",
                &volumeDescription,
                ImGuiInputTextFlags_CharsNoBlank |
                    ImGuiInputTextFlags_EnterReturnsTrue))
        {
            tempConf = cr::VolumeConfig(volumeDescription);
            if(tempConf.isValid())
            {
                m_volumeDescriptionFile = volumeDescription;
                loadVolume(tempConf, 0);
            }
        }
        ImGui::SameLine();
        createHelpMarker("Path to the volume description file");
        ImGui::Text("Mode");
        ImGui::RadioButton(
            "line of sight",
            &renderMode,
            static_cast<int>(Mode::line_of_sight));
        ImGui::RadioButton(
            "maximum intensity projection",
            &renderMode,
            static_cast<int>(Mode::maximum_intensity_projection));
        ImGui::RadioButton(
            "isosurface",
            &renderMode,
            static_cast<int>(Mode::isosurface));
        ImGui::RadioButton(
            "transfer function",
            &renderMode,
            static_cast<int>(Mode::transfer_function));
        m_renderMode = static_cast<mvr::Mode>(renderMode);

        ImGui::Spacing();

        if(ImGui::InputInt(
            "timestep",
            &timestep,
            1,
            1,
            ImGuiInputTextFlags_CharsNoBlank |
                ImGuiInputTextFlags_EnterReturnsTrue))
        {
            if (timestep < 0) timestep = 0;
            else if(timestep >
                    static_cast<int>(
                        m_volumeData->getVolumeConfig().getNumTimesteps() - 1))
            {
                timestep = static_cast<int>(
                    m_volumeData->getVolumeConfig().getNumTimesteps() - 1);
            }

            loadVolume(m_volumeData->getVolumeConfig(), timestep);
        }

        ImGui::Spacing();

        ImGui::SliderFloat(
            "step size", &m_stepSize, 0.05f, 2.f, "%.3f");

        ImGui::Spacing();

        ImGui::InputFloat("brightness", &m_brightness, 0.01f, 0.1f);

        ImGui::Spacing();

        ImGui::Text("Gradient Calculation Method:");
        ImGui::RadioButton(
            "central differences",
            &gradientMethod,
            static_cast<int>(Gradient::central_differences));
        ImGui::RadioButton(
            "sobel operators",
            &gradientMethod,
            static_cast<int>(Gradient::sobel_operators));
        m_gradientMethod = static_cast<mvr::Gradient>(gradientMethod);

        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Isosurface"))
        {
            ImGui::SliderFloat(
                "isovalue", &m_isovalue, 0.f, 1.f, "%.3f");
            ImGui::Checkbox("denoise", &m_isovalueDenoising);
            ImGui::SliderFloat(
                "denoise radius",
                &m_isovalueDenoisingRadius,
                0.001f,
                5.f,
                "%.3f");
            if (ImGui::TreeNode("Lighting"))
            {
                ImGui::SliderFloat3(
                    "light direction", m_lightDirection.data(), -1.f, 1.f);
                ImGui::ColorEdit3("ambient", m_ambientColor.data());
                ImGui::ColorEdit3("diffuse", m_diffuseColor.data());
                ImGui::ColorEdit3("specular", m_specularColor.data());

                ImGui::Spacing();

                ImGui::SliderFloat("k_amb", &m_ambientFactor, 0.f, 1.f);
                ImGui::SliderFloat("k_diff", &m_diffuseFactor, 0.f, 1.f);
                ImGui::SliderFloat("k_spec", &m_specularFactor, 0.f, 1.f);
                ImGui::SliderFloat("k_exp", &m_specularExponent, 0.f, 50.f);
                ImGui::TreePop();
            }
        }

        if (ImGui::CollapsingHeader("Transfer Function"))
        {
            ImGui::Checkbox("show histogram", &m_showHistogramWindow);
            ImGui::SameLine();
            createHelpMarker(
                "Only visible in transfer function mode.");

            ImGui::Spacing();

            ImGui::Checkbox(
                "show transfer function editor", &m_showTfWindow);
            ImGui::SameLine();
            createHelpMarker(
                "Only visible in transfer function mode.");
        }

        if (ImGui::CollapsingHeader("Camera"))
        {
            ImGui::InputFloat(
                "camera zoom speed",
                &m_cameraZoomSpeed,
                0.01f,
                0.1f,
                "%.3f");
            ImGui::SameLine();
            createHelpMarker(
                "Scroll up or down while holding CTRL to zoom.");

            ImGui::InputFloat(
                "camera translation speed",
                &m_cameraTranslationSpeed,
                0.0001f,
                0.1f,
                "%.3f");
            ImGui::SameLine();
            createHelpMarker(
                "Hold the right mouse button and move the mouse to translate "
                "the camera");

            ImGui::InputFloat(
                "camera rotation speed",
                &m_cameraRotationSpeed,
                0.01f,
                0.1f,
                "%.3f");
            ImGui::SameLine();
            createHelpMarker(
                "Hold the middle mouse button and move the mouse to rotate "
                "the camera");

            glm::vec3 polar = util::cartesianToPolar<glm::vec3>(
                m_cameraPosition);
            ImGui::Text("phi: %.3f", polar.y);
            ImGui::Text("theta: %.3f", polar.z);
            ImGui::Text("radius: %.3f", polar.x);
            ImGui::Text(
                "Camera position: x=%.3f, y=%.3f, z=%.3f",
                m_cameraPosition.x,
                m_cameraPosition.y,
                m_cameraPosition.z);
            ImGui::Text(
                "Camera look at: x=%.3f, y=%.3f, z=%.3f",
                m_cameraLookAt.x,
                m_cameraLookAt.y,
                m_cameraLookAt.z);

            if(ImGui::Button("reset camera position"))
            {
                m_cameraPosition = mvr::Renderer::DEFAULT_CAMERA_POSITION;
                m_cameraLookAt = mvr::Renderer::DEFAULT_CAMERA_LOOKAT;
            }

            ImGui::Separator();

            ImGui::SliderFloat(
                "vertical field of view",
                &m_fovY,
                10.f,
                160.f);

            ImGui::Separator();

            ImGui::RadioButton(
                "perspective",
                &projection,
                static_cast<int>(Projection::perspective)); ImGui::SameLine();
            ImGui::RadioButton(
                "orthographic",
                &projection,
                static_cast<int>(Projection::orthographic));
            m_projection = static_cast<mvr::Projection>(projection);
        }

        if (ImGui::CollapsingHeader("General"))
        {
            ImGui::DragInt2(
                    "Rendering Resolution",
                    renderingDimensions,
                    1.0f,
                    240,
                    8192);
            if (ImGui::Button("Change Resolution"))
                resizeRendering(
                    renderingDimensions[0], renderingDimensions[1]);

            ImGui::Separator();

            ImGui::ColorEdit3("background color", m_clearColor.data());

            ImGui::Separator();

            ImGui::Checkbox(
                "show ImGui demo window", &m_showDemoWindow);

            ImGui::Separator();

            ImGui::Checkbox("draw frame", &m_showVolumeFrame);
            ImGui::SameLine();
            ImGui::Checkbox("wireframe", &m_showWireframe);

            ImGui::Separator();

            ImGui::Checkbox("invert colors", &m_invertColors);
            ImGui::SameLine();
            ImGui::Checkbox("invert alpha", &m_invertAlpha);

            ImGui::Separator();

            ImGui::Checkbox("slice volume", &m_slicingPlane);
            ImGui::SliderFloat3(
                "slicing plane normal",
                m_slicingPlaneNormal.data(),
                -1.f,
                1.f);
            ImGui::SliderFloat3(
                "slicing plane base",
                m_slicingPlaneBase.data(),
                -1.f,
                1.f);

            ImGui::Separator();

            ImGui::Checkbox("ambient occlusion", &m_ambientOcclusion);
            ImGui::SliderFloat(
                "proportion", &m_ambientOcclusionProportion, 0.f, 1.f);
            ImGui::SliderFloat(
                "halfdome radius", &m_ambientOcclusionRadius, 0.01f, 10.f);
            ImGui::SliderInt(
                "number of samples", &m_ambientOcclusionNumSamples, 1, 100);

            ImGui::Separator();

            ImGui::RadioButton(
                "volume rendering",
                &outputSelect,
                static_cast<int>(Output::volume_rendering));
            ImGui::RadioButton(
                "random number generator",
                &outputSelect,
                static_cast<int>(Output::random_number_generator));
            ImGui::RadioButton(
                "volume data slice",
                &outputSelect,
                static_cast<int>(Output::volume_data_slice));
            m_outputSelect = static_cast<mvr::Output>(outputSelect);
            ImGui::SliderFloat(
                "volume z coordinate", &m_outputDataZSlice, 0.f, 1.f);
        }

        ImGui::Separator();

        if(ImGui::Button("save screenshot"))
        {
            timer = std::time(nullptr);
            std::time_t t = std::time(nullptr);
            std::tm* tm = std::localtime(&t);

            strftime(
                filename,
                sizeof(filename),
                "./screenshots/%F_%H%M%S.tiff",
                tm);

            util::makeScreenshot(
                m_framebuffers[0],
                m_renderingDimensions[0],
                m_renderingDimensions[1],
                filename,
                FIF_TIFF);
        }
        ImGui::SameLine();
        if(ImGui::Button("save configuration"))
        {
            timer = std::time(nullptr);
            std::time_t t = std::time(nullptr);
            std::tm* tm = std::localtime(&t);

            strftime(
                filename,
                sizeof(filename),
                "./configurations/%F_%H%M%S.json",
                tm);
            saveConfigToFile(std::string(filename));
        }


        if ((std::difftime(std::time(nullptr), timer) < 3.f) &&
            (filename[0] != '\0'))
        {
            ImGui::Separator();
            ImGui::Text("Saved to %s", filename);
        }

        ImGui::Separator();

        ImGui::Text(
            "Application average %.3f ms/frame (%.1f FPS)",
            1000.0f / ImGui::GetIO().Framerate,
            ImGui::GetIO().Framerate);
    }
    ImGui::End();
}

void mvr::Renderer::drawHistogramWindow()
{
    float values[m_histogramBins.size()];

    for(size_t i = 0; i < m_histogramBins.size(); i++)
    {
        if (m_semilogHistogram)
            values[i] = log10(std::get<2>(m_histogramBins[i]));
        else
            values[i] = static_cast<float>(std::get<2>(m_histogramBins[i]));
    }

    ImGui::Begin("Histogram", &m_showHistogramWindow);
    ImGui::PushItemWidth(-1);
    ImGui::PlotHistogram(
        "",
        values,
        m_histogramBins.size(),
        0,
        nullptr,
        0.f,
        m_semilogHistogram ?
            static_cast<int>(log10(m_yLimitHistogramMax)) :
            m_yLimitHistogramMax,
        ImVec2(0, 160));
    ImGui::PopItemWidth();
    ImGui::InputInt("y limit", &m_yLimitHistogramMax, 1, 100);
    ImGui::SameLine();
    ImGui::Checkbox("semi-logarithmic", &m_semilogHistogram);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::DragFloatRange2(
            "interval",
            &m_xLimitsMin,
            &m_xLimitsMax,
            1.f,
            0.f,
            0.f,
            "Min: %.1f",
            "Max: %.1f");
    ImGui::InputInt("number of bins", &m_binNumberHistogram);
    if(ImGui::Button("Regenerate Histogram"))
    {
        m_histogramBins = cr::bucketVolumeData(
            *m_volumeData,
            m_binNumberHistogram,
            m_xLimitsMin,
            m_xLimitsMax);
    }
    ImGui::End();
}

/**
 * \brief Shows and handles the ImGui Window for the transfer function editor
*/
void mvr::Renderer::drawTransferFunctionWindow()
{
    glm::vec4 tempVec4 = glm::vec4(0.f);
    util::tf::ControlPointRGBA1D cp = util::tf::ControlPointRGBA1D();
    static util::tf::controlPointSet1D_t::iterator cpSelected;
    util::tf::controlPointSet1D_t::iterator cpIterator;
    std::pair<util::tf::controlPointSet1D_t::iterator, bool> ret;
    ImVec2 tfScreenPosition = ImVec2();
    static float tfControlPointPos = 0.f, tfControlPointAlpha = 0.f;
    static float tfControlPointColor[3] = {0.f, 0.f, 0.f};
    static time_t timer = std::time(nullptr);
    static char filename[200] = {};

    // render the transfer function
    drawTfColor(m_tfColorWidgetFBO);
    drawTfFunc(m_tfFuncWidgetFBO);

    // draw the imgui elements
    ImGui::Begin("Transfer Function Editor", &m_showTfWindow);

    ImGui::DragFloatRange2(
            "interval",
            &m_xLimitsMin,
            &m_xLimitsMax,
            1.f,
            0.f,
            0.f,
            "Min: %.1f",
            "Max: %.1f");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    tfScreenPosition = ImGui::GetCursorScreenPos();
    m_tfScreenPosition[0] = tfScreenPosition[0];
    m_tfScreenPosition[1] = tfScreenPosition[1];
    ImGui::Image(
        reinterpret_cast<ImTextureID>(
            m_tfFuncWidgetFBO.accessTextures()[0].getID()),
        ImVec2(m_tfFuncWidgetDimensions[0], m_tfFuncWidgetDimensions[1]));

    ImGui::Spacing();

    ImGui::Image(
        reinterpret_cast<ImTextureID>(
            m_tfColorWidgetFBO.accessTextures()[0].getID()),
        ImVec2(m_tfColorWidgetDimensions[0], m_tfColorWidgetDimensions[1]));


    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Edit selected control point:");

    // get attributes of the selected control point
    cp = util::tf::ControlPointRGBA1D(m_selectedTfControlPointPos);
    cpIterator = m_transferFunction.accessControlPoints()->find(cp);
    if (m_transferFunction.accessControlPoints()->cend() != cpIterator)
    {
        cpSelected = cpIterator;
    }
    cp = *cpSelected;

    ImGui::SliderFloat(
        "position##edit", &(cp.pos), m_xLimitsMin, m_xLimitsMax);
    ImGui::SliderFloat("slope##edit", &(cp.fderiv), -10.f, 10.f);
    ImGui::ColorEdit3("assigned color##edit", glm::value_ptr(cp.color));
    ImGui::SliderFloat("alpha##edit", &(cp.color.a), 0.f, 1.f);

    if (cp != (*cpSelected))
    {
        ret = m_transferFunction.updateControlPoint(cpIterator, cp);
        if (ret.second == true)
        {
            m_transferFunction.updateTexture();
            cpSelected = ret.first;
            m_selectedTfControlPointPos = cpSelected->pos;
        }
    }
    if (ImGui::Button("remove"))
    {
        if(m_transferFunction.accessControlPoints()->size() > 1)
        {
            m_transferFunction.removeControlPoint(cpSelected);
            cpSelected = m_transferFunction.accessControlPoints()->begin();
            m_transferFunction.updateTexture(
                std::max(
                    m_xLimitsMin,
                    m_transferFunction.accessControlPoints()->begin()->pos),
                std::min(
                    m_xLimitsMax,
                    m_transferFunction.accessControlPoints()->rbegin()->pos));
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Add new control point:");
    ImGui::SliderFloat(
        "position", &tfControlPointPos, m_xLimitsMin, m_xLimitsMax);
    ImGui::ColorEdit3("assigned color", tfControlPointColor);
    ImGui::SliderFloat("alpha", &tfControlPointAlpha, 0.f, 1.f);
    if(ImGui::Button("add"))
    {
        tempVec4 = glm::vec4(
            tfControlPointColor[0],
            tfControlPointColor[1],
            tfControlPointColor[2],
            tfControlPointAlpha);

        ret = m_transferFunction.insertControlPoint(
            tfControlPointPos, tempVec4);
        if(ret.second == true)
        {
            m_transferFunction.updateTexture(
                std::max(
                    m_xLimitsMin,
                    m_transferFunction.accessControlPoints()->begin()->pos),
                std::min(
                    m_xLimitsMax,
                    m_transferFunction.accessControlPoints()->rbegin()->pos));
        }
    }

    // dynamic list of control points
    if(m_showControlPointList)
    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        int idx = 0;
        for (
                auto i = m_transferFunction.accessControlPoints()->cbegin();
                i != m_transferFunction.accessControlPoints()->cend();
                ++i)
        {

            cp = *i;

            ImGui::SliderFloat(
                (std::string("position##") + std::to_string(idx)).c_str(),
                &(cp.pos),
                m_xLimitsMin,
                m_xLimitsMax);
            ImGui::SliderFloat(
                (std::string("slope##") + std::to_string(idx)).c_str(),
                &(cp.fderiv),
                -1.f,
                1.f);
            ImGui::ColorEdit3(
                (std::string("assigned color##") + std::to_string(idx)).c_str(),
                glm::value_ptr(cp.color));
            ImGui::SliderFloat(
                (std::string("alpha##") + std::to_string(idx)).c_str(),
                &(cp.color.a),
                0.f,
                1.f);
            ImGui::Spacing();

            if (cp != *i)
            {
                m_transferFunction.updateControlPoint(i, cp);
                m_transferFunction.updateTexture();
            }

            ++idx;
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if(ImGui::Button("save as csv"))
    {
        timer = std::time(nullptr);
        std::time_t t = std::time(nullptr);
        std::tm* tm = std::localtime(&t);

        strftime(
            filename,
            sizeof(filename),
            "./configurations/%F_%H%M%S_transfer-function.csv",
            tm);
        saveTransferFunctionToFile(std::string(filename));
    }
    if ((std::difftime(std::time(nullptr), timer) < 3.f) &&
            (filename[0] != '\0'))
    {
        ImGui::Text("Saved to %s", filename);
    }

    ImGui::End();
}



/**
 * \brief draws the color resulting from the transfer function to an fbo object
 */
void mvr::Renderer::drawTfColor(util::FramebufferObject &tfColorWidgetFBO)
{
    GLint prevFBO = 0;

    // store the previously bound framebuffer
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);

    // activate the framebuffer object as current framebuffer
    glViewport(
        0, 0, m_tfColorWidgetDimensions[0], m_tfColorWidgetDimensions[1]);

    tfColorWidgetFBO.bind();

    // clear old buffer content
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw the resulting transfer function colors
    m_shaderTfColor.use();

    glActiveTexture(GL_TEXTURE0);
    m_transferFunction.accessTexture().bind();
    m_shaderTfColor.setInt("transferTex", 0);

    m_shaderTfColor.setMat4("projMX", m_quadProjMx);
    m_shaderTfColor.setFloat("x_min", m_xLimitsMin);
    m_shaderTfColor.setFloat("x_max", m_xLimitsMax);
    m_shaderTfColor.setFloat("tf_interval_lower",
            m_transferFunction.accessControlPoints()->begin()->pos);
    m_shaderTfColor.setFloat("tf_interval_upper",
            m_transferFunction.accessControlPoints()->rbegin()->pos);

    m_windowQuad.draw();

    // reset framebuffer to the one previously bound
    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
}

/**
 * \brief draws the alpha value from the transfer function to an fbo object
 */
void mvr::Renderer::drawTfFunc(util::FramebufferObject &tfFuncWidgetFBO)
{
    bool enableBlend = false;
    GLint prevFBO = 0;

    // disable alpha blending to prevent discarding of fragments
    if (glIsEnabled(GL_BLEND))
    {
        glDisable(GL_BLEND);
        enableBlend = true;
    }

    // store the previously bound framebuffer
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);

    // activate the framebuffer object as current framebuffer
    glViewport(0, 0, m_tfFuncWidgetDimensions[0], m_tfFuncWidgetDimensions[1]);

    tfFuncWidgetFBO.bind();

    // clear old buffer content
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw the line plot of the transfer function alpha value
    m_shaderTfFunc.use();

    glActiveTexture(GL_TEXTURE0);
    m_transferFunction.accessTexture().bind();
    m_shaderTfFunc.setInt("transferTex", 0);

    m_shaderTfFunc.setMat4("projMX", m_quadProjMx);
    m_shaderTfFunc.setFloat("x_min", m_xLimitsMin);
    m_shaderTfFunc.setFloat("x_max", m_xLimitsMax);
    m_shaderTfFunc.setFloat(
        "tf_interval_lower",
        m_transferFunction.accessControlPoints()->begin()->pos);
    m_shaderTfFunc.setFloat(
        "tf_interval_upper",
        m_transferFunction.accessControlPoints()->rbegin()->pos);
    m_shaderTfFunc.setInt("width", m_tfFuncWidgetDimensions[0]);
    m_shaderTfFunc.setInt("height", m_tfFuncWidgetDimensions[1]);

    m_windowQuad.draw();

    // draw the control points
    glClear(GL_DEPTH_BUFFER_BIT);
    m_shaderTfPoint.use();

    m_shaderTfPoint.setMat4("projMX", m_quadProjMx);
    m_shaderTfPoint.setFloat("x_min", m_xLimitsMin);
    m_shaderTfPoint.setFloat("x_max", m_xLimitsMax);

    for (
            auto i = m_transferFunction.accessControlPoints()->cbegin();
            i != m_transferFunction.accessControlPoints()->cend();
            ++i)
    {
        m_shaderTfPoint.setFloat("pos", i->pos);
        m_shaderTfPoint.setVec4("color", i->color);
        m_tfPoint.draw();
    }

    glBindVertexArray(0);

    if (enableBlend)
        glEnable(GL_BLEND);

    // reset framebuffer to the one previously bound
    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);

}

void mvr::Renderer::loadVolume(
        cr::VolumeConfig volumeConfig, unsigned int timestep)
{
    m_timestep = timestep;
    m_volumeData = cr::loadScalarVolumeTimestep(
        volumeConfig, m_timestep, false);
    m_volumeModelMx = glm::scale(
        glm::mat4(1.f),
        glm::normalize(glm::vec3(
            static_cast<float>(volumeConfig.getVolumeDim()[0]),
            static_cast<float>(volumeConfig.getVolumeDim()[1]),
            static_cast<float>(volumeConfig.getVolumeDim()[2]))));
    m_voxelDiagonal = glm::length(glm::vec3(
        (m_volumeModelMx *
            glm::vec4(
                1.f / static_cast<float>(volumeConfig.getVolumeDim()[0]),
                1.f / static_cast<float>(volumeConfig.getVolumeDim()[1]),
                1.f / static_cast<float>(volumeConfig.getVolumeDim()[2]),
                1.f)).xyz));
    m_histogramBins = bucketVolumeData(
        *m_volumeData, m_binNumberHistogram, m_xLimitsMin, m_xLimitsMax);
    m_volumeTex = cr::loadScalarVolumeTex(*m_volumeData);
    m_boundingBoxMin = m_volumeModelMx * glm::vec4(glm::vec3(-0.5f), 1.f);
    m_boundingBoxMax = m_volumeModelMx * glm::vec4(glm::vec3(0.5f), 1.f);
}
//-----------------------------------------------------------------------------
// helper functions
//-----------------------------------------------------------------------------
int mvr::Renderer::initializeGl3w()
{
    if (gl3wInit())
    {
        std::cerr << "Failed to initialize OpenGL" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }
    if (!gl3wIsSupported(
            REQUIRED_OGL_VERSION_MAJOR, REQUIRED_OGL_VERSION_MINOR))
    {
        std::cerr << "OpenGL " << REQUIRED_OGL_VERSION_MAJOR << "." <<
            REQUIRED_OGL_VERSION_MINOR << " not supported" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "OpenGL " << glGetString(GL_VERSION) << ", GLSL " <<
        glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    return EXIT_SUCCESS;
}

int mvr::Renderer::initializeImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForOpenGL(m_window, false);
    ImGui_ImplOpenGL3_Init();

    ImGui::StyleColorsDark();

    return EXIT_SUCCESS;
}

GLFWwindow* mvr::Renderer::createWindow(
    unsigned int width, unsigned int height, const char* title)
{
    glfwSetErrorCallback(error_cb);
    if (!glfwInit()) exit(EXIT_FAILURE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        width, height, title, nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetWindowUserPointer(window, this);

    // install callbacks
    glfwSetMouseButtonCallback(window, mouseButton_cb);
    glfwSetScrollCallback(window, scroll_cb);
    glfwSetKeyCallback(window, key_cb);
    glfwSetCharCallback(window, char_cb);
    glfwSetCursorPosCallback(window, cursorPosition_cb);
    glfwSetFramebufferSizeCallback(window, framebufferSize_cb);

    return window;
}

void mvr::Renderer::updatePingPongFramebufferObjects()
{
    std::vector<util::texture::Texture2D> fboTexturesPing;
    std::vector<util::texture::Texture2D> fboTexturesPong;

    fboTexturesPing.emplace_back(
            GL_RGBA,
            GL_RGBA,
            0,
            GL_FLOAT,
            GL_LINEAR,
            GL_CLAMP_TO_BORDER,
            m_renderingDimensions[0],
            m_renderingDimensions[1]);

    fboTexturesPing.emplace_back(
            GL_RGBA32UI,
            GL_RGBA_INTEGER,
            0,
            GL_UNSIGNED_INT,
            GL_NEAREST,
            GL_CLAMP_TO_BORDER,
            m_renderingDimensions[0],
            m_renderingDimensions[1]);

    fboTexturesPong.emplace_back(
            GL_RGBA,
            GL_RGBA,
            0,
            GL_FLOAT,
            GL_LINEAR,
            GL_CLAMP_TO_BORDER,
            m_renderingDimensions[0],
            m_renderingDimensions[1]);

    fboTexturesPong.emplace_back(
            GL_RGBA32UI,
            GL_RGBA_INTEGER,
            0,
            GL_UNSIGNED_INT,
            GL_NEAREST,
            GL_CLAMP_TO_BORDER,
            m_renderingDimensions[0],
            m_renderingDimensions[1]);

    const std::vector<GLenum> attachments {
        GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};

    m_framebuffers[0] = util::FramebufferObject(
        std::move(fboTexturesPing), attachments);
    m_framebuffers[1] = util::FramebufferObject(
        std::move(fboTexturesPong), attachments);
}

void::mvr::Renderer::reloadShaders()
{
    std::cout << "Reloading shaders..." << std::endl;
    m_shaderQuad = Shader("src/shader/quad.vert", "src/shader/quad.frag");
    m_shaderFrame = Shader("src/shader/frame.vert", "src/shader/frame.frag");
    m_shaderVolume =
        Shader("src/shader/volume.vert", "src/shader/volume.frag");
    m_shaderTfColor =
        Shader("src/shader/tfColor.vert", "src/shader/tfColor.frag");
    m_shaderTfFunc =
        Shader("src/shader/tfFunc.vert", "src/shader/tfFunc.frag");
    m_shaderTfPoint =
        Shader("src/shader/tfPoint.vert", "src/shader/tfPoint.frag");
}

void mvr::Renderer::resizeRendering(
    int width,
    int height)
{
    m_renderingDimensions[0] = width;
    m_renderingDimensions[1] = height;

    updatePingPongFramebufferObjects();

    m_randomSeedTex = util::texture::create2dHybridTausTexture(
        m_renderingDimensions[0], m_renderingDimensions[1]);
}

// from imgui_demo.cpp
void mvr::Renderer::createHelpMarker(const char* desc)
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

//-----------------------------------------------------------------------------
// glfw callback functions
//-----------------------------------------------------------------------------
void mvr::Renderer::cursorPosition_cb(
        GLFWwindow *window, double xpos, double ypos)
{
    static double xpos_old = 0.0;
    static double ypos_old = 0.0;
    double dx, dy;

    dx = xpos - xpos_old; xpos_old = xpos;
    dy = ypos - ypos_old; ypos_old = ypos;

    mvr::Renderer *pThis =
        reinterpret_cast<mvr::Renderer*>(glfwGetWindowUserPointer(window));

    if (GLFW_PRESS == glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE))
    {
        glm::vec3 polar = util::cartesianToPolar<glm::vec3>(
            pThis->m_cameraPosition);
        float half_pi = glm::half_pi<float>();

        polar.y += glm::radians(dx) * pThis->m_cameraRotationSpeed;
        polar.z += glm::radians(dy) * pThis->m_cameraRotationSpeed;
        if (polar.z <= -0.999f * half_pi)
            polar.z = -0.999f * half_pi;
        else if (polar.z >= 0.999f * half_pi)
            polar.z = 0.999f * half_pi;

        pThis->m_cameraPosition = util::polarToCartesian<glm::vec3>(polar);
    }
    else if (GLFW_PRESS == glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT))
    {
        glm::vec3 horizontal = glm::normalize(
            glm::cross(-(pThis->m_cameraPosition), glm::vec3(0.f, 1.f, 0.f)));
        glm::vec3 vertical = glm::vec3(0.f, 1.f, 0.f);
        pThis->m_cameraPosition +=
            static_cast<float>(
                dx * pThis->m_cameraTranslationSpeed) * horizontal +
            static_cast<float>(
                dy * pThis->m_cameraTranslationSpeed) * vertical;
        pThis->m_cameraLookAt +=
            static_cast<float>(
                dx * pThis->m_cameraTranslationSpeed) * horizontal +
            static_cast<float>(
                dy * pThis->m_cameraTranslationSpeed) * vertical;
    }
}

void mvr::Renderer::mouseButton_cb(
        GLFWwindow* window, int button, int action, int mods)
{
    double mouseX = 0.0, mouseY = 0.0;
    glm::vec2 temp = glm::vec2(0.f);
    GLint prevFBO = 0;

    mvr::Renderer *pThis =
        reinterpret_cast<mvr::Renderer*>(glfwGetWindowUserPointer(window));

    if ((button == GLFW_MOUSE_BUTTON_LEFT) && (action == GLFW_RELEASE))
    {
        if ( pThis->m_showTfWindow )
        {
            // the user might try to select a control point in the transfer
            // function editor
            glfwGetCursorPos(window, &mouseX, &mouseY);

            // store the previously bound framebuffer
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);

            // bind FBO-object and the color-attachment which contains
            // the unique picking ID
            pThis->m_tfFuncWidgetFBO.bindRead(1);

            // send all commands to the GPU and wait until everything is drawn
            glFlush();
            glFinish();

            glReadPixels(
                static_cast<GLint>(mouseX - pThis->m_tfScreenPosition[0]),
                static_cast<GLint>(mouseY - pThis->m_tfScreenPosition[1]),
                1,
                1,
                GL_RG,
                GL_FLOAT,
                glm::value_ptr(temp));

            if (temp.g > 0.f)
                pThis->m_selectedTfControlPointPos = temp.r;

            glBindFramebuffer(GL_READ_FRAMEBUFFER, prevFBO);
        }
    }
    // chain ImGui callback
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
}

void mvr::Renderer::scroll_cb(GLFWwindow *window, double xoffset, double yoffset)

{
    mvr::Renderer *pThis =
        reinterpret_cast<mvr::Renderer*>(glfwGetWindowUserPointer(window));

    if ((GLFW_PRESS == glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)) ||
        (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL)))
    {
        // y scrolling changes the distance of the camera from the origin
        pThis->m_cameraPosition +=
            static_cast<float>(-yoffset) *
            pThis->m_cameraZoomSpeed *
            pThis->m_cameraPosition;
    }

    // chain ImGui callback
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
}

void mvr::Renderer::key_cb(
        GLFWwindow* window, int key, int scancode , int action, int mods)
{
    mvr::Renderer *pThis =
        reinterpret_cast<mvr::Renderer*>(glfwGetWindowUserPointer(window));

    if((key == GLFW_KEY_ESCAPE) && (action == GLFW_PRESS))
        glfwSetWindowShouldClose(window, true);

    if((key == GLFW_KEY_F5) && (action == GLFW_PRESS))
        pThis->reloadShaders();

    if((key == GLFW_KEY_F10) && (action == GLFW_PRESS))
        pThis->m_showMenues = !(pThis->m_showMenues);

    if((key == GLFW_KEY_F9) && (action == GLFW_PRESS))
    {
        std::time_t t = std::time(nullptr);
        std::tm* tm = std::localtime(&t);
        char filename[200];

        strftime(
                filename,
                sizeof(filename),
                "./screenshots/%F_%H%M%S.tiff",
                tm);

        util::makeScreenshot(
            pThis->m_framebuffers[0],
            pThis->m_renderingDimensions[0],
            pThis->m_renderingDimensions[1],
            filename,
            FIF_TIFF);
        std::cout << "Saved screenshot " << filename << std::endl;
    }
    // chain ImGui callback
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
}

void mvr::Renderer::char_cb(GLFWwindow* window, unsigned int c)
{
    // chain ImGui callback
    ImGui_ImplGlfw_CharCallback(window, c);
}

void mvr::Renderer::framebufferSize_cb(
    __attribute__((unused)) GLFWwindow* window,
    int width,
    int height)
{
    mvr::Renderer *pThis =
        reinterpret_cast<mvr::Renderer*>(glfwGetWindowUserPointer(window));

    pThis->m_windowDimensions[0] = width;
    pThis->m_windowDimensions[1] = height;
}

void mvr::Renderer::error_cb(int error, const char* description)
{
    std::cerr << "Glfw error " << error << ": " << description << std::endl;
}

//-----------------------------------------------------------------------------
// C-Interface wrapper
//-----------------------------------------------------------------------------
extern "C"
{
    mvr::Renderer* Renderer_new() { return new mvr::Renderer(); }
    void Renderer_delete(mvr::Renderer* obj) { delete obj; }

    int Renderer_initialize(mvr::Renderer* obj) { return obj->initialize(); }
    int Renderer_run(mvr::Renderer* obj) { return obj->run(); }
    int Renderer_loadConfigFromFile(mvr::Renderer* obj, char* path)
        { return obj->loadConfigFromFile(std::string(path)); }
    int Renderer_renderToFile(mvr::Renderer* obj, char* path)
        { return obj->renderToFile(std::string(path)); }
    int Renderer_saveConfigToFile(mvr::Renderer* obj, char* path)
        { return obj->saveConfigToFile(std::string(path)); }
    int Renderer_loadVolumeFromFile(
            mvr::Renderer* obj, char* path, unsigned int timestep)
        { return obj->loadVolumeFromFile(std::string(path), timestep); }
}

