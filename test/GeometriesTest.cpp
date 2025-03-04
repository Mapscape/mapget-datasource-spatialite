// Copyright (c) 2024 NavInfo Europe B.V.

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "DatabaseTestFixture.h"
#include "FeatureMock.h"

#include <gmock/gmock.h>
#include <boost/algorithm/string/case_conv.hpp>

using SpatialiteDatasource::SpatialIndex;

struct GeometryTestCase
{
    std::vector<std::string> inputGeometries;
    MapgetGeometries expectedGeometries;
};

class SpatialiteDatabaseGeometriesTest
    : public DatabaseTestFixture
    , public testing::WithParamInterface<std::tuple<GeometryTestCase, SpatialIndex>>{};

INSTANTIATE_TEST_SUITE_P(Database, SpatialiteDatabaseGeometriesTest, testing::Combine(
    testing::ValuesIn(std::initializer_list<GeometryTestCase>{
        {
            {
                "POINT(1 2)", 
                "POINT(3 4)"
            },
            {
                {{1, 2}}, 
                {{3, 4}}
            }
        },
        {
            {
                "POINTZ(1 2 3)", 
                "POINTZ(4 5 6)"
            },
            {
                {{1, 2, 3}}, 
                {{4, 5, 6}}
            }
        },
        {
            {
                "LINESTRING(1 2, 3 4, 5 6, 7 8)", 
                "LINESTRING(9 1, 2 3)"
            },
            {
                {{1, 2}, {3, 4}, {5, 6}, {7, 8}}, 
                {{9, 1}, {2, 3}}
            }
        },
        {
            {
                "LINESTRINGZ(1 2 3, 4 5 6, 7 8 9, 10 11 12)", 
                "LINESTRINGZ(3 4 5, 6 7 8)"
            },
            {
                {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10, 11, 12}}, 
                {{3, 4, 5}, {6, 7, 8}}
            }
        },
        {
            {
                "POLYGON((1 2, 3 4, 5 6, 1 2))", 
                "POLYGON((7 8, 9 10, 11 12, 13 14, 7 8))"
            },
            {
                {{1, 2}, {3, 4}, {5, 6}, {1, 2}}, 
                {{7, 8}, {9, 10}, {11, 12}, {13, 14}, {7, 8}}
            }
        },
        {
            {
                "POLYGONZ((1 2 3, 4 5 6, 7 8 9, 1 2 3))", 
                "POLYGONZ((11 12 13, 14 15 16, 17 18 19, 1 2 3, 11 12 13))"
            },
            {
                {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {1, 2, 3}}, 
                {{11, 12, 13}, {14, 15, 16}, {17, 18, 19}, {1, 2, 3}, {11, 12, 13}}
            }
        },
        {
            {
                "MULTIPOINT((1 2), (3 4), (5 6))", 
                "MULTIPOINT((7 8))"
            },
            {
                {{1, 2}}, 
                {{3, 4}}, 
                {{5, 6}}, 
                {{7, 8}}
            }
        },
        {
            {
                "MULTIPOINTZ((1 2 3), (4 5 6), (7 8 9))", 
                "MULTIPOINTZ((11 12 13))"
            },
            {
                {{1, 2, 3}}, 
                {{4, 5, 6}}, 
                {{7, 8, 9}}, 
                {{11, 12, 13}}
            }
        },
        {
            {
                "MULTILINESTRING((1 2, 3 4), (5 6, 7 8), (9 1, 2 3, 4 5))", 
                "MULTILINESTRING((13 14, 15 16))"
            },
            {
                {{1, 2}, {3, 4}},
                {{5, 6}, {7, 8}}, 
                {{9, 1}, {2, 3}, {4, 5}},
                {{13, 14}, {15, 16}}
            }
        },
        {
            {
                "MULTILINESTRINGZ((1 2 3, 3 4 2), (5 6 3, 7 8 4), (9 1 5, 2 3 6, 4 5 7))", 
                "MULTILINESTRINGZ((13 14 8, 16 17 9))"
            },
            {
                {{1, 2, 3}, {3, 4, 2}},
                {{5, 6, 3}, {7, 8, 4}}, 
                {{9, 1, 5}, {2, 3, 6}, {4, 5, 7}},
                {{13, 14, 8}, {16, 17, 9}}
            }
        },
        {
            {
                "MULTIPOLYGON(((1 2, 3 4, 5 6, 1 2)), ((7 8, 9 10, 11 12, 13 14, 7 8)), ((13 14, 15 16, 21 22, 13 14)))", 
                "MULTIPOLYGON(((17 18, 19 20, 1 2, 17 18)))"
            },
            {
                {{1, 2}, {3, 4}, {5, 6}, {1, 2}}, 
                {{7, 8}, {9, 10}, {11, 12}, {13, 14}, {7, 8}},
                {{13, 14}, {15, 16}, {21, 22}, {13, 14}},
                {{17, 18}, {19, 20}, {1, 2}, {17, 18}}
            }
        },
        {
            {
                "MULTIPOLYGONZ(((1 2 3, 3 4 1, 5 6 2, 1 2 3)), ((7 8 1, 9 10 2, 11 12 3, 13 14 4, 7 8 1)), ((13 14 1, 15 16 2, 21 22 3, 13 14 1)))", 
                "MULTIPOLYGONZ(((17 18 5, 19 20 6, 1 2 7, 17 18 5)))"
            },
            {
                {{1, 2, 3}, {3, 4, 1}, {5, 6, 2}, {1, 2, 3}}, 
                {{7, 8, 1}, {9, 10, 2}, {11, 12, 3}, {13, 14, 4}, {7, 8, 1}},
                {{13, 14, 1}, {15, 16, 2}, {21, 22, 3}, {13, 14, 1}},
                {{17, 18, 5}, {19, 20, 6}, {1, 2, 7}, {17, 18, 5}}
            }
        },
    }),
    testing::Values(
        SpatialIndex::None, 
        SpatialIndex::RTree, 
        SpatialIndex::MbrCache
    #if NAVINFO_INTERNAL_BUILD
        , SpatialIndex::NavInfo
    #endif
    )),
    [](const auto& info)
    {
        auto testName = std::get<std::string>(GetGeometryInfoFromGeometry(
            std::get<0>(info.param).inputGeometries[0]));
        boost::to_lower(testName);
        testName[0] = std::toupper(testName[0]);
        testName += SpatialIndexToString(std::get<1>(info.param));
        return testName;
    }
);

TEST_P(SpatialiteDatabaseGeometriesTest, GeometriesAreCreated)
{
    const auto& [geometries, index] = GetParam();
    auto table = InitializeDbWithGeometries(geometries.inputGeometries, index);
    
    const auto [geometryType, dimension, geometryTypeStr] = GetGeometryInfoFromGeometry(geometries.inputGeometries[0]);
    auto resultGeometries = GetGeometries(geometryType, dimension, table);
    FeatureMock featureMock;
    featureMock.AddGeometries(resultGeometries);
    EXPECT_THAT(featureMock.types, testing::Each(geometryType));
    EXPECT_THAT(featureMock.geometries,
                testing::ContainerEq(geometries.expectedGeometries));
    ASSERT_EQ(featureMock.initialCapacities.size(), geometries.expectedGeometries.size());
    EXPECT_THAT(featureMock.initialCapacities,
        testing::ElementsAreArray(std::views::transform(
            geometries.expectedGeometries, [](const auto& v) { return v.size(); })));
}