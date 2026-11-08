#include "openglwidget.h"

#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

#include <algorithm>
#include <stdexcept>
#include <string>

OpenGLWidget::OpenGLWidget(int width, int height, const char* title) {
    constexpr int OPENGL_MAJOR_VERSION = 3;
    constexpr int OPENGL_MINOR_VERSION = 3;
    constexpr int OPENGL_DOUBLEBUFFER = 1;
    constexpr int OPENGL_DEPTH_SIZE_BITS = 24;
    constexpr int OPENGL_DEPTH_STENCIL_SIZE_BITS = 8;
    
    try {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            throw std::runtime_error(std::string("SDL_Init: ") + SDL_GetError());
        }

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, OPENGL_MAJOR_VERSION);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, OPENGL_MINOR_VERSION);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef __APPLE__
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, OPENGL_DOUBLEBUFFER);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, OPENGL_DEPTH_SIZE_BITS);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, OPENGL_DEPTH_STENCIL_SIZE_BITS);

        const float contentScale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
        window_ = SDL_CreateWindow(
            title,
            static_cast<int>(width * contentScale),
            static_cast<int>(height * contentScale),
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN
        );

        if (!window_) {
            throw std::runtime_error(std::string("SDL_CreateWindow: ") + SDL_GetError());
        }

        context_ = SDL_GL_CreateContext(window_);
        if (!context_) {
            throw std::runtime_error(std::string("SDL_GL_CreateContext: ") + SDL_GetError());
        }

        if (!SDL_GL_MakeCurrent(window_, context_)) {
            throw std::runtime_error(std::string("SDL_GL_MakeCurrent: ") + SDL_GetError());
        }

        if (!gladLoadGLLoader([](const char* name) -> void* {
                return reinterpret_cast<void*>(SDL_GL_GetProcAddress(name));
            }) || !GLAD_GL_VERSION_3_3) {
            throw std::runtime_error("cannot load OpenGL " + std::to_string(OPENGL_MAJOR_VERSION) + "." + std::to_string(OPENGL_MINOR_VERSION));
        }

        SDL_GL_SetSwapInterval(1);
        SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        if (!SDL_ShowWindow(window_)) {
            throw std::runtime_error(std::string("SDL_ShowWindow: ") + SDL_GetError());
        }

        const char* basePath = SDL_GetBasePath();
        if (!basePath) {
            throw std::runtime_error(std::string("SDL_GetBasePath: ") + SDL_GetError());
        }
        assetDirectory_ = std::filesystem::path(basePath);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = nullptr;
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(contentScale);
        style.FontScaleDpi = contentScale;
        if (!ImGui_ImplSDL3_InitForOpenGL(window_, context_)) {
            throw std::runtime_error("cannot initialize the ImGui SDL3 backend");
        }
        if (!ImGui_ImplOpenGL3_Init("#version 330 core")) {
            throw std::runtime_error("cannot initialize the ImGui OpenGL backend");
        }
    } catch (...) {
        cleanup();
        throw;
    }
}

OpenGLWidget::~OpenGLWidget() {
    cleanup();
}

int OpenGLWidget::run() {
    initialized_ = true;
    initializeGL();

    int previousWidth = -1;
    int previousHeight = -1;
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT ||
                (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                 event.window.windowID == SDL_GetWindowID(window_))) {
                running = false;
            }
        }
        if (!running) {
            break;
        }

        int width = 0;
        int height = 0;
        if (!SDL_GetWindowSizeInPixels(window_, &width, &height)) {
            throw std::runtime_error(std::string("SDL_GetWindowSizeInPixels: ") + SDL_GetError());
        }
        if (width != previousWidth || height != previousHeight) {
            resizeGL(width, height);
            previousWidth = width;
            previousHeight = height;
        }
        if (width <= 0 || height <= 0 ||
            (SDL_GetWindowFlags(window_) & SDL_WINDOW_MINIMIZED)) {
            SDL_Delay(10);
            continue;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        onGui();
        ImGui::Render();
        paintGL();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (!SDL_GL_SwapWindow(window_)) {
            throw std::runtime_error(std::string("SDL_GL_SwapWindow: ") + SDL_GetError());
        }
    }
    return 0;
}

void OpenGLWidget::initializeGL() {
    onInit();
    fpsTimer_ = std::chrono::steady_clock::now();
}

void OpenGLWidget::resizeGL(int width, int height) {
    onResize(static_cast<std::size_t>(width), static_cast<std::size_t>(height));
}

void OpenGLWidget::paintGL() {
    onRender();
    ++frameCount_;

    const auto now = std::chrono::steady_clock::now();
    const double elapsedSeconds = std::chrono::duration<double>(now - fpsTimer_).count();
    if (elapsedSeconds >= 1.0) {
        fps_ = static_cast<double>(frameCount_) / elapsedSeconds;
        fpsTimer_ = now;
        frameCount_ = 0;
    }
}

std::filesystem::path OpenGLWidget::assetPath(const std::filesystem::path& relativePath) const {
    return assetDirectory_ / relativePath;
}

void OpenGLWidget::cleanup() noexcept {
    if (ImGui::GetCurrentContext()) {
        const ImGuiIO& io = ImGui::GetIO();
        if (io.BackendRendererUserData) {
            ImGui_ImplOpenGL3_Shutdown();
        }
        if (io.BackendPlatformUserData) {
            ImGui_ImplSDL3_Shutdown();
        }
        ImGui::DestroyContext();
    }
    if (context_) {
        SDL_GL_DestroyContext(context_);
    }
    if (window_) {
        SDL_DestroyWindow(window_);
    }
    SDL_Quit();
}
