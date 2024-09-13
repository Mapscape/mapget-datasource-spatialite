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

#include "Database.h"
#include "GeometryType.h"
#include "NavInfoIndex.h"

#include <mapget/log.h>
#include <sqlite3.h>
#include <spatialite.h>
#include <fmt/format.h>
#include <boost/algorithm/string/case_conv.hpp>

#include <stdexcept>

namespace SpatialiteDatasource {

Database::Database(const std::filesystem::path& dbPath)
    : m_db{dbPath}
{
    m_spatialiteCache = spatialite_alloc_connection();
    spatialite_init_ex(m_db.getHandle(), m_spatialiteCache, 0);
}
Database::~Database()
{
    spatialite_cleanup_ex(m_spatialiteCache);
}

[[nodiscard]] GeometryColumnInfo Database::GetGeometryColumnInfo(const std::string& tableName) const
{
    SQLite::Statement stmt{m_db, R"SQL(
        SELECT f_geometry_column, geometry_type, srid FROM geometry_columns WHERE f_table_name = ?;
    )SQL"};
    stmt.bind(1, boost::to_lower_copy(tableName));
    if (!stmt.executeStep())
    {
        throw std::runtime_error{fmt::format("Table '{}' is not in 'geometry_columns'", tableName)};
    }
    constexpr int Wgs84Srid = 4326;
    if (stmt.getColumn(2).getInt() != Wgs84Srid)
    {
        throw std::runtime_error{fmt::format("Geometry column of '{}' table is not in WGS84", tableName)};
    }
    return stmt.getColumns<GeometryColumnInfo, 2>();
}

[[nodiscard]] SpatialIndex Database::GetSpatialIndexType(const std::string& tableName) const
{
    if (IsNavInfoIndexAvailable(m_db, tableName))
    {
        mapget::log().debug("NavInfo spatial index found for table '{}'", tableName);
        return SpatialIndex::NavInfo;
    }
    SQLite::Statement stmt{m_db, R"SQL(
        SELECT spatial_index_enabled FROM geometry_columns WHERE f_table_name = ?;
    )SQL"};
    stmt.bind(1, boost::to_lower_copy(tableName));
    if (!stmt.executeStep())
    {
        throw std::runtime_error{fmt::format("Table '{}' is not in 'geometry_columns'", tableName)};
    }
    switch (stmt.getColumn(0).getInt())
    {
    case 0:
        mapget::log().warn("No spatial index found for table '{}'", tableName);
        return SpatialIndex::None;
    case 1:
        mapget::log().debug("R*Tree spatial index found for table '{}'", tableName);
        return SpatialIndex::RTree;
    case 2:
        mapget::log().debug("MBRCache found for table '{}'", tableName);
        return SpatialIndex::MbrCache;
    default:
        throw std::runtime_error{"Unknown spatial index type"};
    }
}

[[nodiscard]] std::vector<std::string> Database::GetTablesNames() const
{
    std::vector<std::string> tables;
    SQLite::Statement stmt{m_db, R"SQL(
        SELECT f_table_name FROM geometry_columns;
    )SQL"};
    while (stmt.executeStep())
    {
        tables.emplace_back(stmt.getColumn(0));
    }
    return tables;
}

[[nodiscard]] const std::string& Database::GetDatabaseFilePath() const
{
    return m_db.getFilename();
}

} // namespace SpatialiteDatasource
