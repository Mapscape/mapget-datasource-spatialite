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

#include <unordered_map>

std::unique_ptr<TestDbDriver> DatabaseTestFixture::testDb{};

void DatabaseTestFixture::SetUpTestSuite()
{
    testDb = std::make_unique<TestDbDriver>();
}

void DatabaseTestFixture::TearDownTestSuite()
{
    const auto path = testDb->GetPath();
    testDb.reset();
    std::filesystem::remove(path);
}

[[nodiscard]] Table DatabaseTestFixture::CreateTable(std::string_view tableName, const std::vector<Column>& columns)
{
    return testDb->CreateTable(tableName, columns);
}

Table DatabaseTestFixture::InitializeDbWithEmptyGeometryTable(
    std::string_view tableName,
    const std::string& geometry,
    SpatialiteDatasource::SpatialIndex spatialIndex,
    int srid)
{
    auto table = CreateTable(tableName, {});
    table.AddGeometryColumn("geometry", geometry, srid);
    table.CreateSpatialIndex(spatialIndex);
    InitializeDb();
    return table;
}

Table DatabaseTestFixture::InitializeDbWithGeometries(
    const std::vector<std::string>& geometries,
    SpatialiteDatasource::SpatialIndex spatialIndex
)
{
    auto table = CreateTable("table_with_geometries", {});
    const auto geometry = std::get<std::string>(GetGeometryInfoFromGeometry(geometries[0]));
    table.AddGeometryColumn("geometry", geometry);
    table.CreateSpatialIndex(spatialIndex);
    for (const auto& geometry : geometries)
    {
        table.Insert(Geometry{geometry});
    }
    InitializeDb();
    return table;
}

void DatabaseTestFixture::InitializeDb()
{
    spatialiteDb = std::make_unique<SpatialiteDatasource::Database>(testDb->GetPath());
}

[[nodiscard]] SpatialiteDatasource::GeometriesView DatabaseTestFixture::GetGeometries(
    SpatialiteDatasource::GeometryType geometryType, 
    SpatialiteDatasource::Dimension dimension, 
    const Table& table
)
{
    return spatialiteDb->GetGeometries(
        table.name, table.GetGeometryColumnName(), geometryType, dimension, emptyTableInfo, mbr);
}

std::tuple<SpatialiteDatasource::GeometryType, SpatialiteDatasource::Dimension, std::string> GetGeometryInfoFromGeometry(
    const std::string& geometry)
{
    using SpatialiteDatasource::GeometryType;
    using SpatialiteDatasource::Dimension;

    auto spatialiteGeometryType = geometry.substr(0, geometry.find('('));
    static const std::unordered_map<std::string, std::tuple<GeometryType, Dimension>> GeometryTypes{
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
    const auto [geometryType, dimension] = GeometryTypes.at(spatialiteGeometryType);
    return {geometryType, dimension, std::move(spatialiteGeometryType)};
}

std::string_view SpatialIndexToString(SpatialiteDatasource::SpatialIndex index)
{
    using SpatialiteDatasource::SpatialIndex;

    switch (index)
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
}
