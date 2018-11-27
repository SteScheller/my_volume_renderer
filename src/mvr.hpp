#pragma once

#include <array>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>

#include "configraw.hpp"

namespace mvr
{
    /**
     * Basic modes for converting the volume data into color and opacity.
     */
    enum class Mode : int
    {
        line_of_sight = 0,
        maximum_intensity_projection,
        isosurface,
        transfer_function
    };

    /**
     * Gradient calculation method selection for all gradient based operations
     */
    enum class Gradient : int
    {
        central_differences = 0,
        sobel_operators
    };

    /**
     * Selection setting for which output shall be shown. Can be used for
     * debugging purposes.
     */
    enum class Output : int
    {
        volume_rendering = 0,
        random_number_generator,
        volume_data_slice
    };

    /**
     * Selection how mapped volume shall be projected onto the screen.
     */
    enum class Projection : int
    {
        perspective = 0,
        orthographic
    };

    /**
     * \brief volume renderer for dynamic 3D scalar data
     *
     * Class providing volume renderer functionality in form of an interactive
     * gui program or as batch interface.
     */
    class Renderer
    {
        public:

        Renderer();
        ~Renderer();

        int setConfig();
        int run();
        int renderImage();

        //---------------------------------------------------------------------
        // class-wide constants and default values
        //---------------------------------------------------------------------
        static const int REQUIRED_OGL_VERSION_MAJOR = 3;
        static const int REQUIRED_OGL_VERSION_MINOR = 3;

        static const size_t MAX_FILEPATH_LENGTH = 200;

        static const std::string DEFAULT_VOLUME_FILE("example/bucky.json");

        static const glm::vec3 DEFAULT_CAMERA_POSITION(1.2f, 0.75f, 1.f);
        static const glm::vec3 DEFAULT_CAMERA_LOOKAT(0.f);

        private:
        //---------------------------------------------------------------------
        // render settings and interactive program parameters
        //---------------------------------------------------------------------
        // window and gui size
        std::array<unsigned int, 2> m_windowDimensions;
        std::array<unsigned int, 2> m_renderingDimensions;
        std::array<unsigned int, 2> m_tfFuncWidgetDimensions;
        std::array<unsigned int, 2> m_tfColorWidgetDimensions;

        // output mode
        Mode m_renderMode;
        Output m_outputSelect;

        // detailled mode and output settings
        bool m_showVolumeFrame;
        bool m_showWireframe;
        bool m_showDemoWindow;
        bool m_showTfWindow;
        bool m_showHistogramWindow;
        bool m_semilogHistogram;
        int m_binNumberHistogram;
        int m_yLimitHistogramMax;
        float m_xLimitsMin;
        float m_xLimitsMax;
        bool m_invertColors;
        bool m_invertAlpha;
        std::array<float, 3> m_clearColor;

        // data selection
        std::string m_volumeDescriptionFile;
        unsigned int m_timestep;
        float m_outputDataZSlice;

        // ray casting
        float m_stepSize;
        Gradient m_gradientMethod;

        // camera settings
        float m_fovY;
        float m_zNear;
        float m_zFar;
        glm::vec3 m_cameraPosition;
        float m_cameraZoomSpeed;
        float m_cameraRotationSpeed;
        float m_cameraTranslationSpeed;
        Projection m_Projection;

        // isosurface mode
        float m_isovalue;
        bool m_isovalueDenoising;
        float m_isovalueDenoisingRadius;

        // lighting
        float m_brightness;
        std::array<float, 3> m_lightDirection;
        std::array<float, 3> m_ambientColor;
        std::array<float, 3> m_diffuseColor;
        std::array<float, 3> m_specularColor;
        float m_ambientFactor;
        float m_diffuceFactor;
        float m_specularFactor;
        float m_specularExponent;

        // slicing plane functionality
        bool m_slicingPlane;
        std::array<float, 3> m_slicingPlaneNormal;
        std::array<float, 3> m_slicingPlaneBase;

        // ambient occlussion functionality
        bool m_ambientOcclusion;
        float m_ambientOcclusionRadius;
        float m_ambientOcclusionProportion;
        int m_ambientOcclusionNumSamples;

        // temporary control points for transfer function
        //float gui_tf_cp_pos = 0.f;
        //float gui_tf_cp_color_rgb[3] = {0.f, 0.f, 0.0f};
        //float gui_tf_cp_color_a = 0.f;

