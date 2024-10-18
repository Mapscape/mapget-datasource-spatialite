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

namespace SpatialiteDatasource {

static UniqueGaiaGeomCollPtr GetGaiaPtr(const SQLite::Statement& stmt)
{
    const auto geomColumn = stmt.getColumn("geometry");
    return UniqueGaiaGeomCollPtr{gaiaFromSpatiaLiteBlobWkb(static_cast<const uint8_t*>(geomColumn.getBlob()), geomColumn.size())};
}

Geometry::Geometry(
    GeometryType geometryType, 
    Dimension dimension,
    SQLite::Statement& stmt,
    const AttributesInfo& attributes
) noexcept 
    : m_geometryType{geometryType}
    , m_dimension{dimension}
    , m_stmt{stmt}
    , m_attributes{attributes}
{}

[[nodiscard]] int Geometry::GetId() const
{
    return m_stmt.getColumn("id");
}

void Geometry::AddTo(IFeature& feature)
{
    AddAttributesTo(feature);

    const auto geomPtr = GetGaiaPtr(m_stmt);
    switch (m_geometryType)
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
    m_stmt.executeStep();
}

void Geometry::AddAttributesTo(IFeature& feature)
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

void Geometry::AddPointTo(gaiaPointPtr point, IFeature& feature)
{
    auto geometry = feature.AddGeometry(m_geometryType, 1);
    switch (m_dimension)
    {
    case Dimension::XY:
    case Dimension::XYM:
        geometry->AddPoint({point->X, point->Y}); 
        break;
    case Dimension::XYZ:
    case Dimension::XYZM:
        geometry->AddPoint({point->X, point->Y, point->Z});
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

GeometryIterator::GeometryIterator() noexcept
    : m_geometryType{GeometryType::Point}
    , m_dimension{Dimension::XY}
    , m_stmt{nullptr}
    , m_attributes{nullptr}
{}

GeometryIterator::GeometryIterator(
    GeometryType geometryType, 
    Dimension dimension, 
    SQLite::Statement& stmt, 
    const AttributesInfo& attributes
) noexcept
    : m_geometryType{geometryType}
    , m_dimension{dimension}
    , m_stmt{&stmt}
    , m_attributes{&attributes}
{}

GeometryIterator& GeometryIterator::operator++() noexcept
{
    if (m_stmt->isDone())
        m_stmt = nullptr;
    return *this;
}
[[nodiscard]] Geometry GeometryIterator::operator*() const noexcept
{
    return Geometry{m_geometryType, m_dimension, *m_stmt, *m_attributes};

}
[[nodiscard]] bool GeometryIterator::operator==(const GeometryIterator& other) const noexcept
{
    return m_stmt == other.m_stmt;
}

GeometriesView::GeometriesView(
    GeometryType geometryType, 
    Dimension dimension, 
    SQLite::Statement&& stmt, 
    const AttributesInfo& attributes
) noexcept 
    : m_geometryType{geometryType}
    , m_dimension{dimension}
    , m_stmt{std::move(stmt)}
    , m_attributes{attributes}
{}

GeometryIterator GeometriesView::begin()
{
    if (m_stmt.executeStep())
        return GeometryIterator{m_geometryType, m_dimension, m_stmt, m_attributes};
    else
        return {};
}

GeometryIterator GeometriesView::end() const noexcept
{
    return {};
}

} // namespace SpatialiteDatasource
