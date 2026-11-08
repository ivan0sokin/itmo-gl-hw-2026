FetchContent_Declare(glm
    URL https://github.com/g-truc/glm/releases/download/1.0.2/glm-1.0.2.zip
    URL_HASH SHA256=77fff075c5ab0caf076fade41558c5458d9a242aba2b2076bb72008bdb51a35e
)

set(GLM_BUILD_LIBRARY OFF CACHE BOOL "Use header-only GLM" FORCE)
set(GLM_BUILD_TESTS OFF CACHE BOOL "Build GLM's tests" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "Install GLM" FORCE)

FetchContent_MakeAvailable(glm)
