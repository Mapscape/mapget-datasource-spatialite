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

add_executable(unit-test
    main.cpp
    AttributesTest.cpp
    DatabaseTest.h
    DatabaseTest.cpp
    FeatureMock.h
    GeometriesTest.cpp
    TestDbDriver.h
    TestDbDriver.cpp
    $<IF:$<BOOL:${NAVINFO_INTERNAL_BUILD}>,NavInfoIndex.cpp,NavInfoIndexDummy.cpp>
)

target_link_libraries(unit-test
    ${PROJECT_NAME}-lib
    gtest::gtest
    
    ${NAVINFO_INDEX_LIBS}
)

add_test(NAME mapget-datasource-spatialite-test COMMAND unit-test)