        //-------------------------------------------------------------------------
        // internals
        //-------------------------------------------------------------------------
        //bool m_isInitialized;
        /*static bool _flag_reload_shaders = false;
        static bool _flag_show_menues = true;

        // default fbo for storing different rendering results and helpers
        static GLuint _ppFBOs[2] = { 0, 0 };
        static GLuint _ppTexIDs[2][2] = {{ 0, 0 }, { 0, 0 }};

        // flag for seeding the random generator in the fragment shader
        static GLuint _rngTex = 0;

        // for picking of control points in transfer function editor
        static float _selected_cp_pos = 0.f;
        static GLuint _selected_cp_fbo = 0.f;
        static ImVec2 _tf_screen_pos = ImVec2();*/

        //---------------------------------------------------------------------
        // subroutines
        //---------------------------------------------------------------------
        void showSettingsWindow(
            cr::VolumeConfig &vConf,
            void *&volumeData,
            GLuint &volumeTex,
            glm::mat4 &modelMX,
            std::vector<util::bin_t> *&histogramBins);

        void showHistogramWindow(
            std::vector<util::bin_t> *&histogramBins,
            cr::VolumeConfig vConf,
            void* volumeData);

        void showTransferFunctionWindow(
            tf::TransferFuncRGBA1D &transferFunction,
            Shader &shaderTfColor,
            Shader &shaderTfFunc,
            Shader &shaderTfPoint,
            GLuint tfColorFBO,
            GLuint *tfColorTexIDs,
            GLuint tfFuncFBO,
            GLuint *tfFuncTexIDs,
            GLuint quadVAO);

        void drawTfColor(
            tf::TransferFuncRGBA1D &transferFunc,
            Shader &shaderTfColor,
            GLuint fboID,
            GLuint quadVAO,
            unsigned int width,
            unsigned int height);

        void drawTfFunc(
            tf::TransferFuncRGBA1D &transferFunc,
            Shader &shaderTfFunc,
            Shader &shaderTfPoint,
            GLuint fboID,
            GLuint quadVAO,
            unsigned int width,
            unsigned int height);

        /**
         * \brief updates the volume data, texture, histogram information...
        */
        void reloadStuff(
            cr::VolumeConfig vConf,
            void *&volumeData,
            GLuint &volumeTex,
            unsigned int timestep,
            glm::mat4 &modelMX,
            std::vector<util::bin_t> *&histogramBins,
            size_t num_bins,
            float x_min,
            float x_max);


        //---------------------------------------------------------------------
        // helper functions
        //---------------------------------------------------------------------
        void initializeGl3w();

        void initializeImGui(GLFWwindow* window);

        GLFWwindow* createWindow(
                unsigned int width,
                unsigned int height,
                const char* title);

        void createPingPongFBO(
            GLuint &fbo,
            GLuint texIDs[2],
            unsigned int width,
            unsigned int height);

        static GLuint createFrameVAO(
            const float vertices[4 * 8],
            const unsigned int indices[2 * 12],
            const float texCoords[3 * 8]);

        static GLuint createVolumeVAO(
            const float vertices[4 * 8],
            const unsigned int indices[3 * 2 * 6],
            const float texCoords[3 * 8]);

        static GLuint createQuadVAO(
                const float vertices[2 * 4],
                const unsigned int indices[4],
                const float texCoords[2 * 4]);

        void resizeRenderResult(int width, int height);

        void createHelpMarker(const char* desc);

        //---------------------------------------------------------------------
        // glfw callback functions
        //---------------------------------------------------------------------
        void cursor_position_cb(
                GLFWwindow *window,
                double xpos,
                double ypos);

        void mouse_button_cb(
                GLFWwindow* window,
                int button,
                int action,
                int mods);

        void scroll_cb(
                GLFWwindow* window,
                double xoffset,
                double yoffset);

        void key_cb(
                GLFWwindow* window,
                int key,
                int scancode,
                int action,
                int mods);

        void char_cb(
                GLFWwindow* window,
                unsigned int c);

        void framebuffer_size_cb(
                GLFWwindow* window,
                int width,
                int height);

        void error_cb(
                int error,
                const char* description);

    };
}

