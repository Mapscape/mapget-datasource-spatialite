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
        const std::vector<std::string>& geometries,
        SpatialiteDatasource::SpatialIndex spatialIndex = SpatialiteDatasource::SpatialIndex::None);

    void InitializeDb();

    [[nodiscard]] SpatialiteDatasource::GeometriesView GetGeometries(
        SpatialiteDatasource::GeometryType geometryType, 
        SpatialiteDatasource::Dimension dimension, 
        Table& table);

    std::unique_ptr<SpatialiteDatasource::Database> spatialiteDb;

    static constexpr SpatialiteDatasource::Mbr mbr{0, 0, 100, 100};
private:
    static std::unique_ptr<TestDbDriver> testDb;
};

std::tuple<SpatialiteDatasource::GeometryType, SpatialiteDatasource::Dimension, std::string>
GetGeometryInfoFromGeometry(const std::string& geometry);

std::string_view SpatialIndexToString(SpatialiteDatasource::SpatialIndex index);
