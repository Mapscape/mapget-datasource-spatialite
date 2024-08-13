# Copyright (c) 2024 NavInfo Europe B.V.

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

cmake_minimum_required(VERSION 3.25)
cmake_policy(VERSION 3.25)

project(mapget-datasource-spatialite)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)

option(BUILD_TESTS "Build unit tests" YES)
option(NAVINFO_INTERNAL_BUILD "NavInfo internal build" NO)

if(NAVINFO_INTERNAL_BUILD)
    include("navinfo_deps.cmake")
else()
    find_package(libspatialite REQUIRED)
endif()

find_package(Boost REQUIRED COMPONENTS boost program_options)
find_package(SQLiteCpp REQUIRED)
find_package(GTest REQUIRED)

set(MAPGET_WITH_HTTPLIB ON)
set(MAPGET_WITH_WHEEL OFF)
set(SPDLOG_FMT_EXTERNAL ON)

FetchContent_Declare(mapget
    GIT_REPOSITORY "https://github.com/ndsev/mapget"
    GIT_TAG        "v2024.3.1"
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(mapget)

add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
