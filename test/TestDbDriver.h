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

#include "Table.h"

#include <SQLiteCpp/Database.h>

#include <filesystem>
#include <vector>

/**
 * @brief Class for creating a spatialite database
 */
class TestDbDriver
{
public:
    TestDbDriver();
    ~TestDbDriver();

    [[nodiscard]] Table CreateTable(std::string_view tableName, const std::vector<Column>& columns);
    [[nodiscard]] const std::filesystem::path& GetPath() const noexcept;

private:
    void InitNavInfoMetaData();

private:
    const std::filesystem::path m_dbPath;
    SQLite::Database m_db;
    void* m_spatialiteCache;
};
