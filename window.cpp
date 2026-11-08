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

constexpr std::array<glm::vec2, 6> functionVertices = {{
    {-1.f, -1.f},
    {1.f, -1.f},
    {1.f, 1.f},
    {-1.f, -1.f},
    {1.f, 1.f},
    {-1.f, 1.f}
}};

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
    glDeleteProgram(functionsProgram_);
    glDeleteProgram(fractalProgram_);
    glDeleteVertexArrays(1, &functionsVao_);
    glDeleteBuffers(1, &functionsVbo_);
    glDeleteVertexArrays(1, &fractalVao_);
}

void Window::onInit() {
    fractalProgram_ = createProgram(assetPath("shaders/template.vs"), assetPath("shaders/template.fs"));
    fractalUniforms_.iterations = glGetUniformLocation(fractalProgram_, "iterations");
    fractalUniforms_.transforms = glGetUniformLocation(fractalProgram_, "fs[0]");
    fractalUniforms_.cdf = glGetUniformLocation(fractalProgram_, "cdf[0]");
    fractalUniforms_.minXY = glGetUniformLocation(fractalProgram_, "min_xy");
    fractalUniforms_.maxXY = glGetUniformLocation(fractalProgram_, "max_xy");
    functionsProgram_ = createProgram(assetPath("shaders/functions.vs"), assetPath("shaders/functions.fs"));
    functionUniforms_.resolution = glGetUniformLocation(functionsProgram_, "resolution");
    functionUniforms_.clearColor = glGetUniformLocation(functionsProgram_, "clear_color");
    functionUniforms_.viewCenter = glGetUniformLocation(functionsProgram_, "view_center");
    functionUniforms_.viewHalfHeight = glGetUniformLocation(functionsProgram_, "view_half_height");
    functionUniforms_.gridStep = glGetUniformLocation(functionsProgram_, "grid_step");
    functionUniforms_.pixelScale = glGetUniformLocation(functionsProgram_, "pixel_scale");
    functionUniforms_.showGrid = glGetUniformLocation(functionsProgram_, "show_grid");
    functionUniforms_.showSine = glGetUniformLocation(functionsProgram_, "show_sine");
    functionUniforms_.showHyperbola = glGetUniformLocation(functionsProgram_, "show_hyperbola");
    functionUniforms_.period = glGetUniformLocation(functionsProgram_, "period");
    functionUniforms_.hyperbolaCenter = glGetUniformLocation(functionsProgram_, "hyperbola_center");
    functionUniforms_.hyperbolaAxes = glGetUniformLocation(functionsProgram_, "hyperbola_axes");
    glGenVertexArrays(1, &functionsVao_);
    glBindVertexArray(functionsVao_);
    glGenBuffers(1, &functionsVbo_);
    glBindBuffer(GL_ARRAY_BUFFER, functionsVbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(functionVertices), functionVertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenVertexArrays(1, &fractalVao_);
    glEnable(GL_PROGRAM_POINT_SIZE);
}

void Window::onRender() {
    glClearColor(clearColor_[0], clearColor_[1], clearColor_[2], 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    if (scene_ == Scene::Functions) {
        renderFunctions();
    } else {
        renderFractal();
    }
}

void Window::renderFunctions() {
    const auto& io = ImGui::GetIO();
    const float logicalHeight = std::max(io.DisplaySize.y, 1.f);
    const float targetStep = 2.f * functionHalfHeight_ * 80.f / logicalHeight;
    const float magnitude = std::pow(10.f, std::floor(std::log10(targetStep)));
    const float fraction = targetStep / magnitude;
    const float gridStep = magnitude * (fraction <= 1.f ? 1.f : fraction <= 2.f ? 2.f : fraction <= 5.f ? 5.f : 10.f);

    glDisable(GL_BLEND);
    glUseProgram(functionsProgram_);
    glUniform2fv(functionUniforms_.resolution, 1, glm::value_ptr(framebufferSize_));
    glUniform3fv(functionUniforms_.clearColor, 1, clearColor_);
    glUniform2fv(functionUniforms_.viewCenter, 1, glm::value_ptr(functionCenter_));
    glUniform1f(functionUniforms_.viewHalfHeight, functionHalfHeight_);
    glUniform1f(functionUniforms_.gridStep, gridStep);
    glUniform1f(functionUniforms_.pixelScale, framebufferSize_.y / logicalHeight);
    glUniform1i(functionUniforms_.showGrid, showGrid_);
    glUniform1i(functionUniforms_.showSine, showSine_);
    glUniform1i(functionUniforms_.showHyperbola, showHyperbola_);
    glUniform1f(functionUniforms_.period, sinePeriod_);
    glUniform2fv(functionUniforms_.hyperbolaCenter, 1, glm::value_ptr(hyperbolaCenter_));
    glUniform2fv(functionUniforms_.hyperbolaAxes, 1, glm::value_ptr(hyperbolaAxes_));
    glBindVertexArray(functionsVao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glUseProgram(0);
}

void Window::renderFractal() {
    if (minXY_.x >= maxXY_.x || minXY_.y >= maxXY_.y) {
        return;
    }

    std::array<glm::mat3, NUM_TRANSFORMS> matrices;
    std::array<float, NUM_TRANSFORMS> cdf;
    float totalProbability = 0.0f;
    for (int i = 0; i < NUM_TRANSFORMS; ++i) {
        const auto& f = transforms()[i];
        matrices[i] = glm::mat3(f.a, f.c, 0.f, f.b, f.d, 0.f, f.e, f.g, 1.0f);
        totalProbability += f.p;
        cdf[i] = totalProbability;
    }
    for (auto& probability : cdf) {
        probability /= totalProbability;
    }

    glUseProgram(fractalProgram_);
    glUniform1i(fractalUniforms_.iterations, iterations_);
    glUniformMatrix3fv(fractalUniforms_.transforms, NUM_TRANSFORMS, GL_FALSE, glm::value_ptr(matrices[0]));
    glUniform1fv(fractalUniforms_.cdf, NUM_TRANSFORMS, cdf.data());
    glUniform2fv(fractalUniforms_.minXY, 1, glm::value_ptr(minXY_));
    glUniform2fv(fractalUniforms_.maxXY, 1, glm::value_ptr(maxXY_));
    glBindVertexArray(fractalVao_);
    glDrawArrays(GL_POINTS, 0, pointCount_);

    glBindVertexArray(0);
    glUseProgram(0);
}

void Window::onResize(std::size_t width, std::size_t height) {
    width = std::max(width, std::size_t{1});
    height = std::max(height, std::size_t{1});
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    framebufferSize_ = glm::vec2(static_cast<float>(width), static_cast<float>(height));
}

void Window::handleFunctionInput() {
    const auto& io = ImGui::GetIO();
    if (scene_ != Scene::Functions || io.DisplaySize.x <= 0.f || io.DisplaySize.y <= 0.f) {
        functionDragging_ = false;
        return;
    }

    const auto* viewport = ImGui::GetMainViewport();
    const glm::vec2 mouse(io.MousePos.x - viewport->Pos.x, io.MousePos.y - viewport->Pos.y);
    const bool overPlot = ImGui::IsMousePosValid() && mouse.x >= 0.f && mouse.x < io.DisplaySize.x &&
        mouse.y >= 0.f && mouse.y < io.DisplaySize.y && !io.WantCaptureMouse;
    const bool clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        functionDragging_ = false;
    } else if (clicked && overPlot) {
        functionDragging_ = true;
    }

    const float aspect = framebufferSize_.x / framebufferSize_.y;
    if (functionDragging_ && !clicked) {
        functionCenter_ += glm::vec2(-io.MouseDelta.x * aspect / io.DisplaySize.x,
            io.MouseDelta.y / io.DisplaySize.y) * (2.f * functionHalfHeight_);
    }
    if (overPlot && io.MouseWheel != 0.f) {
        const glm::vec2 cursor(2.f * mouse.x / io.DisplaySize.x - 1.f,
            1.f - 2.f * mouse.y / io.DisplaySize.y);
        const glm::vec2 anchor = functionCenter_ + cursor * glm::vec2(aspect, 1.f) * functionHalfHeight_;
        functionHalfHeight_ = std::clamp(functionHalfHeight_ * std::pow(1.02f, -io.MouseWheel), 0.01f, 10000.f);
        functionCenter_ = anchor - cursor * glm::vec2(aspect, 1.f) * functionHalfHeight_;
    }
}

void Window::onGui() {
    ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("hw01", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("fps: %.0f", fps());
        ImGui::ColorEdit3("clear color", clearColor_);
        if (ImGui::RadioButton("functions", scene_ == Scene::Functions)) {
            scene_ = Scene::Functions;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("fractal", scene_ == Scene::Fractal)) {
            scene_ = Scene::Fractal;
        }
        if (ImGui::CollapsingHeader("fuctions", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("grid", &showGrid_);
            ImGui::Checkbox("sine", &showSine_);
            ImGui::TextColored(ImVec4(1.f, 0.65f, 0.2f, 1.f), "y = sin(2 * pi * x / T)");
            ImGui::DragFloat("period(T)", &sinePeriod_, 0.01f, 0.01f, 10000.f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::Checkbox("hyperbola", &showHyperbola_);
            ImGui::TextColored(ImVec4(0.2f, 0.75f, 1.f, 1.f), "(x - x0)^2 / a^2 - (y - y0)^2 / b^2 = 1");
            ImGui::DragFloat2("center(x0, y0)", glm::value_ptr(hyperbolaCenter_), 0.01f);
            ImGui::DragFloat("horizontal scale(a)", &hyperbolaAxes_.x, 0.01f, 0.01f, 10000.f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::DragFloat("vertical scale(b)", &hyperbolaAxes_.y, 0.01f, 0.01f, 10000.f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            if (ImGui::Button("reset function view")) {
                functionCenter_ = glm::vec2(0.f);
                functionHalfHeight_ = 5.f;
            }
            ImGui::TextUnformatted("zoom: wheel");
            ImGui::TextUnformatted("mouse left: drag");
        }
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
    handleFunctionInput();
}
