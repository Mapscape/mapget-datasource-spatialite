// Copyright (C) 2024 by NavInfo Europe B.V. The Netherlands - All rights reserved
// Information classification: Confidential
// This content is protected by international copyright laws.
// Reproduction and distribution is prohibited without written permission.

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
    const std::string& geometry, 
    SpatialiteDatasource::SpatialIndex spatialIndex,
    const std::vector<std::string>& geometries)
{
    auto table = CreateTable("table_with_geometries", {});
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
        table.name, table.GetGeometryColumnName(), geometryType, dimension, emptyAttributesInfo, mbr);
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
