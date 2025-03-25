// Copyright (C) 2025 by NavInfo Europe B.V. The Netherlands - All rights reserved
// Information classification: Confidential
// This content is protected by international copyright laws.
// Reproduction and distribution is prohibited without written permission.

#include "ConfigLoader.h"
#include "DatabaseTestFixture.h"

#include <gmock/gmock.h>

using namespace SpatialiteDatasource;

TEST(ConfigLoaderTest, LoadsDatasourceOptionsFromConfig)
{
    const auto config = YAML::Load(R"(
        map:
          path: default/path
        datasourcePort: 1234
    )");

    const ConfigLoader loader{config, {}};
    const auto& datasourceOptions = loader.GetDatasourceOptions();

    EXPECT_EQ(datasourceOptions.mapPath, "default/path");
    EXPECT_EQ(datasourceOptions.port, 1234);
}

TEST(ConfigLoaderTest, OptionsOverrideConfigValues)
{
    const auto config = YAML::Load(R"(
        map:
          path: default/path
        datasourcePort: 1234
    )");

    const std::filesystem::path mapPath = "override/path";
    constexpr uint16_t Port = 5678;

    OverrideOptions options;
    options.mapPath = mapPath;
    options.port = Port;

    const ConfigLoader loader{config, options};
    const auto& datasourceOptions = loader.GetDatasourceOptions();
    
    EXPECT_EQ(datasourceOptions.mapPath, mapPath);
    EXPECT_EQ(datasourceOptions.port, Port);
}

TEST(ConfigLoaderTest, WrongConfigFormatThrows)
{
    const auto config = YAML::Load(R"(
        someCompleteNonsense: nonsense
    )");
    EXPECT_THROW((ConfigLoader{config, {}}), std::runtime_error);
}

class ConfigLoaderTestFixture : public DatabaseTestFixture
{
protected:
    template <class... String>
    std::vector<Table> CreateEmptyGeometryTables(const String&... tableNames)
    {
        std::vector<Table> tables;
        ([&](const auto& tableName){
            auto table = CreateTable(tableName, {});
            table.AddGeometryColumn("geometry", "POINT");
            tables.emplace_back(std::move(table));
        }(tableNames), ...);
        InitializeDb();
        return tables;
    }

    Table CreateTableWithAttributes(std::string_view tableName)
    {
        auto table = CreateTable(tableName, {
            {"intAttribute", "INTEGER"},
            {"doubleAttribute", "FLOAT"},
            {"stringAttribute", "STRING"},
            {"blobAttribute", "BLOB"}
        });
        table.AddGeometryColumn("geometry", "POINT");
        table.Insert(42, 6.66, "value", Binary{"DEADBEEF"}, ::Geometry{"POINT(1,1)"});
        InitializeDb();
        return table;
    }

    static ConfigLoader CreateConfigLoader(const char* yamlConfig)
    {
        return ConfigLoader{YAML::Load(yamlConfig), DefaultOptions};
    }

    inline static const OverrideOptions DefaultOptions{"/map", {}, {}};
};

TEST_F(ConfigLoaderTestFixture, LoadsInfoFromDbOnEmptyConfig)
{
    auto table = CreateTableWithAttributes("test_table");
    const ConfigLoader loader{YAML::Load(""), {"/path/to/map", {}, {}}};

    const auto expectedDatasourceConfig = nlohmann::json::parse(R"JSON(
    {
      "layers": {
        "test_table": {
          "featureTypes": [
            {
              "name": "test_table",
              "uniqueIdCompositions": [
                [
                  {
                    "datatype": "I32",
                    "partId": "id"
                  }
                ]
              ]
            }
          ]
        }
      },
      "mapId": "map"
    })JSON");
    const auto datasourceConfig = loader.GenerateDatasourceConfig(*spatialiteDb);
    EXPECT_EQ(datasourceConfig, expectedDatasourceConfig);

    auto expectedTableInfo = table.UpdateAndGetTableInfo(GeometryType::Point, Dimension::XY);
    expectedTableInfo.attributes = {
        {"intAttribute", {ColumnType::Int64}},
        {"doubleAttribute", {ColumnType::Double}},
        {"stringAttribute", {ColumnType::Text}},
        {"blobAttribute", {ColumnType::Blob}}
    };
    const auto tablesInfo = loader.LoadTablesInfo(*spatialiteDb);
    ASSERT_EQ(tablesInfo.size(), 1);
    EXPECT_EQ(tablesInfo.at(expectedTableInfo.name), expectedTableInfo);
}

