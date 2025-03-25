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
#include "MapgetFeature.h"

#include <mapget/log.h>

#include <stdexcept>
#include <ranges>

namespace SpatialiteDatasource {

Datasource::Datasource(ConfigLoader&& configLoader)
    : m_db{configLoader.GetDatasourceOptions().mapPath}
    , m_ds{mapget::DataSourceInfo::fromJson(configLoader.GenerateDatasourceConfig(m_db))}
    , m_tablesInfo{configLoader.LoadTablesInfo(m_db)}
    , m_port{configLoader.GetDatasourceOptions().port}
{
    for (const auto& table : std::views::keys(m_tablesInfo))
    {
        m_featuresTilesByTable[table];
    }
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

void Datasource::FillTileWithGeometries(const mapget::TileFeatureLayer::Ptr& tile)
{
    const auto layerInfo = tile->layerInfo();
    for (const auto& featureType : layerInfo->featureTypes_)
    {
        const auto& tableName = featureType.name_;
        const auto tableInfoIt = m_tablesInfo.find(tableName);
        if (tableInfoIt == m_tablesInfo.end())
        {
            throw std::runtime_error{fmt::format("Unknown table '{}'", tableName)};
        }
        CreateGeometries(tile, tableInfoIt->second);
    }
}

void Datasource::CreateGeometries(const mapget::TileFeatureLayer::Ptr& tile, const TableInfo& tableInfo)
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

    auto geometries = m_db.GetGeometries(tableInfo, mbr);
    for (auto geometry : geometries)
    {
        const auto featureId = geometry.GetId();
        auto feature = tile->newFeature(tableInfo.name, {{"id", featureId}});
        MapgetFeature geometryFabric{*feature};
        geometry.AddTo(geometryFabric);
        featuresIds.push_back(featureId);
    }
    auto& [lock, map] = m_featuresTilesByTable.at(tableInfo.name);
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

Datasource CreateDatasourceDefaultConfig(const OverrideOptions& options)
{
    return Datasource{{YAML::Load(""), options}};
}

Datasource CreateDatasource(const std::filesystem::path& configPath, const OverrideOptions& options)
{
    return Datasource{{YAML::LoadFile(configPath), options}};
}

} // namespace SpatialiteDatasource
