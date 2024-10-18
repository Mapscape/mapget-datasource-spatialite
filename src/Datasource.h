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

#include "Database.h"
#include "GeometryType.h"
#include "MapgetFeature.h"
#include "AttributesInfo.h"

#include <mapget/http-datasource/datasource-server.h>

namespace SpatialiteDatasource {

class Datasource
{
public:
    /**
     * @brief Construct a new Datasource object
     * 
     * @param mapPath Path to a spatialite database
     * @param jsonInfoPath Path to a json file which contains datasource info
     * @param port Port which the HTTP server shall bind to.
     *  With port=0, a random free port will be chosen.
     */
    Datasource(const std::filesystem::path& mapPath, const std::filesystem::path& jsonInfoPath, uint16_t port);

    /**
     * @brief Enable additional feature attributes. The attributes info will be loaded from the database
     */
    void EnableAttributes();

    /**
     * @brief Enable additional feature attributes using attributes info json
     * 
     * @param attributesInfoPath Path to a json file which contains info about additional feature attributes
     */
    void EnableAttributesWithInfoJson(const std::filesystem::path& attributesInfoPath);

    /**
     * @brief Run the datasource server
     */
    void Run();

private:
    [[nodiscard]] static mapget::DataSourceInfo LoadDataSourceInfoFromJson(const std::filesystem::path& jsonInfoPath);
    [[nodiscard]] static mapget::DataSourceInfo LoadDataSourceInfoFromDatabase(const Database& db);

    /**
     * @brief Fill the given tile with geometries
     * 
     * @param tile Tile to create geometries in
     */
    void Fill(const mapget::TileFeatureLayer::Ptr& tile) const;

    /**
     * @brief Create geometries on the tile
     * 
     * @param tile Tile to create geometries in
     * @param tableName Table which contains geometries
     * @param geometryColumn Name of the spatialite geometry column of the table
     * @param geometryType Type of the geometries @sa GeometryType.h
     * @param dimension Dimension of the geometries (2D/3D)
     */
    void CreateGeometries(
        const mapget::TileFeatureLayer::Ptr& tile, 
        const std::string& tableName, 
        const std::string& geometryColumn,
        GeometryType geometryType,
        Dimension dimension) const;
private:
    Database m_db;
    mapget::DataSourceServer m_ds;
    mutable TablesAttributesInfo m_attributesInfo; // mutable because empty entries can be added
    const uint16_t m_port = 0;
};

} // namespace SpatialiteDatasource
