#pragma once

#include <SDL3/SDL.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>

class OpenGLWidget {
public:
    explicit OpenGLWidget(int width = 640, int height = 480, const char* title = "glhw");
    virtual ~OpenGLWidget();

    OpenGLWidget(const OpenGLWidget&) = delete;
    OpenGLWidget& operator=(const OpenGLWidget&) = delete;

    int run();

    virtual void onInit() = 0;
    virtual void onRender() = 0;
    virtual void onResize(std::size_t width, std::size_t height) = 0;
    virtual void onGui() {}

protected:
    double fps() const noexcept { return fps_; }
    std::filesystem::path assetPath(const std::filesystem::path& relativePath) const;

private:
    void initializeGL();
    void resizeGL(int width, int height);
    void paintGL();
    void cleanup() noexcept;

    SDL_Window* window_ = nullptr;
    SDL_GLContext context_ = nullptr;
    bool initialized_ = false;
    std::filesystem::path assetDirectory_;

    std::chrono::steady_clock::time_point fpsTimer_;
    std::uint64_t frameCount_ = 0;
    double fps_ = 0.0;
};
