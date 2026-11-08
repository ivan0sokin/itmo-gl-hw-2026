FetchContent_Declare(glad
    URL https://github.com/Dav1dde/glad/archive/v0.1.36.tar.gz
    URL_HASH SHA256=8470ed1b0e9fbe88e10c34770505c8a1dc8ccb78cadcf673331aaf5224f963d2
)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(Python_EXECUTABLE "${Python3_EXECUTABLE}")
set(GLAD_PROFILE core CACHE STRING "" FORCE)
set(GLAD_API "gl=3.3" CACHE STRING "" FORCE)
set(GLAD_REPRODUCIBLE ON CACHE BOOL "" FORCE)
set(GLAD_INSTALL OFF CACHE BOOL "" FORCE)
set(GLAD_EXPORT OFF CACHE BOOL "" FORCE)

set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
FetchContent_MakeAvailable(glad)
target_link_libraries(glad PRIVATE ${CMAKE_DL_LIBS})
