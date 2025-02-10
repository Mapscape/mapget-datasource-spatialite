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
#include "TableInfo.h"
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

static std::string GetTableNameFromLayerInfo(const std::shared_ptr<mapget::LayerInfo>& info)
{
    return boost::to_lower_copy(info->featureTypes_[0].name_);
}

Datasource::Datasource(const std::filesystem::path& mapPath, const nlohmann::json& config, uint16_t port, UseAttributes useAttributes)
    : m_db{mapPath}
    , m_ds{config.contains("datasourceInfo") ? mapget::DataSourceInfo::fromJson(config.at("datasourceInfo"))
                                             : LoadDataSourceInfoFromDatabase(m_db)}
    , m_port{port}
{
    LoadDefaultTablesInfo();

    const auto coordinatesScalingIt = config.find("coordinatesScaling");
    if (coordinatesScalingIt != config.end())
    {
        LoadCoordinatesScaling(*coordinatesScalingIt);
    }
    if (useAttributes == UseAttributes::Yes)
    {
        const auto attributesInfo = config.find("attributesInfo");
        LoadAttributes(attributesInfo != config.end() ? *attributesInfo : nlohmann::json::object());
    }
}

static void LogTablesInfo(const TablesInfo& info)
{
    std::string log = "Loaded attributes config:";
    constexpr auto Indent = 2;
    for (const auto& [table, tableInfo] : info)
    {
        log += fmt::format("\n{0:{1}}{2}:", "", Indent * 1, table);
        
        log += fmt::format("\n{0:{1}}{2}:", "", Indent * 2, "coordinatesScaling");
        const auto& scaling = tableInfo.scaling;
        log += fmt::format("\n{0:{1}}x: {2}", "", Indent * 3, scaling.x);
        log += fmt::format("\n{0:{1}}y: {2}", "", Indent * 3, scaling.y);
        log += fmt::format("\n{0:{1}}z: {2}", "", Indent * 3, scaling.z);

        log += fmt::format("\n{0:{1}}{2}:", "", Indent * 2, "attributes");
        for (const auto& [attribute, attributeInfo] : tableInfo.attributes)
        {
            log += fmt::format("\n{0:{1}}{2}({3})", "", Indent * 3, attribute, ColumnTypeToString(attributeInfo.type));
            if (attributeInfo.relation.has_value())
            {
                const auto& relation = attributeInfo.relation.value();
                log += fmt::format(":\n{0:{1}}columns: {2}\n{0:{1}}matchCondition: {3}\n{0:{1}}delimiter: '{4}'", 
                    "", Indent * 4, 
                    fmt::join(relation.columns, ", "),
                    relation.matchCondition,
                    relation.delimiter);
            }
        }
    }
    mapget::log().info(log);
}

static ScalingInfo ParseScalingConfig(const nlohmann::json::object_t& config)
{
    ScalingInfo result;
    for (const auto& [key, value] : config)
    {
        for (const auto& proj : key)
        {
            switch (proj)
            {
            case 'x':
                result.x = value;
                break;
            case 'y':
                result.y = value;
                break;
            case 'z':
                result.z = value;
                break;
            default:
                throw std::runtime_error{fmt::format("Unknown projection '{}' in '{}'", proj, key)};
            }
        }
    }
    return result;
}

void Datasource::LoadDefaultTablesInfo()
{
    const auto tables = m_ds.info().layers_ | std::views::values | std::views::transform(GetTableNameFromLayerInfo);
    for (const auto&& table : tables)
    {
        m_tablesInfo[table];
        m_featuresTilesByTable[table];
    }
}

void Datasource::LoadCoordinatesScaling(const nlohmann::json& coordinatesScalingConfig)
{
    const auto globalConfigIt = coordinatesScalingConfig.find("global");
    if (globalConfigIt != coordinatesScalingConfig.end())
    {
        const auto scaling = ParseScalingConfig(globalConfigIt.value());
        for (auto& info : m_tablesInfo | std::views::values)
        {
            info.scaling = scaling;
        }
    }
    const auto layersConfigIt = coordinatesScalingConfig.find("layers");
    if (layersConfigIt != coordinatesScalingConfig.end())
    {
        for (const auto& [layer, scaling] : layersConfigIt.value().items())
        {
            const auto table = GetTableNameFromLayerInfo(m_ds.info().layers_.at(layer));
            m_tablesInfo.at(table).scaling = ParseScalingConfig(scaling);
        }
    }
}

