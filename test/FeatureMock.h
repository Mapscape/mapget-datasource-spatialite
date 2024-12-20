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

#include "GeometriesView.h"
#include <IFeature.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using MapgetGeometry = std::vector<mapget::Point>;
using MapgetGeometries = std::vector<MapgetGeometry>;

struct GeometryMock : SpatialiteDatasource::IGeometry
{
    explicit GeometryMock(MapgetGeometry& geometry) : geometry{geometry} {}

    void AddPoint(const mapget::Point& point) override
    {
        geometry.push_back(point);
    }

    MapgetGeometry& geometry;
};

template <typename T>
struct Attribute
{
    std::string name;
    T value;
};

struct FeatureMock : SpatialiteDatasource::IFeature
{
    std::unique_ptr<SpatialiteDatasource::IGeometry> AddGeometry(
        SpatialiteDatasource::GeometryType type, size_t initialCapacity) override
    {
        types.push_back(type);
        initialCapacities.emplace_back(initialCapacity);
        return std::make_unique<GeometryMock>(geometries.emplace_back());
    }

    void AddGeometries(SpatialiteDatasource::GeometriesView& geometries)
    {
        for (auto g : geometries)
        {
            g.AddTo(*this);
        }
    }

    MOCK_METHOD(void, AddAttribute, (std::string_view name, int64_t value), (override));
    MOCK_METHOD(void, AddAttribute, (std::string_view name, double value), (override));
    MOCK_METHOD(void, AddAttribute, (std::string_view name, std::string_view value), (override));

    MapgetGeometries geometries;
    std::vector<SpatialiteDatasource::GeometryType> types;
    std::vector<size_t> initialCapacities;
};
