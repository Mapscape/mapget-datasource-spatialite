// Copyright (C) 2024 by NavInfo Europe B.V. The Netherlands - All rights reserved
// Information classification: Confidential
// This content is protected by international copyright laws.
// Reproduction and distribution is prohibited without written permission.

#pragma once

#include "GeometryType.h"
#include "TestDbDriver.h"

#include <Database.h>

#include <gtest/gtest.h>
#include <memory>

class DatabaseTestFixture : public testing::Test
{
public:
    static void SetUpTestSuite();
    static void TearDownTestSuite();

    [[nodiscard]] Table CreateTable(std::string_view tableName, const std::vector<Column>& columns);

    Table InitializeDbWithEmptyGeometryTable(
        std::string_view tableName,
        const std::string& geometry,
        SpatialiteDatasource::SpatialIndex spatialIndex,
        int srid = Wgs84Srid);

    Table InitializeDbWithGeometries(
        const std::string& geometry,
        SpatialiteDatasource::SpatialIndex spatialIndex,
        const std::vector<std::string>& geometries);

    void InitializeDb();

    [[nodiscard]] SpatialiteDatasource::GeometriesView GetGeometries(
        SpatialiteDatasource::GeometryType geometryType, 
        SpatialiteDatasource::Dimension dimension, 
        const Table& table);

    std::unique_ptr<SpatialiteDatasource::Database> spatialiteDb;

    static constexpr SpatialiteDatasource::Mbr mbr{0, 0, 100, 100};
    inline static const SpatialiteDatasource::AttributesInfo emptyAttributesInfo{};
private:
    static std::unique_ptr<TestDbDriver> testDb;
};

std::tuple<SpatialiteDatasource::GeometryType, SpatialiteDatasource::Dimension, std::string>
GetGeometryInfoFromGeometry(const std::string& geometry);
