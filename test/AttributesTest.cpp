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

#include "AttributesInfo.h"
#include "DatabaseTest.h"
#include "FeatureMock.h"
#include "GeometryType.h"

using SpatialiteDatasource::ColumnType;

class SpatialiteDatabaseAttributesTest 
    : public SpatialiteDatabaseTest
    , public testing::WithParamInterface<std::string>{};

INSTANTIATE_TEST_SUITE_P(Database, SpatialiteDatabaseAttributesTest, testing::Values(
    "POINT(1 2)",
    "POINTZ(1 2 3)",
    "LINESTRING(9 1, 2 3)",
    "LINESTRINGZ(3 4 5, 6 7 8)",
    "POLYGON((1 2, 3 4, 5 6, 1 2))",
    "POLYGONZ((1 2 3, 4 5 6, 7 8 9, 1 2 3))",
    "MULTIPOINT((1 2), (3 4), (5 6))",
    "MULTIPOINTZ((1 2 3), (4 5 6), (7 8 9))",
    "MULTILINESTRING((5 6, 7 8), (9 1, 2 3, 4 5))",
    "MULTILINESTRINGZ((5 6 3, 7 8 4), (9 1 5, 2 3 6, 4 5 7))",
    "MULTIPOLYGON(((7 8, 9 10, 11 12, 13 14, 7 8)), ((13 14, 15 16, 21 22, 13 14)))",
    "MULTIPOLYGONZ(((7 8 1, 9 10 2, 11 12 3, 13 14 4, 7 8 1)), ((13 14 1, 15 16 2, 21 22 3, 13 14 1)))"),
    [](const auto& info)
    {
        return std::get<std::string>(GetGeometryInfoFromGeometry(info.param));
    }
);

TEST_P(SpatialiteDatabaseAttributesTest, Attributes)
{
    const auto& geometry = GetParam();
    const auto [geometryType, dimension, spatialiteType] = GetGeometryInfoFromGeometry(geometry);
    const auto table = InitializeDbWithGeometries(
        spatialiteType, 
        SpatialiteDatasource::SpatialIndex::None, 
        {geometry}
    );
    SpatialiteDatasource::AttributesInfo attributesInfo{
        {"intAttribute", ColumnType::Int64},
        {"doubleAttribute", ColumnType::Double},
        {"stringAttribute", ColumnType::String}
    };

    auto geometries = spatialiteDb->GetGeometries(
        table.name,
        table.geometryColumn,
        geometryType,
        dimension,
        attributesInfo,
        mbr);
    FeatureMock featureMock;
    for (auto g : geometries)
    {
        g.AddTo(featureMock);
    }
    EXPECT_EQ(featureMock.attributesCount, 3);

    EXPECT_EQ(featureMock.intAttribute.name, "intAttribute");
    EXPECT_EQ(featureMock.intAttribute.value, 42);

    EXPECT_EQ(featureMock.doubleAttribute.name, "doubleAttribute");
    EXPECT_EQ(featureMock.doubleAttribute.value, 6.66);

    EXPECT_EQ(featureMock.stringAttribute.name, "stringAttribute");
    EXPECT_EQ(featureMock.stringAttribute.value, "value");
}
