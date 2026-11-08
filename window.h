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
        {0.0f, 0.0f, 0.0f, 0.16f, 0.0f, 0.0f, 0.01f},
        {0.85f, 0.04f, -0.04f, 0.85f, 0.0f, 1.60f, 0.85f},
        {0.20f, -0.26f, 0.23f, 0.22f, 0.0f, 1.60f, 0.07f},
        {-0.15f, 0.28f, 0.26f, 0.24f, 0.0f, 0.44f, 0.07f}
    }};

    static constexpr std::array<Transform, NUM_TRANSFORMS> mutantFern_ = {{
        {0.0f, 0.0f, 0.0f, 0.25f, 0.0f, -0.4f, 0.02f},
        {0.95f, 0.005f, -0.005f, 0.93f, -0.002f, 0.5f, 0.84f},
        {0.035f, -0.2f, 0.16f, 0.04f, -0.09f, 0.02f, 0.07f},
        {-0.04f, 0.2f, 0.16f, 0.04f, 0.083f, 0.12f, 0.07f}
    }};

    const std::array<Transform, NUM_TRANSFORMS>& transforms() const;

    int preset_ = 0;
    std::array<Transform, NUM_TRANSFORMS> customTransforms_ = defaultFern_;
    int iterations_ = 100;
    int pointCount_ = 100000;
    float clearColor_[3] = {0.0f, 0.0f, 0.0f};
    glm::vec2 minXY_{-3.0f, -1.0f};
    glm::vec2 maxXY_{3.0f, 10.0f};
    GLint iterationsUniform_ = -1;
    GLint transformsUniform_ = -1;
    GLint cdfUniform_ = -1;
    GLint minXYUniform_ = -1;
    GLint maxXYUniform_ = -1;
    GLuint vao_ = 0;
    GLuint program_ = 0;
};