TEST_F(ConfigLoaderTestFixture, OnlyLayersFromConfigAreLoaded)
{
    const auto tables = CreateEmptyGeometryTables("table_from_config", "another_table");
    
    const auto loader = CreateConfigLoader(R"(
        layers:
        - table: table_from_config
          name: LayerNameFromConfig

        loadRemainingLayersFromDb: false
    )");

    const auto datasourceConfig = loader.GenerateDatasourceConfig(*spatialiteDb);
    const auto& layers = datasourceConfig["layers"];
    ASSERT_EQ(layers.size(), 1);
    EXPECT_EQ(layers["LayerNameFromConfig"]["featureTypes"][0]["name"], "table_from_config");

    const auto tablesInfo = loader.LoadTablesInfo(*spatialiteDb);
    ASSERT_EQ(tablesInfo.size(), 1);
    EXPECT_TRUE(tablesInfo.contains("table_from_config"));
}

TEST_F(ConfigLoaderTestFixture, LayersFromBothConfigAndDatabaseAreLoaded)
{
    const auto tables = CreateEmptyGeometryTables("table_from_config", "another_table");

    const auto loader = CreateConfigLoader(R"(
        layers:
        - table: table_from_config
    )");

    const auto datasourceConfig = loader.GenerateDatasourceConfig(*spatialiteDb);
    const auto& layers = datasourceConfig["layers"];
    ASSERT_EQ(layers.size(), 2);
    EXPECT_EQ(layers["table_from_config"]["featureTypes"][0]["name"], "table_from_config");
    EXPECT_EQ(layers["another_table"]["featureTypes"][0]["name"], "another_table");

    const auto tablesInfo = loader.LoadTablesInfo(*spatialiteDb);
    ASSERT_EQ(tablesInfo.size(), 2);
    EXPECT_TRUE(tablesInfo.contains("table_from_config"));
    EXPECT_TRUE(tablesInfo.contains("another_table"));
}

TEST_F(ConfigLoaderTestFixture, ParsesScaling)
{
    const auto tables = CreateEmptyGeometryTables("test_table");

    const auto loader = CreateConfigLoader(R"(
        layers:
        - table: test_table
          coordinatesScaling:
            xy: 10
            z: 100
    )");
    const auto tablesInfo = loader.LoadTablesInfo(*spatialiteDb);

    const auto& scaling = tablesInfo.at("test_table").scaling;
    EXPECT_EQ(scaling.x, 10);
    EXPECT_EQ(scaling.y, 10);
    EXPECT_EQ(scaling.z, 100);
}

TEST_F(ConfigLoaderTestFixture, DoesNotScaleIfScalingIsNotProvided)
{
    const auto tables = CreateEmptyGeometryTables("test_table");

    const auto loader = CreateConfigLoader(R"(
        layers:
        - table: test_table
    )");
    const auto tablesInfo = loader.LoadTablesInfo(*spatialiteDb);

    const auto& scaling = tablesInfo.at("test_table").scaling;
    EXPECT_EQ(scaling.x, 1);
    EXPECT_EQ(scaling.y, 1);
    EXPECT_EQ(scaling.z, 1);
}

