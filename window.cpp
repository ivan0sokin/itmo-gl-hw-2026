#include "window.h"

#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat3x3.hpp>
#include <imgui.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>

namespace {

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

}

const std::array<Window::Transform, Window::NUM_TRANSFORMS>& Window::transforms() const {
    if (preset_ == 0) {
        return defaultFern_;
    }
    if (preset_ == 1) {
        return mutantFern_;
    }
    return customTransforms_;
}

Window::~Window() {
    glDeleteProgram(program_);
    glDeleteVertexArrays(1, &vao_);
}

void Window::onInit() {
    program_ = createProgram(assetPath("shaders/template.vs"), assetPath("shaders/template.fs"));
    iterationsUniform_ = glGetUniformLocation(program_, "iterations");
    transformsUniform_ = glGetUniformLocation(program_, "fs[0]");
    cdfUniform_ = glGetUniformLocation(program_, "cdf[0]");
    minXYUniform_ = glGetUniformLocation(program_, "min_xy");
    maxXYUniform_ = glGetUniformLocation(program_, "max_xy");
    glGenVertexArrays(1, &vao_);
    glEnable(GL_PROGRAM_POINT_SIZE);
}

void Window::onRender() {
    glClearColor(clearColor_[0], clearColor_[1], clearColor_[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    if (minXY_.x >= maxXY_.x || minXY_.y >= maxXY_.y) {
        return;
    }

    std::array<glm::mat3, NUM_TRANSFORMS> matrices;
    std::array<float, NUM_TRANSFORMS> cdf;
    float totalProbability = 0.0f;
    for (int i = 0; i < NUM_TRANSFORMS; ++i) {
        const auto& f = transforms()[i];
        matrices[i] = glm::mat3(f.a, f.c, 0.0f, f.b, f.d, 0.0f, f.e, f.g, 1.0f);
        totalProbability += f.p;
        cdf[i] = totalProbability;
    }
    if (totalProbability <= 0.0f) {
        return;
    }
    for (auto& probability : cdf) {
        probability /= totalProbability;
    }

    glUseProgram(program_);
    glUniform1i(iterationsUniform_, iterations_);
    glUniformMatrix3fv(transformsUniform_, NUM_TRANSFORMS, GL_FALSE, glm::value_ptr(matrices[0]));
    glUniform1fv(cdfUniform_, NUM_TRANSFORMS, cdf.data());
    glUniform2fv(minXYUniform_, 1, glm::value_ptr(minXY_));
    glUniform2fv(maxXYUniform_, 1, glm::value_ptr(maxXY_));
    glBindVertexArray(vao_);
    glDrawArrays(GL_POINTS, 0, pointCount_);

    glBindVertexArray(0);
    glUseProgram(0);
}

void Window::onResize(std::size_t width, std::size_t height) {
    width = std::max(width, std::size_t{1});
    height = std::max(height, std::size_t{1});
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
}

void Window::onGui() {
    ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("hw01", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("fps: %.0f", fps());
        ImGui::ColorEdit3("clear color", clearColor_);
        if (ImGui::CollapsingHeader("barnsley fern", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderInt("iterations", &iterations_, 1, 1000, "%d", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderInt("points per call", &pointCount_, 1, 1000000, "%d", ImGuiSliderFlags_AlwaysClamp);
            ImGui::DragFloat2("min XY", glm::value_ptr(minXY_), 0.01f);
            ImGui::DragFloat2("max XY", glm::value_ptr(maxXY_), 0.01f);
            if (minXY_.x >= maxXY_.x || minXY_.y >= maxXY_.y) {
                ImGui::TextUnformatted("max XY must be greater than min XY on both axes");
            }
            ImGui::Combo("preset", &preset_, "default\0mutant\0custom\0");

            auto editedTransforms = transforms();
            bool changed = false;
            constexpr std::array<const char*, 7> columns = {"a", "b", "c", "d", "e", "g", "p"};
            if (ImGui::BeginTable("Transforms", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
                ImGui::TableSetupColumn("w", ImGuiTableColumnFlags_WidthFixed, 24.0f);
                for (const auto* column : columns) {
                    ImGui::TableSetupColumn(column, ImGuiTableColumnFlags_WidthFixed, 62.0f);
                }
                ImGui::TableHeadersRow();
                for (int i = 0; i < NUM_TRANSFORMS; ++i) {
                    ImGui::PushID(i);
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("f%d", i + 1);
                    auto& f = editedTransforms[i];
                    const std::array<float*, 7> coefs = {&f.a, &f.b, &f.c, &f.d, &f.e, &f.g, &f.p};
                    for (int column = 0; column < 7; ++column) {
                        ImGui::TableSetColumnIndex(column + 1);
                        ImGui::PushID(column);
                        ImGui::SetNextItemWidth(-1.0f);
                        changed |= ImGui::DragFloat(
                            "##value",
                            coefs[column],
                            0.001f,
                            0.0f,
                            column == 6 ? 1.0f : 0.0f,
                            "%.3f",
                            column == 6 ? ImGuiSliderFlags_AlwaysClamp : ImGuiSliderFlags_None
                        );
                        ImGui::PopID();
                    }
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
            if (changed) {
                customTransforms_ = editedTransforms;
                preset_ = 2;
            }
            ImGui::TextUnformatted("probabilities are normalized automatically");
            if (std::all_of(transforms().begin(), transforms().end(), [](const auto& f) { return f.p == 0.0f; })) {
                ImGui::TextUnformatted("set at least one probability above zero");
            }
        }
    }
    ImGui::End();
}
