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

#pragma once

#include "Database.h"
#include "GeometryType.h"
#include "TableInfo.h"
#include "ConfigLoader.h"

#include <mapget/http-datasource/datasource-server.h>
#include <filesystem>

namespace SpatialiteDatasource {

class Datasource
{
public:
    /**
     * @brief Run the datasource server
     */
    void Run();

private:
    explicit Datasource(ConfigLoader&& configLoader);

    friend Datasource CreateDatasourceDefaultConfig(const OverrideOptions& options);
    friend Datasource CreateDatasource(const std::filesystem::path& configPath, const OverrideOptions& options);

    struct FeatureTileMapThreadSafe
    {
        std::shared_mutex lock;
        std::unordered_map<int /* featureId */, mapget::TileId> map;
    };

    [[nodiscard]] std::string GetLayerIdFromTypeId(const std::string& typeId);

    /**
     * @brief Fill the given tile with geometries
     * 
     * @param tile Tile to create geometries in
     */
    void FillTileWithGeometries(const mapget::TileFeatureLayer::Ptr& tile);

    /**
     * @brief Process Mapget '/locate' request
     */
    [[nodiscard]] std::vector<mapget::LocateResponse> LocateFeature(const mapget::LocateRequest& request);

    /**
     * @brief Create geometries on the tile
     * 
     * @param tile Tile to create geometries in
     * @param tableName Table which contains geometries
     * @param geometryColumn Name of the spatialite geometry column of the table
     * @param geometryType Type of the geometries @sa GeometryType.h
     * @param dimension Dimension of the geometries (2D/3D)
     */
    void CreateGeometries(const mapget::TileFeatureLayer::Ptr& tile, const TableInfo& tableInfo);
private:
    Database m_db;
    mapget::DataSourceServer m_ds;
    std::unordered_map<
        std::string, // typeId (table)
        FeatureTileMapThreadSafe> m_featuresTilesByTable;

    const TablesInfo m_tablesInfo;
    const uint16_t m_port = 0;
};

/**
 * @brief Create a datasource with default config
 * 
 * @param options Options that will override the options from the config
 */
Datasource CreateDatasourceDefaultConfig(const OverrideOptions& options);

/**
 * @brief Create a datasource object
 * 
 * @param configPath Path to a config
 * @param options Options that will override the options from the config
 */
Datasource CreateDatasource(const std::filesystem::path& configPath, const OverrideOptions& options);

} // namespace SpatialiteDatasource
