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
#include "TableInfo.h"
#include "GeometryType.h"
#include "NavInfoIndex.h"

#include <mapget/log.h>
#include <sqlite3.h>
#include <spatialite.h>
#include <fmt/format.h>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <stdexcept>

namespace SpatialiteDatasource {

Database::Database(const std::filesystem::path& dbPath)
    : m_db{dbPath}
{
    m_spatialiteCache = spatialite_alloc_connection();
    spatialite_init_ex(m_db.getHandle(), m_spatialiteCache, 0);

    for (const auto& table : GetTablesNames())
    {
        m_primaryKeys[table] = GetPrimaryKeyColumnName(table);
    }
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

[[nodiscard]] AttributesInfo Database::GetTableAttributes(const std::string& tableName) const
{
    AttributesInfo info;
    const auto tableNameLower = boost::to_lower_copy(tableName);

    const auto pkNameIter = m_primaryKeys.find(tableNameLower);
    if (pkNameIter == m_primaryKeys.end())
        throw std::runtime_error{fmt::format("Can't find primary key column name for table '{}'", tableName)};
    const auto geometryColumn = GetGeometryColumnName(tableNameLower);

    // PRAGMA_TABLE_INFO gives unreliable results that may differ 
    // from version to version and depends on the way a table was created,
    // so it's easier to get a row from the table and check types by sqlite API
    SQLite::Statement stmt{m_db, fmt::format(R"SQL(
        SELECT * FROM {} LIMIT 1;
    )SQL", tableNameLower)};
    if (stmt.executeStep()) // if no - table is empty, return empty info
    {
        for (int i = 0; i < stmt.getColumnCount(); ++i)
        {
            const auto column = stmt.getColumn(i);
            const std::string name = column.getName();
            // skip id and geometry
            if (boost::iequals(name, pkNameIter->second) || boost::iequals(name, geometryColumn))
                continue;
            info[name] = {.type = ColumnTypeFromSqlType(column.getType())};
        }
    }
    return info;
}

[[nodiscard]] ColumnType Database::GetColumnType(const std::string& tableName, const std::string& columnName) const
{
    SQLite::Statement stmt{m_db, fmt::format(R"SQL(
        SELECT {} FROM {} LIMIT 1;
    )SQL", columnName, tableName)};
    if (!stmt.executeStep())
    {
        mapget::log().error("Failed to detect column type of '{}' from table '{}'. Falling back with blob type",
                            columnName, tableName);
        return ColumnType::Blob;
    }
    return ColumnTypeFromSqlType(stmt.getColumn(0).getType());
}

[[nodiscard]] GeometriesView Database::GetGeometries(
    const std::string& tableName, 
    const std::string& geometryColumn,
    GeometryType geometryType,
    Dimension dimension,
    const TableInfo& tableInfo,
    const Mbr& mbr) const
{
    const auto indexType = GetSpatialIndexType(tableName);
    SQLite::Statement stmt{m_db, GetSqlQuery(
        tableName, 
        m_primaryKeys.at(tableName), 
        geometryColumn, 
        tableInfo.attributes, 
        indexType)
    };
    double xScaling = 1;
    double yScaling = 1;
    // NavInfo index always works with the original coordinates,
    // so we must not scale MBR coordinates here
    if (indexType != SpatialIndex::NavInfo)
    {
        xScaling = tableInfo.scaling.x;
        yScaling = tableInfo.scaling.y;
    }
    stmt.bind("@xMin", mbr.xmin / xScaling);
    stmt.bind("@yMin", mbr.ymin / yScaling);
    stmt.bind("@xMax", mbr.xmax / xScaling);
    stmt.bind("@yMax", mbr.ymax / yScaling);
    mapget::log().debug("Getting geometries with an SQL query: {}", stmt.getExpandedSQL());
    return GeometriesView{geometryType, dimension, std::move(stmt), tableInfo};
}

[[nodiscard]] std::string Database::GetPrimaryKeyColumnName(const std::string& tableName) const
{
    SQLite::Statement stmt{m_db, fmt::format(R"SQL(
        SELECT name FROM PRAGMA_TABLE_INFO('{}') WHERE pk = 1;
    )SQL", tableName)};
    if (!stmt.executeStep()) [[unlikely]]
    {
        mapget::log().warn("Can't find primary key column for table '{}'. Trying to use 'id'...", tableName);
        SQLite::Statement idStmt{m_db, fmt::format(R"SQL(
            SELECT name FROM PRAGMA_TABLE_INFO('{}') WHERE LOWER(name) = 'id';
        )SQL", tableName)};
        if (!idStmt.executeStep())
        {
            mapget::log().warn("Can't find primary key column for table '{}'. Using 'rowid' instead", tableName);
            return "rowid";
        }
        return idStmt.getColumn(0);
    }
    return stmt.getColumn(0);
}

[[nodiscard]] std::string Database::GetGeometryColumnName(const std::string& tableName) const
{
    SQLite::Statement stmt{m_db, R"SQL(
        SELECT f_geometry_column FROM geometry_columns WHERE f_table_name = ?;
    )SQL"};
    stmt.bind(1, tableName);
    if (!stmt.executeStep()) [[unlikely]]
        throw std::runtime_error{fmt::format("Failed to get a geometry column name for table '{}'", tableName)};
    return stmt.getColumn(0);
}

} // namespace SpatialiteDatasource
