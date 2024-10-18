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

#pragma once

#include "GeometryType.h"
#include "TestDbDriver.h"

#include <Database.h>

#include <gtest/gtest.h>
#include <memory>
#include <unordered_map>

class SpatialiteDatabaseTest : public testing::Test
{
public:
    static void SetUpTestSuite()
    {
        testDb = std::make_unique<TestDbDriver>();
    }

    static void TearDownTestSuite()
    {
        const auto path = testDb->GetPath();
        testDb.reset();
        std::filesystem::remove(path);
    }

    auto InitializeDbWithEmptyTable(const std::string& geometry, SpatialiteDatasource::SpatialIndex spatialIndex, int srid = Wgs84Srid)
    {
        auto table = testDb->CreateTable(geometry, spatialIndex, srid);
        InitializeDb();
        return table;
    }

    auto InitializeDbWithGeometries(
        const std::string& geometry, 
        SpatialiteDatasource::SpatialIndex spatialIndex,
        const std::vector<std::string>& geometries
    )
    {
        auto table = testDb->CreateTableWithGeometries(geometry, spatialIndex, geometries);
        InitializeDb();
        return table;
    }

    void InitializeDb()
    {
        spatialiteDb = std::make_unique<SpatialiteDatasource::Database>(testDb->GetPath());
    }

    template <class L>
    [[nodiscard]] auto GetGeometries(
        SpatialiteDatasource::GeometryType geometryType, 
        SpatialiteDatasource::Dimension dimension, 
        const Table<L>& table
    )
    {
        return spatialiteDb->GetGeometries(
            table.name, table.geometryColumn, geometryType, dimension, emptyAttributesInfo, mbr);
    }

    std::unique_ptr<SpatialiteDatasource::Database> spatialiteDb;

    static constexpr SpatialiteDatasource::Mbr mbr{0, 0, 100, 100};
    inline static const SpatialiteDatasource::AttributesInfo emptyAttributesInfo{};
private:
    static std::unique_ptr<TestDbDriver> testDb;
};

inline std::tuple<SpatialiteDatasource::GeometryType, SpatialiteDatasource::Dimension, std::string> GetGeometryInfoFromGeometry(
    const std::string& geometry)
{
    using SpatialiteDatasource::GeometryType;
    using SpatialiteDatasource::Dimension;

    auto spatialiteGeometryType = geometry.substr(0, geometry.find('('));
    static const std::unordered_map<std::string, std::tuple<GeometryType, Dimension>> geometryTypes{
        {"POINT", {GeometryType::Point, Dimension::XY}},
        {"POINTZ", {GeometryType::Point, Dimension::XYZ}},
        {"LINESTRING", {GeometryType::Line, Dimension::XY}},
        {"LINESTRINGZ", {GeometryType::Line, Dimension::XYZ}},
        {"POLYGON", {GeometryType::Polygon, Dimension::XY}},
        {"POLYGONZ", {GeometryType::Polygon, Dimension::XYZ}},
        {"MULTIPOINT", {GeometryType::MultiPoint, Dimension::XY}},
        {"MULTIPOINTZ", {GeometryType::MultiPoint, Dimension::XYZ}},
        {"MULTILINESTRING", {GeometryType::MultiLine, Dimension::XY}},
        {"MULTILINESTRINGZ", {GeometryType::MultiLine, Dimension::XYZ}},
        {"MULTIPOLYGON", {GeometryType::MultiPolygon, Dimension::XY}},
        {"MULTIPOLYGONZ", {GeometryType::MultiPolygon, Dimension::XYZ}}
    };
    const auto [geometryType, dimension] = geometryTypes.at(spatialiteGeometryType);
    return {geometryType, dimension, std::move(spatialiteGeometryType)};
}
