// Copyright (C) 2024 by NavInfo Europe B.V. The Netherlands - All rights reserved
// Information classification: Confidential
// This content is protected by international copyright laws.
// Reproduction and distribution is prohibited without written permission.

#pragma once

#include <GeometryType.h>
#include <SQLiteCpp/Database.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <vector>
#include <optional>

static constexpr int Wgs84Srid = 4326;

struct Column
{
    std::string name;
    std::string type;
};

struct GeometryColumn
{
    std::string name;
    SpatialiteDatasource::SpatialIndex indexType;
    std::string geometryType;
    int srid;
};

struct Binary
{
    const std::string& string;
};

struct Geometry : Binary {};

template <class T>
std::string FormatSqlValue(const T& value)
{
    if constexpr (std::is_arithmetic_v<T>)
        return std::to_string(value);
    else if constexpr (std::is_convertible_v<T, std::string>)
        return fmt::format("'{}'", value);
    else if constexpr (std::is_same_v<T, Binary>)
        return fmt::format("X'{}'", value.string);
    else if constexpr (std::is_same_v<T, Geometry>)
        return fmt::format("GeomFromText('{}', @srid)", value.string);
    else
        static_assert(!sizeof(T), "Unknown value type");
}

class Table
{
public:
    Table(SQLite::Database& db, std::string_view name, const std::vector<Column>& columns);
    ~Table();

    void AddGeometryColumn(const std::string& geometryColumn, const std::string& geometry, int srid = Wgs84Srid);
    void CreateSpatialIndex(SpatialiteDatasource::SpatialIndex spatialIndex);
    [[nodiscard]] const std::string& GetGeometryColumnName() const;

    template <class... Values>
    void Insert(Values&&...values)
    {
        // perhaps not effective, but simple and beautiful
        SQLite::Statement stmt{
            m_db, fmt::format(
                      "INSERT INTO {} ({}) VALUES ({});", name, m_columnsSql,
                      fmt::join(std::vector{FormatSqlValue(values)...}, ", "))};
        if (m_geometryColumn.has_value())
            stmt.bind("@srid", m_geometryColumn->srid);
        if (stmt.exec() != 1)
            throw std::runtime_error{"Can't insert values into table"};
    }

private:
    void CreateNavInfoIndex();
    void RemoveNavInfoIndex();

public:
    const std::string name;

private:
    SQLite::Database& m_db;
    std::string m_columnsSql;
    std::optional<GeometryColumn> m_geometryColumn;
};