TEST_F(ConfigLoaderTestFixture, UsesScalingFromGlobalAsDefault)
{
    const auto tables = CreateEmptyGeometryTables("test_table");

    const auto loader = CreateConfigLoader(R"(
        global:
          coordinatesScaling:
            x: 2
            y: 2
            z: 2
        layers:
          - table: test_table
    )");
    const auto tablesInfo = loader.LoadTablesInfo(*spatialiteDb);

    const auto& scaling = tablesInfo.at("test_table").scaling;
    EXPECT_EQ(scaling.x, 2);
    EXPECT_EQ(scaling.y, 2);
    EXPECT_EQ(scaling.z, 2);
}

TEST_F(ConfigLoaderTestFixture, ParsesAttributes)
{
    const auto tables = CreateEmptyGeometryTables("test_table");

    const auto loader = CreateConfigLoader(R"(
        layers:
        - table: test_table
          attributes:
          - name: my_attribute
            type: text
            relation:
              relatedColumns:
              - any_table.attribute
              delimiter: ' '
              matchCondition: "match condition"
          loadRemainingAttributesFromDb: false
    )");
    const auto tablesInfo = loader.LoadTablesInfo(*spatialiteDb);

    const auto& attributes = tablesInfo.at("test_table").attributes;
    ASSERT_EQ(attributes.size(), 1);
    const auto& attribute = attributes.at("my_attribute");
    EXPECT_EQ(attribute.type, ColumnType::Text);
    ASSERT_TRUE(attribute.relation.has_value());
    EXPECT_THAT(attribute.relation->columns, testing::ElementsAre("any_table.attribute"));
    EXPECT_EQ(attribute.relation->delimiter, " ");
    EXPECT_EQ(attribute.relation->matchCondition, "match condition");
}

TEST_F(ConfigLoaderTestFixture, OnlyAttributesFromConfigAreLoaded)
{
    const auto table = CreateTableWithAttributes("test_table");

    const auto loader = CreateConfigLoader(R"(
        layers:
        - table: test_table
          attributes:
          - name: my_attribute
            type: text
            relation:
              relatedColumns:
              - any_table.attribute
              delimiter: ' '
              matchCondition: "match condition"
          loadRemainingAttributesFromDb: false
    )");
    const auto tablesInfo = loader.LoadTablesInfo(*spatialiteDb);

    const auto& attributes = tablesInfo.at("test_table").attributes;
    ASSERT_EQ(attributes.size(), 1);
    EXPECT_TRUE(attributes.contains("my_attribute"));
}

TEST_F(ConfigLoaderTestFixture, DoesNotLoadAttributesIfDisabled)
{
    const auto table = CreateTableWithAttributes("test_table");

    const auto loader = CreateConfigLoader(R"(
        layers:
        - table: test_table
          attributes:
          - name: my_attribute
            type: text
            relation:
              relatedColumns:
              - any_table.attribute
              delimiter: ' '
              matchCondition: "match condition"
          loadRemainingAttributesFromDb: true
        
        disableAttributes: true
    )");
    const auto tablesInfo = loader.LoadTablesInfo(*spatialiteDb);

    const auto& attributes = tablesInfo.at("test_table").attributes;
    EXPECT_TRUE(attributes.empty());
}

TEST_F(ConfigLoaderTestFixture, LoadAttributesFromBothConfigAndDatabase)
{
    const auto table = CreateTableWithAttributes("test_table");

    const auto loader = CreateConfigLoader(R"(
        layers:
        - table: test_table
          attributes:
          - name: my_attribute
            type: text
            relation:
              relatedColumns:
              - any_table.attribute
              delimiter: ' '
              matchCondition: "match condition"
    )");
    const auto tablesInfo = loader.LoadTablesInfo(*spatialiteDb);

    const auto& attributes = tablesInfo.at("test_table").attributes;
    ASSERT_EQ(attributes.size(), 5);
    const auto keys = std::views::keys(attributes);
    EXPECT_THAT(std::vector(keys.begin(), keys.end()), testing::UnorderedElementsAre(
        "intAttribute", "doubleAttribute", "stringAttribute", "blobAttribute", "my_attribute"));
}
