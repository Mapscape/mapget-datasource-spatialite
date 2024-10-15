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

#include "IFeature.h"
#include "AttributesInfo.h"

#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Column.h>

#include <stdexcept>

namespace SpatialiteDatasource {
namespace Detail {

template <GeometryType GeomType, Dimension Dim>
class Geometry
{
public:
    explicit Geometry(SQLite::Statement& stmt, const AttributesInfo& attributes) noexcept 
        : m_stmt{stmt}, m_attributes{attributes}
    {}

    /**
     * @brief Get the id of the geometry (primary key)
     * 
     * @return Geometry id
     */
    [[nodiscard]] int GetId() const
    {
        return m_stmt.getColumn("id");
    }

    /**
     * @brief Add the geometry and it's attributes to the given feature
     */
    void AddTo(IFeature& feature)
    {
        AddAttributesTo(feature);

        if constexpr (GeomType == GeometryType::Point)
        {
            AddPointTo(feature);
        }
        else if constexpr (GeomType == GeometryType::MultiPoint)
        {
            AddMultiPointTo(feature);
        }
        else if constexpr (GeomType == GeometryType::Line || GeomType == GeometryType::Polygon)
        {
            AddLineTo(feature);
        }
        else if constexpr (GeomType == GeometryType::MultiLine || GeomType == GeometryType::MultiPolygon)
        {
            AddMultiLineTo(feature);
        }
        else
        {
            throw std::logic_error{"Unknown geometry type"};
        }
    }

private:
    void AddAttributesTo(IFeature& feature)
    {
        for (const auto& [name, type] : m_attributes)
        {
            const auto value = m_stmt.getColumn(name.c_str());
            switch (type)
            {
            case ColumnType::Int64:
                feature.AddAttribute(name, value.getInt64());
                break;
            case ColumnType::Double:
                feature.AddAttribute(name, value.getDouble());
                break;
            case ColumnType::String:
                feature.AddAttribute(name, value.getString());
                break;
            }
        }
    }

    void AddPointTo(IFeature& feature)
    {
        auto geometry = feature.AddGeometry(GeomType, 1);
        geometry->AddPoint(m_stmt.getColumns<mapget::Point, static_cast<int>(Dim)>());
        m_stmt.executeStep();
    }

    void AddMultiPointTo(IFeature& feature)
    {
        const int count = m_stmt.getColumn("pointsCount");
        for (int i = count; i > 0; --i)
        {
            AddPointTo(feature);
        }
    }

    void AddLineTo(IFeature& feature)
    {
        const int count = m_stmt.getColumn("pointsCount");
        auto geometry = feature.AddGeometry(GeomType, count);
        for (int i = count; i > 0; --i)
        {
            geometry->AddPoint(m_stmt.getColumns<mapget::Point, static_cast<int>(Dim)>());
            m_stmt.executeStep();
        }
    }

    void AddMultiLineTo(IFeature& feature)
    {
        const int count = m_stmt.getColumn("linesCount");
        for (int i = count; i > 0; --i)
        {
            AddLineTo(feature);
        }
    }

private:
    SQLite::Statement& m_stmt;
    const AttributesInfo& m_attributes;
};

template <GeometryType GeomType, Dimension Dim>
class GeometryIterator
{
public:
    GeometryIterator() noexcept = default;

    explicit GeometryIterator(SQLite::Statement& stmt, const AttributesInfo& attributes) noexcept
        : m_stmt{&stmt}, m_attributes{&attributes}
    {}

    GeometryIterator& operator++() noexcept
    {
        if (m_stmt->isDone())
            m_stmt = nullptr;
        return *this;
    }
    [[nodiscard]] Geometry<GeomType, Dim> operator*() const noexcept
    {
        return Geometry<GeomType, Dim>{*m_stmt, *m_attributes};

    }
    [[nodiscard]] bool operator==(const GeometryIterator& other) const noexcept
    {
        return m_stmt == other.m_stmt;
    }

private:
    SQLite::Statement* m_stmt = nullptr;
    const AttributesInfo* const m_attributes = nullptr;
};

} // namespace Detail

/**
 * @brief View that allows iterating over geometries from a spatialite table
 * 
 * @tparam GeomType Type of the geometries
 * @tparam Dim Dimension of the geometries (2D/3D)
 */
template <GeometryType GeomType, Dimension Dim>
class GeometriesView
{
public:
    using Iterator = Detail::GeometryIterator<GeomType, Dim>;

    explicit GeometriesView(SQLite::Statement&& stmt, const AttributesInfo& attributes) noexcept 
        : m_stmt{std::move(stmt)}, m_attributes{attributes}
    {}

    Iterator begin()
    {
        if (m_stmt.executeStep())
            return Iterator{m_stmt, m_attributes};
        else
            return {};
    }
    Iterator end() const noexcept
    {
        return {};
    }

private:
    SQLite::Statement m_stmt;
    const AttributesInfo& m_attributes;
};

} // namespace SpatialiteDatasource
