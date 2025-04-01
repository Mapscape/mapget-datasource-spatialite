// Copyright (c) 2025 NavInfo Europe B.V.

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

#include "TableInfo.h"
#include "DatabaseTestFixture.h"
#include "FeatureMock.h"
#include "GeometryType.h"

using SpatialiteDatasource::ColumnType;

class SpatialiteDatabaseAttributesTest 
    : public DatabaseTestFixture
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

TEST_P(SpatialiteDatabaseAttributesTest, AttributesAreAddedToFeature)
{
    const auto& geometry = GetParam();
    const auto [geometryType, dimension, spatialiteType] = GetGeometryInfoFromGeometry(geometry);
    auto table = CreateTable("table_with_attributes", {
        {"intAttribute", "INTEGER"},
        {"doubleAttribute", "FLOAT"},
        {"stringAttribute", "STRING"},
        {"blobAttribute", "BLOB"},
    });
    table.AddGeometryColumn("geometry", spatialiteType);
    table.Insert(42, 6.66, "value", Binary{"DEADBEEF"}, Geometry{geometry});
    InitializeDb();

    auto& tableInfo = table.UpdateAndGetTableInfo(geometryType, dimension);
    tableInfo.attributes = {
        {"intAttribute", {ColumnType::Int64}},
        {"doubleAttribute", {ColumnType::Double}},
        {"stringAttribute", {ColumnType::Text}},
        {"blobAttribute", {ColumnType::Blob}}
    };

    auto geometries = spatialiteDb->GetGeometries(tableInfo, mbr);

    FeatureMock featureMock;
    {
        using testing::TypedEq;
        EXPECT_CALL(featureMock, AddAttribute("intAttribute", TypedEq<int64_t>(42))).Times(1);
        EXPECT_CALL(featureMock, AddAttribute("doubleAttribute", TypedEq<double>(6.66))).Times(1);
        EXPECT_CALL(featureMock, AddAttribute("stringAttribute", TypedEq<std::string_view>("value"))).Times(1);
        EXPECT_CALL(featureMock, AddAttribute("blobAttribute", TypedEq<std::string_view>("DEADBEEF"))).Times(1);
    }

    featureMock.AddGeometries(geometries);
}

class SpatialiteDatabaseAttributesRelationsTest : public DatabaseTestFixture
{
public:
    Table CreateGeometryTable()
    {
        auto table = CreateTable("geometries_table", {{"myEnum", "INTEGER"}});
        table.AddGeometryColumn("geometry", "POINT");
        table.Insert(42, Geometry{"POINT(1 1)"});
        return table;
    }

    auto GetGeometries(Table& geometryTable, SpatialiteDatasource::AttributesInfo&& attributesInfo)
    {
        auto& tableInfo = geometryTable.UpdateAndGetTableInfo(SpatialiteDatasource::GeometryType::Point, SpatialiteDatasource::Dimension::XY);
        tableInfo.attributes = std::move(attributesInfo);
        return spatialiteDb->GetGeometries(tableInfo, mbr);
    }
};

TEST_F(SpatialiteDatabaseAttributesRelationsTest, SingleColumnRelatedAttributeIsAddedToFeature)
{
    auto geometryTable = CreateGeometryTable();
    auto relatedTable = CreateTable("related_table", {{"meaningfulNumber", "INTEGER"}, {"value", "INTEGER"}});
    relatedTable.Insert(666, 42);
    InitializeDb();

    auto geometries = GetGeometries(geometryTable, 
        {{"attribute",
            {ColumnType::Int64,
            SpatialiteDatasource::Relation{
                .columns = {"related_table.meaningfulNumber"},
                .delimiter = ";",
                .matchCondition = "layerTable.myEnum == related_table.value"}}}});

    FeatureMock featureMock;
    EXPECT_CALL(featureMock, AddAttribute("attribute", testing::TypedEq<int64_t>(666))).Times(1);
    featureMock.AddGeometries(geometries);
}

TEST_F(SpatialiteDatabaseAttributesRelationsTest, MultiColumnSingleTableRelatedAttributeIsAddedToFeature)
{
    auto geometryTable = CreateGeometryTable();
    auto relatedTable = CreateTable("related_table", {{"meaningfulNumber", "INTEGER"}, {"meaningfulString", "STRING"}, {"value", "INTEGER"}});
    relatedTable.Insert(666, "spasibo", 42);
    InitializeDb();

    auto geometries = GetGeometries(geometryTable,
        {{"attribute",
            {ColumnType::Text,
             SpatialiteDatasource::Relation{
                 .columns = {"related_table.meaningfulString", "related_table.meaningfulNumber"},
                 .delimiter = " - ",
                 .matchCondition = "layerTable.myEnum == related_table.value"}}}});

    FeatureMock featureMock;
    EXPECT_CALL(featureMock, AddAttribute("attribute", testing::TypedEq<std::string_view>("spasibo - 666"))).Times(1);
    featureMock.AddGeometries(geometries);
}

TEST_F(SpatialiteDatabaseAttributesRelationsTest, MultiColumnMultiTableRelatedAttributeIsAddedToFeature)
{
    auto geometryTable = CreateGeometryTable();
    auto relatedTable1 = CreateTable("related_table1", {{"meaningfulNumber", "INTEGER"}, {"value", "INTEGER"}});
    relatedTable1.Insert(666, 42);
    auto relatedTable2 = CreateTable("related_table2", {{"meaningfulNumber", "INTEGER"}, {"value", "INTEGER"}});
    relatedTable2.Insert(333, 42);
    InitializeDb();

    auto geometries = GetGeometries(geometryTable, 
        {{"attribute",
            {ColumnType::Text,
             SpatialiteDatasource::Relation{
                 .columns = {"related_table2.meaningfulNumber", "related_table1.meaningfulNumber"},
                 .delimiter = "*2=",
                 .matchCondition = "layerTable.myEnum == related_table1.value AND layerTable.myEnum == related_table2.value"}}}});

    FeatureMock featureMock;
    EXPECT_CALL(featureMock, AddAttribute("attribute", testing::TypedEq<std::string_view>("333*2=666"))).Times(1);
    featureMock.AddGeometries(geometries);
}
