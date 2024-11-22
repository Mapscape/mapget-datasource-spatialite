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

#include "Datasource.h"
#include "AttributesInfo.h"
#include "GeometryType.h"
#include "MapgetFeature.h"

#include <fmt/ranges.h>
#include <boost/algorithm/string/case_conv.hpp>
#include <nlohmann/json.hpp>
#include <mapget/log.h>
#include <spatialite/gg_const.h>

#include <filesystem>
#include <stdexcept>
#include <ranges>

namespace SpatialiteDatasource {

Datasource::Datasource(const std::filesystem::path& mapPath, const nlohmann::json& config, uint16_t port, UseAttributes useAttributes)
    : m_db{mapPath}
    , m_ds{config.contains("datasourceInfo") ? mapget::DataSourceInfo::fromJson(config.at("datasourceInfo"))
                                             : LoadDataSourceInfoFromDatabase(m_db)}
    , m_port{port}
{
    if (useAttributes == UseAttributes::Yes)
{
        const auto attributesInfo = config.find("attributesInfo");
        LoadAttributes(attributesInfo != config.end() ? *attributesInfo : nlohmann::json::object());
    }
}

static void LogTableAttributes(const TablesAttributesInfo& info)
{
    std::string log = "Loaded attributes config:";
    constexpr auto Indent = 2;
    for (const auto& [table, attributesInfo] : info)
    {
        log += fmt::format("\n{0:{1}}{2}:", "", Indent, table);
        for (const auto& [attribute, attributeInfo] : attributesInfo)
        {
            log += fmt::format("\n{0:{1}}{2}({3})", "", Indent * 2, attribute, ColumnTypeToString(attributeInfo.type));
            if (attributeInfo.relation.has_value())
            {
                const auto& relation = attributeInfo.relation.value();
                log += fmt::format(":\n{0:{1}}columns: {2}\n{0:{1}}matchCondition: {3}\n{0:{1}}delimiter: '{4}'", 
                    "", Indent * 3, 
                    fmt::join(relation.columns, ", "),
                    relation.matchCondition,
                    relation.delimiter);
            }
        }
    }
    mapget::log().info(log);
}

void Datasource::LoadAttributes(const nlohmann::json& attributesConfig)
{
    const auto getLayerAndTable = [](const auto& layer) {
        return std::make_pair(std::cref(layer.first), boost::to_lower_copy(layer.second->featureTypes_[0].name_));
    };
    const auto layers = m_ds.info().layers_ | std::views::transform(getLayerAndTable);

    for (const auto&& [layer, table] : layers)
    {
        auto& attributesInfo = m_attributesInfo[table];
        const auto layerInfo = attributesConfig.find(layer);
        if (layerInfo != attributesConfig.end())
    {
            const auto getFromDbFlag = layerInfo->value("getRemainingAttributesFromDb", true);
            if (getFromDbFlag)
            {
                attributesInfo = m_db.GetTableAttributes(table);
            }
            for (const auto& [attributeName, attributeDescription] : layerInfo->at("attributes").items())
        {
                attributesInfo[attributeName] = ParseAttributeInfo(attributeDescription);
            }
        }
        else
        {
            attributesInfo = m_db.GetTableAttributes(table);
        }
    }
    LogTableAttributes(m_attributesInfo);
}

void Datasource::Run()
{
    m_ds.onTileFeatureRequest(
        [this](auto&& tile)
        {
            try 
            {
                Fill(tile);
            }
            catch (const std::exception& e)
            {
                mapget::log().error(e.what());
            }
}
    );
    m_ds.go("0.0.0.0", m_port);
    mapget::log().info("Running on port {}...", m_ds.port());
    m_ds.waitForSignal();
}

[[nodiscard]] mapget::DataSourceInfo Datasource::LoadDataSourceInfoFromDatabase(const Database& db)
{
    nlohmann::json infoJson;
    infoJson["mapId"] = db.GetDatabaseFilePath();
    infoJson["layers"] = nlohmann::json::object();
    auto& layers = infoJson["layers"];

    const auto tables = db.GetTablesNames();
    for (const auto& table : tables)
    {
        layers[table] = {{"featureTypes", nlohmann::json::array({nlohmann::json::object({
            {"name", table},
            {"uniqueIdCompositions", nlohmann::json::array({nlohmann::json::array({nlohmann::json::object({
                {"partId", "id"},
                {"datatype", "I32"}
        })})})}})})}};
    }
    mapget::log().info("Datasource info read from the database:\n{}", infoJson.dump(2));
    return mapget::DataSourceInfo::fromJson(infoJson);
}

[[nodiscard]] AttributeInfo Datasource::ParseAttributeInfo(const nlohmann::json& attributeDescription) const
{
    AttributeInfo attribute;
    std::optional<ColumnType> type;
    const auto typeIter = attributeDescription.find("type");
    if (typeIter != attributeDescription.end())
    {
        type = ParseColumnType(*typeIter);
    }

    const auto relationIter = attributeDescription.find("relation");
    if (relationIter != attributeDescription.end())
    {
        auto& relation = attribute.relation.emplace();
        relation.delimiter = relationIter->value("delimiter", "|");;
        relation.matchCondition = relationIter->at("matchCondition");
        const auto& relatedColumns = relationIter->at("relatedColumns");
        if (relatedColumns.size() == 1)
        {
            std::string columnName = relatedColumns[0];
            auto tableName = columnName.substr(0, columnName.find('.'));
            if (!type.has_value())
            {
                type = m_db.GetColumnType(tableName, columnName);
            }
            relation.columns.emplace_back(std::move(columnName));
        }
        else 
        {
            if (!type.has_value())
            {
                type = ColumnType::Text;
            }
            for (std::string column : relatedColumns)
            {
                relation.columns.emplace_back(std::move(column));
            }
        }
    }
    attribute.type = type.value();
    return attribute;
}

static GeometryType GetGeometryType(int spatialiteType)
{
    int geometry = spatialiteType % 1'000;
    if (geometry < static_cast<int>(GeometryType::Point) || geometry > static_cast<int>(GeometryType::MultiPolygon))
        throw std::runtime_error{fmt::format("Unknown spatialite geometry type: {}", spatialiteType)};
    return static_cast<GeometryType>(geometry);
}

static Dimension GetDimension(int spatialiteType)
{
    int dimension = spatialiteType / 1'000;
    if (dimension < static_cast<int>(Dimension::XY) || dimension > static_cast<int>(Dimension::XYZM))
        throw std::runtime_error{fmt::format("Can't get dimension from spatialite geometry type: {}", spatialiteType)};
    return static_cast<Dimension>(dimension);
}

void Datasource::Fill(const mapget::TileFeatureLayer::Ptr& tile) const
{
    const auto layerInfo = tile->layerInfo();
    for (const auto& featureType : layerInfo->featureTypes_)
    {
        const auto& tableName = featureType.name_;
        auto [geomColumn, geomType] = m_db.GetGeometryColumnInfo(tableName);
        CreateGeometries(tile, tableName, geomColumn, GetGeometryType(geomType), GetDimension(geomType));
    }
}

void Datasource::CreateGeometries(
    const mapget::TileFeatureLayer::Ptr& tile, 
    const std::string& tableName, 
    const std::string& geometryColumn,
    GeometryType geometryType,
    Dimension dimension) const
{
    const auto tid = tile->tileId();
    const Mbr mbr{
        .xmin = tid.sw().x,
        .ymin = tid.sw().y,
        .xmax = tid.ne().x,
        .ymax = tid.ne().y
    };
    // it's much simpler to just create an empty info if not found, just 56 bytes per table
    const auto& attributesInfo = m_attributesInfo[tableName];
    auto geometries = m_db.GetGeometries(tableName, geometryColumn, geometryType, dimension, attributesInfo, mbr);
    for (auto geometry : geometries)
    {
        auto feature = tile->newFeature(tableName, {{"id", geometry.GetId()}});
        MapgetFeature geometryFabric{*feature};
        geometry.AddTo(geometryFabric);
    }
}

} // namespace SpatialiteDatasource
