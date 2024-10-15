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

#include "DatabaseTest.h"
#include "FeatureMock.h"

#include <gmock/gmock.h>

#define CHECK_GEOMETRIES(table, type, dimension, ...)        \
    auto geometries = GetGeometries<type, dimension>(table); \
    FeatureMock featureMock;                                 \
    for (auto g : geometries)                                \
    {                                                        \
        g.AddTo(featureMock);                                \
    }                                                        \
    EXPECT_THAT(featureMock.types, testing::Each(type));     \
    const Geometries expectedGeometries{__VA_ARGS__};        \
    EXPECT_THAT(featureMock.geometries,                      \
                testing::ContainerEq(expectedGeometries));   \
    ASSERT_EQ(featureMock.initialCapacities.size(),          \
                expectedGeometries.size());                  \
    EXPECT_THAT(                                             \
        featureMock.initialCapacities,                       \
        testing::ElementsAreArray(std::views::transform(     \
            expectedGeometries, [](const auto& v) { return v.size(); })))

using SpatialiteDatasource::GeometryType;
using SpatialiteDatasource::Dimension;
using SpatialiteDatasource::SpatialIndex;

class SpatialiteDatabaseGeometriesTest : public SpatialiteDatabaseTest,
                                public testing::WithParamInterface<SpatialiteDatasource::SpatialIndex>{};

INSTANTIATE_TEST_SUITE_P(Database, SpatialiteDatabaseGeometriesTest, testing::Values(
    SpatialIndex::None, 
    SpatialIndex::RTree, 
    SpatialIndex::MbrCache
#if NAVINFO_INTERNAL_BUILD
    , SpatialIndex::NavInfo
#endif
),
[](const testing::TestParamInfo<SpatialiteDatabaseGeometriesTest::ParamType>& info) {
    switch (info.param)
    {
    case SpatialIndex::None:
        return "NoIndex";
    case SpatialIndex::RTree:
        return "RTreeIndex";
    case SpatialIndex::MbrCache:
        return "MBRCache";
    case SpatialIndex::NavInfo:
        return "NavInfo";
    default:
        throw std::logic_error{"Unknown param"};
    }
});

TEST_P(SpatialiteDatabaseGeometriesTest, Points2DAreCreated)
{
    const auto table = InitializeDbWithGeometries("POINT", GetParam(), {
        "POINT(1 2)", 
        "POINT(3 4)"
    });
    
    CHECK_GEOMETRIES(table, GeometryType::Point, Dimension::D2, {{1, 2}}, {{3, 4}});
}

TEST_P(SpatialiteDatabaseGeometriesTest, Points3DAreCreated)
{
    const auto table = InitializeDbWithGeometries("POINTZ", GetParam(), {
        "POINTZ(1 2 3)", 
        "POINTZ(4 5 6)"
    });
    
    CHECK_GEOMETRIES(table, GeometryType::Point, Dimension::D3, {{1, 2, 3}}, {{4, 5, 6}});
}

TEST_P(SpatialiteDatabaseGeometriesTest, Lines2DAreCreated)
{
    const auto table = InitializeDbWithGeometries("LINESTRING", GetParam(), {
        "LINESTRING(1 2, 3 4, 5 6, 7 8)", 
        "LINESTRING(9 1, 2 3)"
    });
    
    CHECK_GEOMETRIES(table, GeometryType::Line, Dimension::D2, 
        {{1, 2}, {3, 4}, {5, 6}, {7, 8}}, 
        {{9, 1}, {2, 3}}
    );
}

TEST_P(SpatialiteDatabaseGeometriesTest, Lines3DAreCreated)
{
    const auto table = InitializeDbWithGeometries("LINESTRINGZ", GetParam(), {
        "LINESTRINGZ(1 2 3, 4 5 6, 7 8 9, 10 11 12)", 
        "LINESTRINGZ(3 4 5, 6 7 8)"
    });
    
    CHECK_GEOMETRIES(table, GeometryType::Line, Dimension::D3, 
        {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10, 11, 12}}, 
        {{3, 4, 5}, {6, 7, 8}}
    );
}

TEST_P(SpatialiteDatabaseGeometriesTest, Polygons2DAreCreated)
{
    const auto table = InitializeDbWithGeometries("POLYGON", GetParam(), {
        "POLYGON((1 2, 3 4, 5 6, 1 2))", 
        "POLYGON((7 8, 9 10, 11 12, 13 14, 7 8))"
    });
    
    CHECK_GEOMETRIES(table, GeometryType::Polygon, Dimension::D2, 
        {{1, 2}, {3, 4}, {5, 6}, {1, 2}}, 
        {{7, 8}, {9, 10}, {11, 12}, {13, 14}, {7, 8}}
    );
}

TEST_P(SpatialiteDatabaseGeometriesTest, Polygons3DAreCreated)
{
    const auto table = InitializeDbWithGeometries("POLYGONZ", GetParam(), {
        "POLYGONZ((1 2 3, 4 5 6, 7 8 9, 1 2 3))", 
        "POLYGONZ((11 12 13, 14 15 16, 17 18 19, 1 2 3, 11 12 13))"
    });
    
    CHECK_GEOMETRIES(table, GeometryType::Polygon, Dimension::D3, 
        {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {1, 2, 3}}, 
        {{11, 12, 13}, {14, 15, 16}, {17, 18, 19}, {1, 2, 3}, {11, 12, 13}}
    );
}

