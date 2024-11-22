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

[[nodiscard]] Table TestDbDriver::CreateTable(std::string_view tableName, const std::vector<Column>& columns)
{
    return {m_db, tableName, columns};
}

const std::filesystem::path& TestDbDriver::GetPath() const noexcept
{
    return m_dbPath;
}
