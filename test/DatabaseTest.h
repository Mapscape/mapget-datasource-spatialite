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

#include "TestDbDriver.h"

#include <Database.h>

#include <gtest/gtest.h>

class SpatialiteDatabaseTest : public testing::Test
{
public:
    SpatialiteDatabaseTest()
        : spatialiteDb{testDb->GetPath()}
    {
    }

    static void SetUpTestSuite()
    {
        testDb = std::make_unique<TestDbDriver>();
    }

    static void TearDownTestSuite()
    {
        const auto path = testDb->GetPath();
        testDb.reset();
        std::filesystem::remove(path);
    }

    template <SpatialiteDatasource::GeometryType GeomType, SpatialiteDatasource::Dimension Dim, class L>
    [[nodiscard]] auto GetGeometries(const Table<L>& table)
    {
        return spatialiteDb.GetGeometries<GeomType, Dim>(
            table.name, table.geometryColumn, mbr);
    }

    SpatialiteDatasource::Database spatialiteDb;

    static constexpr SpatialiteDatasource::Mbr mbr{0, 0, 100, 100};
    static std::unique_ptr<TestDbDriver> testDb;
};