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

#include "GeometriesView.h"
#include "SqlStatements.h"

#include <SQLiteCpp/Database.h>

namespace SpatialiteDatasource {

struct GeometryColumnInfo
{
    std::string name; /// Geometry column name
    int type;         /// Geometry type (POINT, LINESTRING, POLYGON, ...)
};

struct Mbr
{
    double xmin, ymin, xmax, ymax;
};

/**
 * @brief Class for reading spatialite geometries from a database
 */
class Database
{
public:
    /**
     * @brief Construct a new Database object
     * 
     * @param mapPath Path to a spatialite database
     */
    explicit Database(const std::filesystem::path& dbPath);
    ~Database();

    /**
     * @brief Get the info about geometry column of the table
     * 
     * @param tableName Name of the table in db with a geometry column
     * @return Column name and type 
     */
    [[nodiscard]] GeometryColumnInfo GetGeometryColumnInfo(const std::string& tableName) const;

    /**
     * @brief Get the spatial index type of the given table
     */
    [[nodiscard]] SpatialIndex GetSpatialIndexType(const std::string& tableName) const;

    /**
     * @brief Get names of the tables that have a geometry column
     */
    [[nodiscard]] std::vector<std::string> GetTablesNames() const;

    /**
     * @brief Get the path to the database
     */
    [[nodiscard]] const std::string& GetDatabaseFilePath() const;

    /**
     * @brief Get geometries within MBR
     * 
     * @tparam GeomType Type of the geometry @sa GeometryType.h
     * @tparam Dim Dimension of the geometry (2D/3D)
     * @param tableName Name of the table that stores geometries
     * @param geometryColumn Name of the spatialite geometry column in the table
     * @param mbr Minimum bounding rectangle
     * @return Geometry view that iterates over geometries
     */
    template <GeometryType GeomType, Dimension Dim>
    [[nodiscard]] auto GetGeometries(
        const std::string& tableName, 
        const std::string& geometryColumn, 
        const Mbr& mbr) const
    {
        SQLite::Statement stmt{m_db, GetSqlQuery<GeomType, Dim>(
            tableName, geometryColumn, GetSpatialIndexType(tableName))};
        stmt.bind("@xMin", mbr.xmin);
        stmt.bind("@yMin", mbr.ymin);
        stmt.bind("@xMax", mbr.xmax);
        stmt.bind("@yMax", mbr.ymax);
        return GeometriesView<GeomType, Dim>{std::move(stmt)};
    }
private:
    const SQLite::Database m_db;
    void* m_spatialiteCache;
};

} // namespace SpatialiteDatasource
