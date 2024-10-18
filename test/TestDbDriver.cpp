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

#include "TestDbDriver.h"

#include <sqlite3.h>
#include <spatialite.h>

TestDbDriver::TestDbDriver()
    : m_dbPath{std::filesystem::temp_directory_path() / std::tmpnam(nullptr)}
    , m_db{m_dbPath, SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE}
{
    m_spatialiteCache = spatialite_alloc_connection();
    spatialite_init_ex(m_db.getHandle(), m_spatialiteCache, 0);

    SQLite::Statement stmt{m_db, "SELECT InitSpatialMetaData(1);"};
    stmt.executeStep();
    if (stmt.getColumn(0).getInt() != 1)
        throw std::runtime_error{"Can't initialize spatial meta data"};

    InitNavInfoMetaData();
}

TestDbDriver::~TestDbDriver()
{
    spatialite_cleanup_ex(m_spatialiteCache);
}

void TestDbDriver::Insert(const std::vector<std::string>& geometries, int srid)
{
    std::string stmtStr = "INSERT INTO tbl (id, geometry, intAttribute, doubleAttribute, stringAttribute, blobAttribute) VALUES";
    for (size_t i = 0; i < geometries.size(); ++i)
    {
        stmtStr += fmt::format(" ({}, GeomFromText('{}', @srid), 42, 6.66, 'value', X'deadbeef'),", i + 1, geometries[i]);
    }
    stmtStr.back() = ';';

    SQLite::Statement stmt{m_db, stmtStr};
    stmt.bind("@srid", srid);
    if (stmt.exec() != geometries.size())
        throw std::runtime_error{"Can't insert geometries into table"};
}

const std::filesystem::path& TestDbDriver::GetPath() const noexcept
{
    return m_dbPath;
}

void TestDbDriver::AddGeometryColumn(const std::string& geometry, int srid)
{
    SQLite::Statement stmt{m_db, 
        "SELECT AddGeometryColumn('tbl', 'geometry', @srid, @geometry);"};
    stmt.bind("@geometry", geometry);
    stmt.bind("@srid", srid);
    stmt.executeStep();
    if (stmt.getColumn(0).getInt() != 1)
        throw std::runtime_error{"Can't add geometry column"};
}

void TestDbDriver::CreateSpatialIndex(
    SpatialiteDatasource::SpatialIndex spatialIndex, 
    const std::string& geometry)
{
    using SpatialiteDatasource::SpatialIndex;

    if (spatialIndex == SpatialIndex::None)
        return;

    std::string stmtStr;
    switch (spatialIndex)
    {
    case SpatialIndex::RTree:
        stmtStr = "SELECT CreateSpatialIndex('tbl', 'geometry');";
        break;
    case SpatialIndex::MbrCache:
        stmtStr = "SELECT CreateMbrCache('tbl', 'geometry');";
        break;
    case SpatialIndex::NavInfo:
        CreateNavInfoIndex(geometry);
        return;
    default:
        throw std::runtime_error{"Unknown spatial index"};
    }
    
    SQLite::Statement stmt{m_db, stmtStr};
    stmt.executeStep();
    if (stmt.getColumn(0).getInt() != 1)
        throw std::runtime_error{"Can't create spatial index"};
}
