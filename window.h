#pragma once

#include "openglwidget.h"

#include <glad/glad.h>
#include <glm/vec2.hpp>

#include <array>
#include <cstddef>

class Window : public OpenGLWidget {
public:
    Window() = default;
    ~Window() override;

    void onInit() override;
    void onRender() override;
    void onResize(std::size_t width, std::size_t height) override;
    void onGui() override;

private:
    static constexpr int NUM_TRANSFORMS = 4;

    struct Transform {
        float a;
        float b;
        float c;
        float d;
        float e;
        float g;
        float p;
    };

    static constexpr std::array<Transform, NUM_TRANSFORMS> defaultFern_ = {{
        {0.f, 0.f, 0.f, 0.16f, 0.f, 0.f, 0.01f},
        {0.85f, 0.04f, -0.04f, 0.85f, 0.f, 1.60f, 0.85f},
        {0.20f, -0.26f, 0.23f, 0.22f, 0.f, 1.60f, 0.07f},
        {-0.15f, 0.28f, 0.26f, 0.24f, 0.f, 0.44f, 0.07f}
    }};

    static constexpr std::array<Transform, NUM_TRANSFORMS> mutantFern_ = {{
        {0.f, 0.f, 0.f, 0.25f, 0.f, -0.4f, 0.02f},
        {0.95f, 0.005f, -0.005f, 0.93f, -0.002f, 0.5f, 0.84f},
        {0.035f, -0.2f, 0.16f, 0.04f, -0.09f, 0.02f, 0.07f},
        {-0.04f, 0.2f, 0.16f, 0.04f, 0.083f, 0.12f, 0.07f}
    }};

    const std::array<Transform, NUM_TRANSFORMS>& transforms() const;

    void renderFunctions();
    void renderFractal();
    void handleFunctionInput();

    struct FunctionUniforms {
        GLint resolution = -1;
        GLint clearColor = -1;
        GLint viewCenter = -1;
        GLint viewHalfHeight = -1;
        GLint gridStep = -1;
        GLint pixelScale = -1;
        GLint showGrid = -1;
        GLint showSine = -1;
        GLint showHyperbola = -1;
        GLint period = -1;
        GLint hyperbolaCenter = -1;
        GLint hyperbolaAxes = -1;
    };

    struct FractalUniforms {
        GLint iterations = -1;
        GLint transforms = -1;
        GLint cdf = -1;
        GLint minXY = -1;
        GLint maxXY = -1;
    };

    enum class Scene {
        Functions,
        Fractal
    };

    Scene scene_ = Scene::Functions;
    bool showSine_ = true;
    bool showHyperbola_ = true;
    bool showGrid_ = true;
    bool functionDragging_ = false;
    float sinePeriod_ = 4.f;
    glm::vec2 hyperbolaCenter_{0.f, 0.f};
    glm::vec2 hyperbolaAxes_{1.f, 1.f};
    glm::vec2 functionCenter_{0.f, 0.f};
    float functionHalfHeight_ = 5.f;
    glm::vec2 framebufferSize_{640.f, 480.f};
    FunctionUniforms functionUniforms_;
    GLuint functionsProgram_ = 0;
    GLuint functionsVao_ = 0;
    GLuint functionsVbo_ = 0;

    int preset_ = 0;
    std::array<Transform, NUM_TRANSFORMS> customTransforms_ = defaultFern_;
    int iterations_ = 100;
    int pointCount_ = 100000;
    float clearColor_[3] = {0.f, 0.f, 0.f};
    glm::vec2 minXY_{-3.f, 0.f};
    glm::vec2 maxXY_{3.f, 10.f};
    FractalUniforms fractalUniforms_;
    GLuint fractalVao_ = 0;
    GLuint fractalProgram_ = 0;
};
