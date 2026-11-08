#pragma once

#include "openglwidget.h"

#include <glad/glad.h>
#include <glm/mat4x4.hpp>

#include <chrono>
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
    std::chrono::steady_clock::time_point timer_;
    GLint colorUniform_ = -1;
    float hue_ = 0.0f;

    GLint mvpUniform_ = -1;

    glm::mat4 model_{1.0f};
    glm::mat4 view_{1.0f};
    glm::mat4 projection_{1.0f};

    GLuint vbo_ = 0;
    GLuint ibo_ = 0;
    GLuint vao_ = 0;

    GLuint texture_ = 0;
    GLuint program_ = 0;

    bool animated_ = true;
    bool colorAnimated_ = true;
};
