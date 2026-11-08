FetchContent_Declare(stb
    URL https://github.com/nothings/stb/archive/a7d4c2175630e1ea5b6f8dd00ab96fed0b97b96d.zip
    URL_HASH SHA256=fc6e7c840a0e025ee238fc09e136452614a4b765afa6ff4177d2e9e1b3493675
)
FetchContent_MakeAvailable(stb)

add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE "${stb_SOURCE_DIR}")
