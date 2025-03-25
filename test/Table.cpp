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

#include "Table.h"

#include <ranges>

Table::Table(
    SQLite::Database& db,
    std::string_view name,
    const std::vector<Column>& columns
) 
    : name{name}
    , m_db{db}
{
    const auto sqlColumn = [](const auto& c){ return fmt::format("{} {},\n", c.name, c.type); };
    SQLite::Statement{m_db, fmt::format(R"SQL(
        CREATE TABLE {} (
            {}
            id INTEGER PRIMARY KEY AUTOINCREMENT);
        )SQL", name, fmt::join(columns | std::views::transform(sqlColumn), ""))}.exec();

    m_columnsSql = fmt::to_string(fmt::join(columns | std::views::transform([](const auto& c){ return c.name; }), ", "));
}

Table::~Table()
{
    if (!name.empty())
    {
        if (m_geometryColumn.has_value())
        {
            using SpatialiteDatasource::SpatialIndex;

            switch (m_geometryColumn->indexType)
            {
            case SpatialIndex::RTree:
            case SpatialIndex::MbrCache:
                SQLite::Statement{m_db, fmt::format("SELECT DisableSpatialIndex('{}', '{}');", name, m_geometryColumn->name)}.executeStep();
                break;
            case SpatialIndex::NavInfo:
                RemoveNavInfoIndex();
                break;
            default:
                break;
            }
            SQLite::Statement{m_db, fmt::format("SELECT DiscardGeometryColumn('{}', '{}');", name, m_geometryColumn->name)}.executeStep();
        }
        
        SQLite::Statement{m_db, fmt::format("DROP TABLE {};", name)}.exec();
    }
}

void Table::AddGeometryColumn(const std::string& geometryColumn, const std::string& geometry, int srid)
{
    SQLite::Statement stmt{m_db, 
        "SELECT AddGeometryColumn(@tableName, @geometryColumn, @srid, @geometry);"};
    stmt.bind("@tableName", name);
    stmt.bind("@geometryColumn", geometryColumn);
    stmt.bind("@srid", srid);
    stmt.bind("@geometry", geometry);
    if (!stmt.executeStep() || stmt.getColumn(0).getInt() != 1)
        throw std::runtime_error{"Can't add geometry column"};
    m_geometryColumn.emplace(geometryColumn, SpatialiteDatasource::SpatialIndex::None, geometry, srid);
    m_columnsSql += fmt::format("{}{}", m_columnsSql.empty() ? "" : ", ", m_geometryColumn->name);
}

void Table::CreateSpatialIndex(SpatialiteDatasource::SpatialIndex spatialIndex)
{
    using SpatialiteDatasource::SpatialIndex;

    if (spatialIndex == SpatialIndex::None)
        return;

    std::string stmtStr;
    switch (spatialIndex)
    {
    case SpatialIndex::RTree:
        stmtStr = fmt::format("SELECT CreateSpatialIndex('{}', '{}');", name, m_geometryColumn->name);
        break;
    case SpatialIndex::MbrCache:
        stmtStr = fmt::format("SELECT CreateMbrCache('{}', '{}');", name, m_geometryColumn->name);
        break;
    case SpatialIndex::NavInfo:
        CreateNavInfoIndex();
        return;
    default:
        throw std::runtime_error{"Unknown spatial index"};
    }
    
    SQLite::Statement stmt{m_db, stmtStr};
    stmt.executeStep();
    if (stmt.getColumn(0).getInt() != 1)
        throw std::runtime_error{"Can't create spatial index"};

    m_geometryColumn->indexType = spatialIndex;
}

[[nodiscard]] const std::string& Table::GetGeometryColumnName() const
{
    return m_geometryColumn->name;
}

[[nodiscard]] SpatialiteDatasource::TableInfo& Table::UpdateAndGetTableInfo(
    SpatialiteDatasource::GeometryType geometryType, 
    SpatialiteDatasource::Dimension dimension
)
{
    m_tableInfo.name = name;
    m_tableInfo.geometryColumn = GetGeometryColumnName();
    m_tableInfo.primaryKey = "id";
    m_tableInfo.geometryType = geometryType;
    m_tableInfo.dimension = dimension;
    m_tableInfo.spatialIndex = m_geometryColumn->indexType;
    return m_tableInfo;
}
