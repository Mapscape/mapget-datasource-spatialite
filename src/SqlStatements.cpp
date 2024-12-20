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

#include "SqlStatements.h"
#include "NavInfoIndex.h"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <stdexcept>
#include <unordered_set>

namespace SpatialiteDatasource {

namespace {

std::string GetMbrCondition(const std::string& tableName, const std::string& geometryColumn, SpatialIndex spatialIndex)
{
    switch (spatialIndex)
    {
    case SpatialIndex::None:
        return fmt::format("Intersects(layerTable.{}, BuildMbr(@xMin, @yMin, @xMax, @yMax))", geometryColumn);
    case SpatialIndex::RTree:
        return fmt::format(R"SQL(
            layerTable.rowid IN (
                SELECT rowid 
                FROM SpatialIndex
                WHERE f_table_name = '{}' 
                    AND search_frame = BuildMbr(@xMin, @yMin, @xMax, @yMax))
        )SQL", tableName);
    case SpatialIndex::MbrCache:
        return fmt::format(R"SQL(
            layerTable.rowid IN (
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

std::string GetAttributesList(const AttributesInfo& attributesInfo)
{
    std::string result;
    for (const auto& [name, info] : attributesInfo)
    {
        if (info.relation.has_value())
        {
            const auto& relation = info.relation.value();
            if (relation.columns.size() == 1)
            {
                result += fmt::format("{} AS {}", relation.columns[0], name);
            }
            else
            {
                result += fmt::format("{} AS {}",
                    fmt::join(relation.columns, fmt::format(" || '{}' || ", relation.delimiter)), name);
            }
        }
        else
        {
            result += name;
        }
        result += ", ";
    }
    return result;
}

std::string GetAttributesRelatedTables(const AttributesInfo& attributesInfo)
{
    std::unordered_set<std::string> uniqueTables;
    std::string result;
    for (const auto& [name, info] : attributesInfo)
    {
        if (info.relation.has_value())
        {
            for (const auto& column : info.relation->columns)
            {
                const auto table = column.substr(0, column.find('.'));
                const auto [unused, isInserted] = uniqueTables.insert(table);
                if (isInserted)
                    result += ", " + table;
            }
        }
    }
    return result;
}

std::string GetAttributesMatchCondition(const AttributesInfo& attributesInfo)
{
    std::string result;
    for (const auto& [name, info] : attributesInfo)
    {
        if (info.relation.has_value())
        {
            result += fmt::format(" AND ({})", info.relation->matchCondition);
        }
    }
    return result;
}

} // namespace

std::string GetSqlQuery(
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
                layerTable.{geometryColumn} as __geometry,
                {attributes}
                layerTable.{primaryKey} AS __id
            FROM {tableName} AS layerTable{attributesRelatedTables}
            WHERE {mbrCondition}{attributesMatchCondition};
        )SQL", 
        "tableName"_a=tableName,
        "primaryKey"_a=primaryKey,
        "geometryColumn"_a=geometryColumn,
        "mbrCondition"_a=GetMbrCondition(tableName, geometryColumn, spatialIndex),
        "attributes"_a=GetAttributesList(attributesInfo),
        "attributesRelatedTables"_a=GetAttributesRelatedTables(attributesInfo),
        "attributesMatchCondition"_a=GetAttributesMatchCondition(attributesInfo)
    );
}

} // namespace SpatialiteDatasource
