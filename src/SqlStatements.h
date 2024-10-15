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
        return fmt::format("Intersects(t.{}, BuildMbr(@xMin, @yMin, @xMax, @yMax))", geometryColumn);
    case SpatialIndex::RTree:
        return fmt::format(R"SQL(
            t.rowid IN (
                SELECT rowid 
                FROM SpatialIndex
                WHERE f_table_name = '{}' 
                    AND search_frame = BuildMbr(@xMin, @yMin, @xMax, @yMax))
        )SQL", tableName);
    case SpatialIndex::MbrCache:
        return fmt::format(R"SQL(
            t.rowid IN (
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

inline std::string GetZCoord(const std::string& geometryColumn, GeometryType geometryType)
{
    switch (geometryType)
    {
    case GeometryType::Point:
        return fmt::format("Z({}),", geometryColumn);
    case GeometryType::Line:
        return fmt::format("Z(PointN({}, n)),", geometryColumn);
    case GeometryType::Polygon:
        return fmt::format("Z(PointN(ExteriorRing({}), n)),", geometryColumn);
    case GeometryType::MultiPoint:
        return "Z(e.geometry),";
    case GeometryType::MultiLine:
    case GeometryType::MultiPolygon:
        return "Z(PointN(elemGeometry, n)),";
    default:
        throw std::logic_error{"Can't find Z coordinate statement: unknown geometry type"};
    }

}

consteval std::string_view GetExteriorRingFunc(GeometryType geometryType)
{
    if (geometryType == GeometryType::Polygon || geometryType == GeometryType::MultiPolygon)
        return "ExteriorRing";
    return "";
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

consteval std::string_view GetSqlQueryTemplate(GeometryType geometryType)
{
    switch (geometryType)
    {
    case GeometryType::Point:
        return R"SQL(
            SELECT
                X({geometryColumn}),
                Y({geometryColumn}),
                {zCoord}
                {attributes}
                {primaryKey} AS id
            FROM {tableName} AS t
            WHERE {mbrCondition};
        )SQL";

    case GeometryType::Line:
    case GeometryType::Polygon:
        return R"SQL(
            WITH RECURSIVE n_max AS (
                SELECT max(NumPoints({exteriorRing}({geometryColumn}))) AS m FROM {tableName}
            ),
            nlist AS (
                SELECT 1 AS n
                UNION ALL
                SELECT n + 1 FROM nlist, n_max WHERE n < m
            )
            SELECT
                X(PointN({exteriorRing}({geometryColumn}), n)),
                Y(PointN({exteriorRing}({geometryColumn}), n)),
                {zCoord}
                {attributes}
                NumPoints({exteriorRing}({geometryColumn})) AS pointsCount,
                {primaryKey} AS id
            FROM {tableName} AS t
            CROSS JOIN nlist
            WHERE PointN({exteriorRing}({geometryColumn}), n) IS NOT NULL
                AND {mbrCondition};
        )SQL";

    case GeometryType::MultiPoint:
        return R"SQL(
            SELECT
                X(e.geometry),
                Y(e.geometry),
                {zCoord}
                {attributes}
                t.{primaryKey} AS id,
                NumGeometries(t.{geometryColumn}) as pointsCount
            FROM {tableName} AS t, ElementaryGeometries AS e 
            WHERE e.f_table_name = '{tableName}' 
                AND e.origin_rowid = t.rowid
                AND {mbrCondition};
        )SQL";

    case GeometryType::MultiLine:
    case GeometryType::MultiPolygon:
        return R"SQL(
            WITH RECURSIVE elementary_geometries AS (
                SELECT
                    t.{primaryKey} AS id,
                    {attributes}
                    NumGeometries(t.{geometryColumn}) AS linesCount,
                    {exteriorRing}(e.geometry) AS elemGeometry
                FROM {tableName} AS t, ElementaryGeometries AS e 
                WHERE e.f_table_name = '{tableName}' 
                    AND e.origin_rowid = t.rowid
                    AND {mbrCondition}
            ),
            n_max AS (
                SELECT max(NumPoints(elemGeometry)) AS m FROM elementary_geometries
            ),
            nlist AS (
                SELECT 1 AS n
                UNION ALL
                SELECT n + 1 FROM nlist, n_max WHERE n < m
            )
            SELECT
                X(PointN(elemGeometry, n)),
                Y(PointN(elemGeometry, n)),
                {zCoord}
                {attributes}
                NumPoints(elemGeometry) AS pointsCount,
                linesCount,
                id
            FROM elementary_geometries
            CROSS JOIN nlist
            WHERE PointN(elemGeometry, n) IS NOT NULL;
        )SQL";

    default:
        throw std::logic_error{"Can't find sql statement template: unknown geometry type"};
    }
}

} // namespace Detail

/**
 * @brief Get an sql query for obtaining geometries from the table
 * 
 * @tparam GeomType Type of the geometries @sa GeometryType.h
 * @tparam Dim Dimension of the geometries (2D/3D)
 * @param tableName Table which contains geometries
 * @param geometryColumn Name of the spatialite geometry column of the table
 * @return SQL query as std::string
 */

/**
 * @brief Get an sql query for obtaining geometries from the table
 * 
 * @tparam GeomType Type of the geometries @sa GeometryType.h
 * @tparam Dim Dimension of the geometries (2D/3D)
 * @param tableName Table which contains geometries
 * @param primaryKey Primary key column name of the table
 * @param geometryColumn Name of the spatialite geometry column of the table
 * @param attributesInfo Additional attributes info
 * @param spatialIndex Spatial index type to use
 * @return SQL query as std::string
 */
template <GeometryType GeomType, Dimension Dim>
std::string GetSqlQuery(
    const std::string& tableName, 
    const std::string& primaryKey, 
    const std::string& geometryColumn, 
    const AttributesInfo& attributesInfo, 
    SpatialIndex spatialIndex
)
{
    using namespace fmt::literals;

    return fmt::format(Detail::GetSqlQueryTemplate(GeomType), 
        "tableName"_a=tableName,
        "primaryKey"_a=primaryKey,
        "geometryColumn"_a=geometryColumn,
        "exteriorRing"_a=Detail::GetExteriorRingFunc(GeomType),
        "zCoord"_a=(Dim == Dimension::D3 ? Detail::GetZCoord(geometryColumn, GeomType) : ""),
        "attributes"_a=Detail::GetAttributesList(attributesInfo),
        "mbrCondition"_a=Detail::GetMbrCondition(tableName, geometryColumn, spatialIndex));
}

} // namespace SpatialiteDatasource