void Datasource::LoadAttributes(const nlohmann::json& attributesConfig)
{
    for (const auto& [layer, layerInfo] : m_ds.info().layers_)
    {
        const auto table = GetTableNameFromLayerInfo(layerInfo);
        auto& attributesInfo = m_tablesInfo.at(table).attributes;
        const auto layerAttributesInfo = attributesConfig.find(layer);
        if (layerAttributesInfo != attributesConfig.end())
        {
            const auto getFromDbFlag = layerAttributesInfo->value("getRemainingAttributesFromDb", true);
            if (getFromDbFlag)
            {
                attributesInfo = m_db.GetTableAttributes(table);
            }
            for (const auto& [attributeName, attributeDescription] : layerAttributesInfo->at("attributes").items())
            {
                attributesInfo[attributeName] = ParseAttributeInfo(attributeDescription);
            }
        }
        else
        {
            attributesInfo = m_db.GetTableAttributes(table);
        }
    }
    LogTablesInfo(m_tablesInfo);
}

[[nodiscard]] std::string Datasource::GetLayerIdFromTypeId(const std::string& typeId)
{
    // O(N), but N is quite small
    for (const auto& [layer, layerInfo] : m_ds.info().layers_)
    {
        if (layerInfo->featureTypes_[0].name_ == typeId)
        {
            return layer;
        }
    }
    mapget::log().error("Couldn't find layerId for typeId '{}'", typeId);
    return "Unknown";
}

void Datasource::Run()
{
    m_ds.onTileFeatureRequest(
        [this](auto&& tile)
        {
            try 
            {
                FillTileWithGeometries(tile);
            }
            catch (const std::exception& e)
            {
                mapget::log().error(e.what());
            }
        }
    );
    m_ds.onLocateRequest(
        [this](const auto& request) -> std::vector<mapget::LocateResponse>
        {
            try 
            {
                return LocateFeature(request);
            }
            catch (const std::exception& e)
            {
                mapget::log().error(e.what());
                return {};
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

void Datasource::FillTileWithGeometries(const mapget::TileFeatureLayer::Ptr& tile)
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
    Dimension dimension)
{
    const auto tid = tile->tileId();
    const Mbr mbr{
        .xmin = tid.sw().x,
        .ymin = tid.sw().y,
        .xmax = tid.ne().x,
        .ymax = tid.ne().y
    };

    constexpr size_t FeaturesBufferSize = 300;
    std::vector<int> featuresIds;
    featuresIds.reserve(FeaturesBufferSize);

    auto geometries = m_db.GetGeometries(tableName, geometryColumn, geometryType, dimension, m_tablesInfo.at(tableName), mbr);
    for (auto geometry : geometries)
    {
        const auto featureId = geometry.GetId();
        auto feature = tile->newFeature(tableName, {{"id", featureId}});
        MapgetFeature geometryFabric{*feature};
        geometry.AddTo(geometryFabric);
        featuresIds.push_back(featureId);
    }
    auto& [lock, map] = m_featuresTilesByTable.at(tableName);
    {
        std::lock_guard lockGuard{lock};
        for (const auto featureId : featuresIds)
        {
            map[featureId] = tid; // overwriting is fine
        }
    }
}

[[nodiscard]] std::vector<mapget::LocateResponse> Datasource::LocateFeature(const mapget::LocateRequest& request)
{
    std::vector<mapget::LocateResponse> responses;
    const auto& table = request.typeId_;
    const auto featureId = request.getIntIdPart("id");
    if (featureId == std::nullopt)
    {
        throw std::runtime_error(fmt::format(
            "Failed to process /locate request:\n{}", request.serialize().dump(2)));
    }
    auto& response = responses.emplace_back(request);
    response.tileKey_.layerId_ = GetLayerIdFromTypeId(request.typeId_);

    auto& [lock, map] = m_featuresTilesByTable.at(table);
    {
        std::shared_lock lockGuard{lock};
        response.tileKey_.tileId_ = map.at(static_cast<int>(*featureId));
    }

    return responses;
}

} // namespace SpatialiteDatasource
