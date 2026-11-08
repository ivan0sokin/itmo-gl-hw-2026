#include "window.h"

#include <SDL3/SDL_main.h>

#include <exception>
#include <iostream>

int main(int, char**) {
    try {
        Window window;
        return window.run();
    } catch (const std::exception& error) {
        std::cerr << "application error: " << error.what() << '\n';
        return 1;
    }
}
