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

#include "TableInfo.h"

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

    Table(const Table&) = delete;
    Table(Table&&) = default;

    Table& operator=(const Table&) = delete;
    Table& operator=(Table&&) = delete;

    void AddGeometryColumn(const std::string& geometryColumn, const std::string& geometry, int srid = Wgs84Srid);
    void CreateSpatialIndex(SpatialiteDatasource::SpatialIndex spatialIndex);
    [[nodiscard]] const std::string& GetGeometryColumnName() const;
    [[nodiscard]] SpatialiteDatasource::TableInfo& UpdateAndGetTableInfo(
        SpatialiteDatasource::GeometryType geometryType, 
        SpatialiteDatasource::Dimension dimension
    );

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
    std::string name;

private:
    SQLite::Database& m_db;
    std::string m_columnsSql;
    std::optional<GeometryColumn> m_geometryColumn;
    SpatialiteDatasource::TableInfo m_tableInfo;
};