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

#define BOOST_HANA_CONFIG_ENABLE_STRING_UDL
#include <boost/hana/string.hpp>

using SpatialiteDatasource::GeometryType;
using SpatialiteDatasource::Dimension;
using SpatialiteDatasource::ColumnType;

using namespace boost::hana::literals;

std::string GetGeometryTypeFromGeometry(const std::string& geometry)
{
    return geometry.substr(0, geometry.find('('));
}

template <typename T>
class SpatialiteDatabaseAttributesTest : public SpatialiteDatabaseTest{};

template <GeometryType GeometryT, Dimension Dim, class GeometryStr>
struct P
{
    using Geometry = GeometryStr;
    static constexpr auto geometryType = GeometryT;
    static constexpr auto dimension = Dim;
};

#define STR(x) decltype(x""_s)

using Parameters = ::testing::Types<
    P<GeometryType::Point,        Dimension::D2, STR("POINT(1 2)")>,
    P<GeometryType::Point,        Dimension::D3, STR("POINTZ(1 2 3)")>,
    P<GeometryType::Line,         Dimension::D2, STR("LINESTRING(9 1, 2 3)")>,
    P<GeometryType::Line,         Dimension::D3, STR("LINESTRINGZ(3 4 5, 6 7 8)")>,
    P<GeometryType::Polygon,      Dimension::D2, STR("POLYGON((1 2, 3 4, 5 6, 1 2))")>,
    P<GeometryType::Polygon,      Dimension::D3, STR("POLYGONZ((1 2 3, 4 5 6, 7 8 9, 1 2 3))")>,
    P<GeometryType::MultiPoint,   Dimension::D2, STR("MULTIPOINT((1 2), (3 4), (5 6))")>,
    P<GeometryType::MultiPoint,   Dimension::D3, STR("MULTIPOINTZ((1 2 3), (4 5 6), (7 8 9))")>,
    P<GeometryType::MultiLine,    Dimension::D2, STR("MULTILINESTRING((5 6, 7 8), (9 1, 2 3, 4 5))")>,
    P<GeometryType::MultiLine,    Dimension::D3, STR("MULTILINESTRINGZ((5 6 3, 7 8 4), (9 1 5, 2 3 6, 4 5 7))")>,
    P<GeometryType::MultiPolygon, Dimension::D2, STR("MULTIPOLYGON(((7 8, 9 10, 11 12, 13 14, 7 8)), ((13 14, 15 16, 21 22, 13 14)))")>,
    P<GeometryType::MultiPolygon, Dimension::D3, STR("MULTIPOLYGONZ(((7 8 1, 9 10 2, 11 12 3, 13 14 4, 7 8 1)), ((13 14 1, 15 16 2, 21 22 3, 13 14 1)))")>
>;

struct NameGenerator
{
    template <typename T>
    static std::string GetName(int) 
    {
        return GetGeometryTypeFromGeometry(typename T::Geometry{}.c_str());
    }
};

TYPED_TEST_SUITE_P(SpatialiteDatabaseAttributesTest);

TYPED_TEST_P(SpatialiteDatabaseAttributesTest, Attr)
{
    std::string geometry = typename TypeParam::Geometry{}.c_str();

    const auto table = this->InitializeDbWithGeometries(
        GetGeometryTypeFromGeometry(geometry), 
        SpatialiteDatasource::SpatialIndex::None, 
        {geometry}
    );
    SpatialiteDatasource::AttributesInfo attributesInfo{
        {"intAttribute", ColumnType::Int64},
        {"doubleAttribute", ColumnType::Double},
        {"stringAttribute", ColumnType::String}
    };

    auto geometries = this->spatialiteDb->template GetGeometries<TypeParam::geometryType, TypeParam::dimension>(
        table.name,
        table.geometryColumn,
        attributesInfo,
        this->mbr);
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

REGISTER_TYPED_TEST_SUITE_P(SpatialiteDatabaseAttributesTest, Attr); 
INSTANTIATE_TYPED_TEST_SUITE_P(Database, SpatialiteDatabaseAttributesTest, Parameters, NameGenerator);
