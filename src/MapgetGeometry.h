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

#include "IGeometry.h"

#include <mapget/model/feature.h>

#include <memory>

namespace SpatialiteDatasource {

class MapgetGeometry : public IGeometry
{
public:
    explicit MapgetGeometry(mapget::model_ptr<mapget::Geometry>&& geometry) : m_geometry{std::move(geometry)} {}
    
    void AddPoint(const mapget::Point& point) final
    {
        m_geometry->append(point);
    }
private:
    mapget::model_ptr<mapget::Geometry> m_geometry;
};

class MapgetFeatureGeometryStorage : public IGeometryStorage
{
public:
    explicit MapgetFeatureGeometryStorage(mapget::Feature& feature) : m_feature{feature} {}

    std::unique_ptr<IGeometry> AddGeometry(GeometryType type, size_t initialCapacity) final
    {
        return std::make_unique<MapgetGeometry>(
            m_feature.geom()->newGeometry(GeometryToMapgetGeometry(type), initialCapacity));
    }

private:
    static mapget::GeomType GeometryToMapgetGeometry(GeometryType geometry)
    {
        switch (geometry) 
        {
        case GeometryType::Point:
        case GeometryType::MultiPoint:
            return mapget::GeomType::Points;
        case GeometryType::Line:
        case GeometryType::MultiLine:
            return mapget::GeomType::Line;
        case GeometryType::Polygon:
        case GeometryType::MultiPolygon:
            return mapget::GeomType::Polygon;
        }
        throw std::logic_error{"Can't convert to mapget geometry type: unknown type"};
    }

private:
    mapget::Feature& m_feature;
};

} // namespace SpatialiteDatasource