TEST_P(SpatialiteDatabaseGeometriesTest, MultiPoints2DAreCreated)
{
    const auto table = InitializeDbWithGeometries("MULTIPOINT", GetParam(), {
        "MULTIPOINT((1 2), (3 4), (5 6))", 
        "MULTIPOINT((7 8))"
    });
    
    CHECK_GEOMETRIES(table, GeometryType::MultiPoint, Dimension::D2, 
        {{1, 2}}, 
        {{3, 4}}, 
        {{5, 6}}, 
        {{7, 8}}
    );
}

TEST_P(SpatialiteDatabaseGeometriesTest, MultiPoints3DAreCreated)
{
    const auto table = InitializeDbWithGeometries("MULTIPOINTZ", GetParam(), {
        "MULTIPOINTZ((1 2 3), (4 5 6), (7 8 9))", 
        "MULTIPOINTZ((11 12 13))"
    });
    
    CHECK_GEOMETRIES(table, GeometryType::MultiPoint, Dimension::D3, 
        {{1, 2, 3}}, 
        {{4, 5, 6}}, 
        {{7, 8, 9}}, 
        {{11, 12, 13}}
    );
}

TEST_P(SpatialiteDatabaseGeometriesTest, MultiLines2DAreCreated)
{
    const auto table = InitializeDbWithGeometries("MULTILINESTRING", GetParam(), {
        "MULTILINESTRING((1 2, 3 4), (5 6, 7 8), (9 1, 2 3, 4 5))", 
        "MULTILINESTRING((13 14, 15 16))"
    });
    
    CHECK_GEOMETRIES(table, GeometryType::MultiLine, Dimension::D2, 
        {{1, 2}, {3, 4}},
        {{5, 6}, {7, 8}}, 
        {{9, 1}, {2, 3}, {4, 5}},
        {{13, 14}, {15, 16}}
    );
}

TEST_P(SpatialiteDatabaseGeometriesTest, MultiLines3DAreCreated)
{
    const auto table = InitializeDbWithGeometries("MULTILINESTRINGZ", GetParam(), {
        "MULTILINESTRINGZ((1 2 3, 3 4 2), (5 6 3, 7 8 4), (9 1 5, 2 3 6, 4 5 7))", 
        "MULTILINESTRINGZ((13 14 8, 16 17 9))"
    });
    
    CHECK_GEOMETRIES(table, GeometryType::MultiLine, Dimension::D3, 
        {{1, 2, 3}, {3, 4, 2}},
        {{5, 6, 3}, {7, 8, 4}}, 
        {{9, 1, 5}, {2, 3, 6}, {4, 5, 7}},
        {{13, 14, 8}, {16, 17, 9}}
    );
}

TEST_P(SpatialiteDatabaseGeometriesTest, MultiPolygons2DAreCreated)
{
    const auto table = InitializeDbWithGeometries("MULTIPOLYGON", GetParam(), {
        "MULTIPOLYGON(((1 2, 3 4, 5 6, 1 2)), ((7 8, 9 10, 11 12, 13 14, 7 8)), ((13 14, 15 16, 21 22, 13 14)))", 
        "MULTIPOLYGON(((17 18, 19 20, 1 2, 17 18)))"
    });
    
    CHECK_GEOMETRIES(table, GeometryType::MultiPolygon, Dimension::D2, 
        {{1, 2}, {3, 4}, {5, 6}, {1, 2}}, 
        {{7, 8}, {9, 10}, {11, 12}, {13, 14}, {7, 8}},
        {{13, 14}, {15, 16}, {21, 22}, {13, 14}},
        {{17, 18}, {19, 20}, {1, 2}, {17, 18}}
    );
}

TEST_P(SpatialiteDatabaseGeometriesTest, MultiPolygons3DAreCreated)
{
    const auto table = InitializeDbWithGeometries("MULTIPOLYGONZ", GetParam(), {
        "MULTIPOLYGONZ(((1 2 3, 3 4 1, 5 6 2, 1 2 3)), ((7 8 1, 9 10 2, 11 12 3, 13 14 4, 7 8 1)), ((13 14 1, 15 16 2, 21 22 3, 13 14 1)))", 
        "MULTIPOLYGONZ(((17 18 5, 19 20 6, 1 2 7, 17 18 5)))"
    });
    
    CHECK_GEOMETRIES(table, GeometryType::MultiPolygon, Dimension::D3, 
        {{1, 2, 3}, {3, 4, 1}, {5, 6, 2}, {1, 2, 3}}, 
        {{7, 8, 1}, {9, 10, 2}, {11, 12, 3}, {13, 14, 4}, {7, 8, 1}},
        {{13, 14, 1}, {15, 16, 2}, {21, 22, 3}, {13, 14, 1}},
        {{17, 18, 5}, {19, 20, 6}, {1, 2, 7}, {17, 18, 5}}
    );
}
