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

#include "GeometriesView.h"

#include "UniqueGaiaGeomCollPtr.h"

#include <boost/algorithm/hex.hpp>
#include <cstdint>
#include <iterator>

namespace SpatialiteDatasource {

static UniqueGaiaGeomCollPtr GetGaiaPtr(const SQLite::Statement& stmt)
{
    const auto geomColumn = stmt.getColumn("__geometry");
    return UniqueGaiaGeomCollPtr{gaiaFromSpatiaLiteBlobWkb(static_cast<const uint8_t*>(geomColumn.getBlob()), geomColumn.size())};
}

static std::string BlobToHex(const void* ptr, int size)
{
    std::string hex;
    hex.reserve(size * 2);
    const auto* blob = static_cast<const uint8_t*>(ptr);
    boost::algorithm::hex(blob, blob + size, std::back_inserter(hex));
    return hex;
}

Geometry::Geometry(const SQLite::Statement& stmt, const TableInfo& tableInfo) noexcept 
    : m_stmt{stmt}
    , m_tableInfo{tableInfo}
{}

[[nodiscard]] int Geometry::GetId() const
{
    return m_stmt.getColumn("__id");
}

void Geometry::AddTo(IFeature& feature)
{
    AddAttributesTo(feature);

    const auto geomPtr = GetGaiaPtr(m_stmt);
    switch (m_tableInfo.geometryType)
    {
    case GeometryType::Point:
        AddPointTo(geomPtr->FirstPoint, feature);
        break;
    case GeometryType::Line:
        AddLineOrPolygonTo(geomPtr->FirstLinestring, feature);
        break;
    case GeometryType::Polygon:
        AddLineOrPolygonTo(geomPtr->FirstPolygon->Exterior, feature);
        break;
    case GeometryType::MultiPoint:
        AddMultiPointTo(geomPtr->FirstPoint, feature);
        break;
    case GeometryType::MultiLine:
        AddMultiLineTo(geomPtr->FirstLinestring, feature);
        break;
    case GeometryType::MultiPolygon:
        AddMultiPolygonTo(geomPtr->FirstPolygon, feature);
        break;
    }
}

void Geometry::AddAttributesTo(IFeature& feature)
{
    for (const auto& [name, info] : m_tableInfo.attributes)
    {
        const auto value = m_stmt.getColumn(name.c_str());
        switch (info.type)
        {
        case ColumnType::Int64:
            feature.AddAttribute(name, value.getInt64());
            break;
        case ColumnType::Double:
            feature.AddAttribute(name, value.getDouble());
            break;
        case ColumnType::Text:
            feature.AddAttribute(name, value.getString());
            break;
        case ColumnType::Blob:
            feature.AddAttribute(name, BlobToHex(value.getBlob(), value.size()));
            break;
        }
    }
}

void Geometry::AddPointTo(gaiaPointPtr point, IFeature& feature)
{
    auto geometry = feature.AddGeometry(m_tableInfo.geometryType, 1);
    const auto& scaling = m_tableInfo.scaling;
    switch (m_tableInfo.dimension)
    {
    case Dimension::XY:
    case Dimension::XYM:
        geometry->AddPoint({point->X * scaling.x, point->Y * scaling.y}); 
        break;
    case Dimension::XYZ:
    case Dimension::XYZM:
        geometry->AddPoint({point->X * scaling.x, point->Y * scaling.y, point->Z * scaling.z});
        break;
    }
}

void Geometry::AddMultiPointTo(gaiaPointPtr firstPoint, IFeature& feature)
{
    for (auto* pointPtr = firstPoint; pointPtr != nullptr; pointPtr = pointPtr->Next)
    {
        AddPointTo(pointPtr, feature);
    }
}

void Geometry::AddMultiLineTo(gaiaLinestringPtr firstLine, IFeature& feature)
{
    for (auto* linePtr = firstLine; linePtr != nullptr; linePtr = linePtr->Next)
    {
        AddLineOrPolygonTo(linePtr, feature);
    }
}

void Geometry::AddMultiPolygonTo(gaiaPolygonPtr firstPolygon, IFeature& feature)
{
    for (auto* polygonPtr = firstPolygon; polygonPtr != nullptr; polygonPtr = polygonPtr->Next)
    {
        AddLineOrPolygonTo(polygonPtr->Exterior, feature);
    }
}

GeometryIterator::GeometryIterator(SQLite::Statement& stmt, const TableInfo& tableInfo) noexcept
    : m_stmt{&stmt}
    , m_tableInfo{&tableInfo}
{}

GeometryIterator& GeometryIterator::operator++() noexcept
{
    if (!m_stmt->executeStep())
        m_stmt = nullptr;
    return *this;
}
[[nodiscard]] Geometry GeometryIterator::operator*() const noexcept
{
    return Geometry{*m_stmt, *m_tableInfo};

}
[[nodiscard]] bool GeometryIterator::operator==(const GeometryIterator& other) const noexcept
{
    return m_stmt == other.m_stmt;
}

GeometriesView::GeometriesView(SQLite::Statement&& stmt, const TableInfo& tableInfo) noexcept 
    : m_stmt{std::move(stmt)}
    , m_tableInfo{tableInfo}
{}

GeometryIterator GeometriesView::begin()
{
    if (m_stmt.executeStep())
        return GeometryIterator{m_stmt, m_tableInfo};
    else
        return {};
}

GeometryIterator GeometriesView::end() const noexcept
{
    return {};
}

} // namespace SpatialiteDatasource
