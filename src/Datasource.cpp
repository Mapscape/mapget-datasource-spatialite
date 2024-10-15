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

#include <boost/algorithm/string/case_conv.hpp>
#include <nlohmann/json.hpp>
#include <mapget/log.h>
#include <spatialite/gg_const.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace SpatialiteDatasource {

Datasource::Datasource(const std::filesystem::path& mapPath, const std::filesystem::path& jsonInfoPath, uint16_t port)
    : m_db{mapPath}
    , m_ds{jsonInfoPath.empty() ? LoadDataSourceInfoFromDatabase(m_db) : LoadDataSourceInfoFromJson(jsonInfoPath)}
    , m_port{port}
{
}

void Datasource::EnableAttributes()
{
    const auto tables = m_db.GetTablesNames();
    std::string logMessage = "Datasource info read from the database:";
    for (const auto& table : tables)
    {
        logMessage += fmt::format("\n{}:", table);
        auto& attributesInfo = m_attributesInfo[table];
        attributesInfo = m_db.GetTableAttributes(table);
        for (const auto& attr : attributesInfo)
        {
            logMessage += fmt::format("\n{}: {}", attr.name, ColumnTypeToString(attr.type));
        }
    }
    mapget::log().info(logMessage);
}
void Datasource::EnableAttributesWithInfoJson(const std::filesystem::path& attributesInfoPath)
{
    mapget::log().info("Reading attributes info from {}", attributesInfoPath.string());
    std::ifstream file{attributesInfoPath};
    nlohmann::json json;
    file >> json;
    for (const auto& [table, attributes] : json.items())
    {
        const auto tableName = boost::to_lower_copy(table);
        for (const auto& [name, type] : attributes.items())
        {
            m_attributesInfo[tableName].emplace_back(name, ParseColumnType(type));
        }
    }
}

void Datasource::Run()
{
    m_ds.onTileRequest(
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

[[nodiscard]] mapget::DataSourceInfo Datasource::LoadDataSourceInfoFromJson(const std::filesystem::path& jsonInfoPath)
{
    mapget::log().info("Reading info from {}", jsonInfoPath.string());
    std::ifstream i{jsonInfoPath};
    nlohmann::json j;
    i >> j;
    return mapget::DataSourceInfo::fromJson(j);
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

void Datasource::Fill(const mapget::TileFeatureLayer::Ptr& tile) const
{
    const auto layerInfo = tile->layerInfo();
    for (const auto& featureType : layerInfo->featureTypes_)
    {
        const auto& tableName = featureType.name_;
        auto [geomColumn, geomType] = m_db.GetGeometryColumnInfo(tableName);
        switch (geomType) 
        {
        case GAIA_POINT:
        case GAIA_POINTM:
            CreateGeometries<GeometryType::Point, Dimension::D2>(tile, tableName, geomColumn);
            break;
        case GAIA_POINTZ:
        case GAIA_POINTZM:
            CreateGeometries<GeometryType::Point, Dimension::D3>(tile, tableName, geomColumn);
            break;
        case GAIA_LINESTRING:
        case GAIA_LINESTRINGM:
            CreateGeometries<GeometryType::Line, Dimension::D2>(tile, tableName, geomColumn);
            break;
        case GAIA_LINESTRINGZ:
        case GAIA_LINESTRINGZM:
            CreateGeometries<GeometryType::Line, Dimension::D3>(tile, tableName, geomColumn);
            break;
        case GAIA_POLYGON:
        case GAIA_POLYGONM:
            CreateGeometries<GeometryType::Polygon, Dimension::D2>(tile, tableName, geomColumn);
            break;
        case GAIA_POLYGONZ:
        case GAIA_POLYGONZM:
            CreateGeometries<GeometryType::Polygon, Dimension::D3>(tile, tableName, geomColumn);
            break;
            
        case GAIA_MULTIPOINT:
        case GAIA_MULTIPOINTM:
            CreateGeometries<GeometryType::MultiPoint, Dimension::D2>(tile, tableName, geomColumn);
            break;
        case GAIA_MULTIPOINTZ:
        case GAIA_MULTIPOINTZM:
            CreateGeometries<GeometryType::MultiPoint, Dimension::D3>(tile, tableName, geomColumn);
            break;
        case GAIA_MULTILINESTRING:
        case GAIA_MULTILINESTRINGM:
            CreateGeometries<GeometryType::MultiLine, Dimension::D2>(tile, tableName, geomColumn);
            break;
        case GAIA_MULTILINESTRINGZ:
        case GAIA_MULTILINESTRINGZM:
            CreateGeometries<GeometryType::MultiLine, Dimension::D3>(tile, tableName, geomColumn);
            break;
        case GAIA_MULTIPOLYGON:
        case GAIA_MULTIPOLYGONM:
            CreateGeometries<GeometryType::MultiPolygon, Dimension::D2>(tile, tableName, geomColumn);
            break;
        case GAIA_MULTIPOLYGONZ:
        case GAIA_MULTIPOLYGONZM:
            CreateGeometries<GeometryType::MultiPolygon, Dimension::D3>(tile, tableName, geomColumn);
            break;
        default:
            throw std::runtime_error{"Unknown spatialite geometry type"};
        }
    }
}

} // namespace SpatialiteDatasource
