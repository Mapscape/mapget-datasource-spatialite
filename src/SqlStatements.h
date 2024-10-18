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

#include "AttributesInfo.h"
#include "GeometryType.h"
#include "NavInfoIndex.h"

#include <fmt/format.h>

#include <string_view>
#include <stdexcept>

namespace SpatialiteDatasource {

namespace Detail {

inline std::string GetMbrCondition(const std::string& tableName, const std::string& geometryColumn, SpatialIndex spatialIndex)
{
    switch (spatialIndex)
    {
    case SpatialIndex::None:
        return fmt::format("Intersects({}, BuildMbr(@xMin, @yMin, @xMax, @yMax))", geometryColumn);
    case SpatialIndex::RTree:
        return fmt::format(R"SQL(
            rowid IN (
                SELECT rowid 
                FROM SpatialIndex
                WHERE f_table_name = '{}' 
                    AND search_frame = BuildMbr(@xMin, @yMin, @xMax, @yMax))
        )SQL", tableName);
    case SpatialIndex::MbrCache:
        return fmt::format(R"SQL(
            rowid IN (
                SELECT rowid 
                FROM cache_{}_{}
                WHERE mbr = FilterMbrIntersects(@xMin, @yMin, @xMax, @yMax))
        )SQL", tableName, geometryColumn);
    case SpatialIndex::NavInfo:
        return GetNavInfoIndexMbrCondition(tableName);
    default:
        throw std::logic_error{"Unknown spatial index type"};
    }
}

inline std::string GetAttributesList(const AttributesInfo& attributesInfo)
{
    std::string result;
    for (const auto& attributeInfo : attributesInfo)
    {
        result += attributeInfo.name + ", ";
    }
    return result;
}

} // namespace Detail

/**
 * @brief Get an sql query for obtaining geometries from the table
 * 
 * @param tableName Table which contains geometries
 * @param primaryKey Primary key column name of the table
 * @param geometryColumn Name of the spatialite geometry column of the table
 * @param attributesInfo Additional attributes info
 * @param spatialIndex Spatial index type to use
 * @return SQL query as std::string
 */
inline std::string GetSqlQuery(
    const std::string& tableName, 
    const std::string& primaryKey, 
    const std::string& geometryColumn, 
    const AttributesInfo& attributesInfo, 
    SpatialIndex spatialIndex
)
{
    using namespace fmt::literals;

    return fmt::format(R"SQL(
            SELECT
                {geometryColumn} as geometry,
                {attributes}
                {primaryKey} AS id
            FROM {tableName}
            WHERE {mbrCondition};
        )SQL", 
        "tableName"_a=tableName,
        "primaryKey"_a=primaryKey,
        "geometryColumn"_a=geometryColumn,
        "attributes"_a=Detail::GetAttributesList(attributesInfo),
        "mbrCondition"_a=Detail::GetMbrCondition(tableName, geometryColumn, spatialIndex));
}

} // namespace SpatialiteDatasource
