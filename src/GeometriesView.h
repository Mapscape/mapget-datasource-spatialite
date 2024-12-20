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

#include "GeometryType.h"
#include "IFeature.h"
#include "TableInfo.h"

#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Column.h>

#include <sqlite3.h>
#include <spatialite.h>

namespace SpatialiteDatasource {
namespace Detail {

template <typename T>
concept LinelikeGeometryPtr = (std::same_as<T, gaiaLinestringPtr> || std::same_as<T, gaiaRingPtr>);

} // namespace Detail

class Geometry
{
public:
    Geometry(
        GeometryType geometryType, 
        Dimension dimension,
        const SQLite::Statement& stmt,
        const TableInfo& tableInfo
    ) noexcept;

    /**
     * @brief Get the id of the geometry (primary key)
     * 
     * @return Geometry id
     */
    [[nodiscard]] int GetId() const;

    /**
     * @brief Add the geometry and it's attributes to the given feature
     */
    void AddTo(IFeature& feature);
    
private:
    void AddAttributesTo(IFeature& feature);
    void AddPointTo(gaiaPointPtr point, IFeature& feature);
    void AddMultiPointTo(gaiaPointPtr firstPoint, IFeature& feature);
    void AddMultiLineTo(gaiaLinestringPtr firstLine, IFeature& feature);
    void AddMultiPolygonTo(gaiaPolygonPtr firstPolygon, IFeature& feature);

    template <Detail::LinelikeGeometryPtr T>
    void AddLineOrPolygonTo(T gaiaGeometry, IFeature& feature)
    {
        auto geometry = feature.AddGeometry(m_geometryType, gaiaGeometry->Points);
        const auto& scaling = m_tableInfo.scaling;
        for (int i = 0; i < gaiaGeometry->Points; ++i)
        {
            double x, y, z, m;
            switch (m_dimension)
            {
            case Dimension::XY:
                gaiaGetPoint(gaiaGeometry->Coords, i, &x, &y);
                geometry->AddPoint({x * scaling.x, y * scaling.y}); 
                break;
            case Dimension::XYM:
                gaiaGetPointXYM(gaiaGeometry->Coords, i, &x, &y, &m);
                geometry->AddPoint({x * scaling.x, y * scaling.y}); 
                break;
            case Dimension::XYZ:
                gaiaGetPointXYZ(gaiaGeometry->Coords, i, &x, &y, &z);
                geometry->AddPoint({x * scaling.x, y * scaling.y, z * scaling.z});
                break;
            case Dimension::XYZM:
                gaiaGetPointXYZM(gaiaGeometry->Coords, i, &x, &y, &z, &m);
                geometry->AddPoint({x * scaling.x, y * scaling.y, z * scaling.z});
                break;
            }
        }
    }

private:
    const GeometryType m_geometryType;
    const Dimension m_dimension;
    const SQLite::Statement& m_stmt;
    const TableInfo& m_tableInfo;
};

class GeometryIterator
{
public:
    GeometryIterator() noexcept;

    GeometryIterator(
        GeometryType geometryType, 
        Dimension dimension, 
        SQLite::Statement& stmt, 
        const TableInfo& tableInfo
    ) noexcept;

    GeometryIterator& operator++() noexcept;
    [[nodiscard]] Geometry operator*() const noexcept;
    [[nodiscard]] bool operator==(const GeometryIterator& other) const noexcept;

private:
    const GeometryType m_geometryType;
    const Dimension m_dimension;
    SQLite::Statement* m_stmt;
    const TableInfo* const m_tableInfo;
};

/**
 * @brief View that allows iterating over geometries from a spatialite table
 */
class GeometriesView
{
public:
    GeometriesView(
        GeometryType geometryType, 
        Dimension dimension, 
        SQLite::Statement&& stmt, 
        const TableInfo& tableInfo
    ) noexcept;

    GeometryIterator begin();
    GeometryIterator end() const noexcept;

private:
    const GeometryType m_geometryType;
    const Dimension m_dimension;
    SQLite::Statement m_stmt;
    const TableInfo& m_tableInfo;
};

} // namespace SpatialiteDatasource
