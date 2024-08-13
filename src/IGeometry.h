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

// glm problems
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvolatile"
#   include <mapget/model/point.h>
#pragma GCC diagnostic pop

namespace SpatialiteDatasource {

struct IGeometry
{
    /**
     * @brief Add point to the geometry
     * 
     * @param point Point to add
     */
    virtual void AddPoint(const mapget::Point& point) = 0;
};

/**
 * @brief Interface for the geometry storage
 *  This interface was created in order to separate Mapget api from the spatialite code,
 *  and make unit testing much easier
 */
struct IGeometryStorage
{
    /**
     * @brief Add geometry to the storage
     * 
     * @param type Type of the geometry to add
     * @param initialCapacity Initial capacity (points) of the geometry
     * @return Created geometry
     */
    virtual std::unique_ptr<IGeometry> AddGeometry(GeometryType type, size_t initialCapacity) = 0;
};

} // namespace SpatialiteDatasource