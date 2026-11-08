#include "window.h"

#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

namespace {

constexpr std::array<GLfloat, 21u> vertices = {
    0.0f, 0.5f, 1.f,  0.f,  0.f,   0.5f, 1.0f, -0.5f, -0.5f, 0.f,  1.f,
    0.f,  0.0f, 0.0f, 0.5f, -0.5f, 0.f,  0.f,  1.f,   1.0f,  0.0f,
};

constexpr std::array<GLuint, 3u> indices = {0, 1, 2};

std::string readFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("cannot open asset: " + path.string());
    }

    const std::string contents{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    
    if (file.bad()) {
        throw std::runtime_error("cannot read asset: " + path.string());
    }
    
    return contents;
}

GLuint compileShader(GLenum type, const std::filesystem::path& path) {
    const auto source = readFile(path);
    const auto* sourceData = source.c_str();
    const GLuint shader = glCreateShader(type);
    if (shader == 0) {
        throw std::runtime_error("cannot create shader: " + path.string());
    }
    glShaderSource(shader, 1, &sourceData, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::string log(static_cast<std::size_t>(std::max(length, 1)), '\0');
        glGetShaderInfoLog(shader, length, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error("shader compilation failed (" + path.string() + "): " + log);
    }
    return shader;
}

GLuint createProgram(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath) {
    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexPath);
    GLuint fragmentShader = 0;
    GLuint program = 0;
    try {
        fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentPath);
        program = glCreateProgram();
        if (program == 0) {
            throw std::runtime_error("cannot create shader program");
        }
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        GLint linked = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (linked != GL_TRUE) {
            GLint length = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
            std::string log(static_cast<std::size_t>(length), '\0');
            glGetProgramInfoLog(program, length, nullptr, log.data());
            throw std::runtime_error("shader program linking failed: " + log);
        }
    } catch (...) {
        glDeleteProgram(program);
        glDeleteShader(fragmentShader);
        glDeleteShader(vertexShader);
        throw;
    }

    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

glm::vec3 hueToRgb(float hue) {
    const auto channels = glm::fract(glm::vec3(hue) + glm::vec3(0.0f, 2.0f / 3.0f, 1.0f / 3.0f));
    return glm::clamp(glm::abs(channels * 6.0f - 3.0f) - 1.0f, 0.0f, 1.0f);
}

}

Window::~Window() {
    glDeleteTextures(1, &texture_);
    glDeleteProgram(program_);
    glDeleteBuffers(1, &vbo_);
    glDeleteBuffers(1, &ibo_);
    glDeleteVertexArrays(1, &vao_);
}

void Window::onInit() {
    constexpr const char* VERTEX_SHADER_PATH = "shaders/template.vs";
    constexpr const char* FRAGMENT_SHADER_PATH = "shaders/template.fs";
    constexpr const char* TEXTURE_PATH = "textures/cat.jpg";

    program_ = createProgram(assetPath(VERTEX_SHADER_PATH), assetPath(FRAGMENT_SHADER_PATH));
    glUseProgram(program_);

    const auto texturePath = assetPath(TEXTURE_PATH);
    const auto imageBytes = readFile(texturePath);
    int width = 0;
    int height = 0;
    
    std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels(
        stbi_load_from_memory(
            reinterpret_cast<const stbi_uc*>(imageBytes.data()),
            static_cast<int>(imageBytes.size()),
            &width,
            &height,
            nullptr,
            STBI_rgb_alpha
        ),
        stbi_image_free
    );

    if (!pixels) {
        const char* reason = stbi_failure_reason();
        throw std::runtime_error("cannot decode texture (" + texturePath.string() + "): " + (reason ? reason : "unknown image error"));
    }

    glGenTextures(1, &texture_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.get());
    glUniform1i(glGetUniformLocation(program_, "tex_2d"), 0);

    mvpUniform_ = glGetUniformLocation(program_, "mvp");
    colorUniform_ = glGetUniformLocation(program_, "color");
    glUniform4f(colorUniform_, 1.0f, 1.0f, 1.0f, 1.0f);

    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &ibo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices.data(), GL_STATIC_DRAW);

    constexpr GLsizei stride = 7 * sizeof(GLfloat);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const void*>(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const void*>(5 * sizeof(GLfloat)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    timer_ = std::chrono::steady_clock::now();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void Window::onRender() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    model_ = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -1.5f));
    view_ = glm::mat4(1.0f);
    const auto mvp = projection_ * view_ * model_;

    glUseProgram(program_);
    glUniformMatrix4fv(mvpUniform_, 1, GL_FALSE, glm::value_ptr(mvp));

    const auto now = std::chrono::steady_clock::now();
    const float dt = std::chrono::duration<float>(now - timer_).count();
    timer_ = now;
    if (animated_ && colorAnimated_) {
        hue_ = std::fmod(hue_ + dt * (60.0f / 360.0f), 1.0f);
        const auto color = hueToRgb(hue_);
        glUniform4f(colorUniform_, color.r, color.g, color.b, 1.0f);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Window::onResize(std::size_t width, std::size_t height) {
    width = std::max(width, std::size_t{1});
    height = std::max(height, std::size_t{1});
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));

    const auto aspect = static_cast<float>(width) / static_cast<float>(height);
    projection_ = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);
}

void Window::onGui() {
    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Homework", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("FPS: %.0f", fps());
        ImGui::Checkbox("Animate", &animated_);
        ImGui::BeginDisabled(!animated_);
        ImGui::Checkbox("Animate color", &colorAnimated_);
        ImGui::EndDisabled();
    }
    ImGui::End();
}
