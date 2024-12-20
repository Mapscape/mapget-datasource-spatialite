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

static const SpatialiteDatasource::Mbr Mbr{5, 5, 10, 10};

using TestParam = std::tuple<std::string, SpatialiteDatasource::ScalingInfo, MapgetGeometry>;

class SpatialiteDatabaseScalingTest 
    : public DatabaseTestFixture
    , public testing::WithParamInterface<std::tuple<TestParam, SpatialIndex>>{};

struct TestNameGenerator
{
    std::string operator()(const auto& info) const
    {
        const auto& [geometry, scaling, expectedGeometry] = std::get<TestParam>(info.param);
        auto testName = std::get<std::string>(GetGeometryInfoFromGeometry(geometry));
        boost::to_lower(testName);
        testName[0] = std::toupper(testName[0]);
        testName += scaling.x < 1 ? "ScaleDown" : "ScaleUp";
        testName += SpatialIndexToString(std::get<SpatialIndex>(info.param));
        return testName;
    }
};

INSTANTIATE_TEST_SUITE_P(Database, SpatialiteDatabaseScalingTest, testing::Combine(
    testing::ValuesIn(std::initializer_list<TestParam>{
        {"POINT(600 7000)",        {0.01, 0.001},         {{6, 7}}},
        {"POINTZ(600 7000 80000)", {0.01, 0.001, 0.0001}, {{6, 7, 8}}},
        {"POINT(0.6 0.07)",        {10, 100},             {{6, 7}}},
        {"POINTZ(0.6 0.07 0.008)", {10, 100, 1000},       {{6, 7, 8}}},

        {"LINESTRING(600 7000, 700 8000)",              {0.01, 0.001},         {{6, 7}, {7, 8}}},
        {"LINESTRINGZ(600 7000 80000, 700 8000 90000)", {0.01, 0.001, 0.0001}, {{6, 7, 8}, {7, 8, 9}}},
        {"LINESTRING(0.6 0.07, 0.7 0.08)",              {10, 100},             {{6, 7}, {7, 8}}},
        {"LINESTRINGZ(0.6 0.07 0.008, 0.7 0.08 0.009)", {10, 100, 1000},       {{6, 7, 8}, {7, 8, 9}}}
    }),
    testing::Values(
        SpatialIndex::None, 
        SpatialIndex::RTree, 
        SpatialIndex::MbrCache
    #if NAVINFO_INTERNAL_BUILD
        , SpatialIndex::NavInfo
    #endif
    )),
    TestNameGenerator{}
);

TEST_P(SpatialiteDatabaseScalingTest, ScaledGeometriesAreAddedToFeature)
{
    const auto& [geometry, scaling, expectedGeometry] = std::get<0>(GetParam());
    const auto table = InitializeDbWithGeometries({geometry});
    SpatialiteDatasource::TableInfo tableInfo{
        .attributes = {},
        .scaling = scaling
    };
    const auto [geometryType, dimension, geometryTypeStr] = GetGeometryInfoFromGeometry(geometry);
    auto geometries = spatialiteDb->GetGeometries(
        table.name,
        table.GetGeometryColumnName(),
        geometryType,
        dimension,
        tableInfo,
        Mbr
    );
    FeatureMock featureMock;
    featureMock.AddGeometries(geometries);
    ASSERT_EQ(featureMock.geometries.size(), 1);
    ASSERT_EQ(featureMock.geometries[0].size(), expectedGeometry.size());
    for (size_t i = 0; i < expectedGeometry.size(); ++i)
    {
        const auto& point = featureMock.geometries[0][i];
        EXPECT_DOUBLE_EQ(point.x, expectedGeometry[i].x);
        EXPECT_DOUBLE_EQ(point.y, expectedGeometry[i].y);
        EXPECT_DOUBLE_EQ(point.z, expectedGeometry[i].z);
    }
}
