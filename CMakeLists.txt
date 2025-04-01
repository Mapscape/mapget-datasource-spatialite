# Copyright (c) 2025 NavInfo Europe B.V.

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.28)
cmake_policy(VERSION 3.28)

project(mapget-datasource-spatialite)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)

option(BUILD_TESTS "Build unit tests" YES)
option(NAVINFO_INTERNAL_BUILD "NavInfo internal build" NO)

if(NAVINFO_INTERNAL_BUILD)
    include("navinfo_deps.cmake")
else()
    find_package(libspatialite REQUIRED)
    find_package(Boost REQUIRED COMPONENTS boost program_options)
    find_package(SQLiteCpp REQUIRED)
    find_package(nlohmann_json REQUIRED)

    set(MAPGET_WITH_HTTPLIB ON)
    set(MAPGET_WITH_WHEEL OFF)
    set(SPDLOG_FMT_EXTERNAL ON)

    FetchContent_Declare(mapget
        GIT_REPOSITORY "https://github.com/ndsev/mapget"
        GIT_TAG        "v2024.4.0"
        GIT_SHALLOW    ON
        EXCLUDE_FROM_ALL)
    FetchContent_MakeAvailable(mapget)

    set(CERBERUS_CPP_FIND_YAML_CPP OFF) # mapget is alredy using it

    FetchContent_Declare(cerberus-cpp
        GIT_REPOSITORY "https://github.com/dokempf/cerberus-cpp"
        GIT_TAG        "422b9c98d385891cde85705b311638412a3307d5"
        GIT_SHALLOW    ON
        EXCLUDE_FROM_ALL)
    FetchContent_MakeAvailable(cerberus-cpp)
endif()

set(CONFIG_SCHEMA_HEADER ${CMAKE_BINARY_DIR}/generated/ConfigSchema.h)

add_custom_command(
    OUTPUT ${CONFIG_SCHEMA_HEADER}
    COMMAND python3
    ARGS generate_config_schema_header.py config_schema.yaml ${CONFIG_SCHEMA_HEADER}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS generate_config_schema_header.py config_schema.yaml
)

add_custom_target(generate-config-schema ALL
    DEPENDS ${CONFIG_SCHEMA_HEADER}
)

add_library(generated-files INTERFACE)
target_include_directories(generated-files INTERFACE ${CMAKE_BINARY_DIR}/generated)
add_dependencies(generated-files generate-config-schema)

add_subdirectory(src)

if(BUILD_TESTS)
    if (NOT NAVINFO_INTERNAL_BUILD)
        find_package(GTest REQUIRED)
    endif()

    enable_testing()
    add_subdirectory(test)
endif()
