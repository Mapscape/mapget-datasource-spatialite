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

#include <GeometryType.h>
#include <boost/scope/scope_exit.hpp>
#include <SQLiteCpp/Database.h>
#include <fmt/format.h>

#include <filesystem>
#include <stdexcept>
#include <vector>

template <class L>
class Table
{
public:
    Table(
        const std::string& name, 
        const std::string geometryColumn, 
        boost::scope::scope_exit<L>&& guard
    ) 
        : name{name}
        , geometryColumn{geometryColumn}
        , m_guard{std::move(guard)} 
    {}

public:
    std::string name;
    std::string geometryColumn;

private:
    boost::scope::scope_exit<L> m_guard;
};

/**
 * @brief Class for creating a spatialite database
 */
class TestDbDriver
{
    static constexpr int Wgs84Srid = 4326;
public:
    TestDbDriver();

    ~TestDbDriver();

    auto CreateTable(const std::string& geometry, SpatialiteDatasource::SpatialIndex spatialIndex, int srid = Wgs84Srid)
    {
        SQLite::Statement{m_db, "CREATE TABLE tbl (id INT PRIMARY KEY);"}.exec();

        Table table{"tbl", "geometry", boost::scope::make_scope_exit(
            [this] {
                SQLite::Statement{m_db, "SELECT DisableSpatialIndex('tbl', 'geometry');"}.executeStep();
                RemoveNavInfoIndex();
                SQLite::Statement{m_db, "SELECT DiscardGeometryColumn('tbl', 'geometry');"}.executeStep();
                SQLite::Statement{m_db, "DROP TABLE tbl;"}.exec();
        })};

        AddGeometryColumn(geometry, srid);
        CreateSpatialIndex(spatialIndex, geometry);

        return std::move(table);
    }

    void Insert(const std::vector<std::string>& geometries, int srid = Wgs84Srid);

    auto CreateTableWithGeometries(
        const std::string& geometry, 
        SpatialiteDatasource::SpatialIndex spatialIndex,
        const std::vector<std::string>& geometries)
    {
        auto table = CreateTable(geometry, spatialIndex);
        Insert(geometries);
        return std::move(table);
    }

    [[nodiscard]] const std::filesystem::path& GetPath() const noexcept;

private:
    void AddGeometryColumn(const std::string& geometry, int srid);
    void CreateSpatialIndex(
        SpatialiteDatasource::SpatialIndex spatialIndex, 
        const std::string& geometry);

    void InitNavInfoMetaData();
    void CreateNavInfoIndex(const std::string& geometry);
    void RemoveNavInfoIndex();
private:
    const std::filesystem::path m_dbPath;
    SQLite::Database m_db;
    void* m_spatialiteCache;
};