// Copyright (C) 2024 by NavInfo Europe B.V. The Netherlands - All rights reserved
// Information classification: Confidential
// This content is protected by international copyright laws.
// Reproduction and distribution is prohibited without written permission.

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
