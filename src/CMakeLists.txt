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

add_compile_definitions(NAVINFO_INTERNAL_BUILD=$<BOOL:${NAVINFO_INTERNAL_BUILD}>)

add_library(${PROJECT_NAME}-lib STATIC
    TableInfo.h
    TableInfo.cpp
    ConfigLoader.h
    ConfigLoader.cpp
    Datasource.h
    Datasource.cpp
    Database.h
    Database.cpp
    GeometriesView.h
    GeometriesView.cpp
    GeometryType.h
    IFeature.h
    MapgetFeature.h
    SqlStatements.h
    SqlStatements.cpp
    NavInfoIndex.h
    $<IF:$<BOOL:${NAVINFO_INTERNAL_BUILD}>,NavInfoIndex.cpp,NavInfoIndexDummy.cpp>
)

target_include_directories(${PROJECT_NAME}-lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(${PROJECT_NAME}-lib
    libspatialite::libspatialite
    boost::boost
    Boost::program_options
    fmt::fmt
    nlohmann_json::nlohmann_json
    SQLite::SQLite3
    SQLiteCpp
    mapget-log
    mapget-model
    mapget-http-datasource
    yaml-cpp
    cerberus-cpp
    generated-files
)

add_executable(${PROJECT_NAME}
    main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}-lib)
install(TARGETS ${PROJECT_NAME})
