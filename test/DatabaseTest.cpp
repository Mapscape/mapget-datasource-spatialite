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
#include "GeometryType.h"

#include <spatialite/gg_const.h>
#include <stdexcept>

#define EXPECT_EXCEPTION(expr, msg)     \
    try                                 \
    {                                   \
        static_cast<void>(expr);        \
    }                                   \
    catch (const std::exception& e)     \
    {                                   \
        EXPECT_STREQ(e.what(), msg);    \
    }

using SpatialiteDatasource::GeometryType;
using SpatialiteDatasource::Dimension;
using SpatialiteDatasource::SpatialIndex;

class SpatialiteDatabaseTest : public DatabaseTestFixture {};

TEST_F(SpatialiteDatabaseTest, ExceptionIsThrownIfNoGeometryColumnInTable)
{
    InitializeDb();
    EXPECT_EXCEPTION(spatialiteDb->GetGeometryColumnInfo("blahblahblah"), "Table 'blahblahblah' is not in 'geometry_columns'");
}

TEST_F(SpatialiteDatabaseTest, ExceptionIsThrownIfGeometryColumnIsNotInWGS84)
{
    const auto table = InitializeDbWithEmptyGeometryTable("my_table", "POINT", SpatialIndex::None, 3857);
    EXPECT_EXCEPTION(spatialiteDb->GetGeometryColumnInfo(table.name), "Geometry column of 'my_table' table is not in WGS84");
}

TEST_F(SpatialiteDatabaseTest, GeometryColumnInfoIsReturned)
{
    const auto table = InitializeDbWithEmptyGeometryTable("my_table", "POINT", SpatialIndex::None);
    const auto geomInfo = spatialiteDb->GetGeometryColumnInfo(table.name);
    EXPECT_EQ(geomInfo.name, table.GetGeometryColumnName());
    EXPECT_EQ(geomInfo.type, GAIA_POINT);
}

TEST_F(SpatialiteDatabaseTest, TablesNamesAreReturned)
{
    const auto table = InitializeDbWithEmptyGeometryTable("my_table", "POINT", SpatialIndex::None);
    const auto tablesNames = spatialiteDb->GetTablesNames();
    ASSERT_EQ(tablesNames.size(), 1);
    EXPECT_EQ(tablesNames[0], table.name);
}

TEST_F(SpatialiteDatabaseTest, EmptyViewDoesNotThrow)
{
    auto table = InitializeDbWithEmptyGeometryTable("my_table", "POINT", SpatialIndex::None);
    const auto& tableInfo = table.UpdateAndGetTableInfo(GeometryType::Point, Dimension::XY);
    auto geometries = spatialiteDb->GetGeometries(tableInfo, {0, 0, 0, 0});

    EXPECT_EQ(geometries.begin(), geometries.end());
}
