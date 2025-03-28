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

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace SpatialiteDatasource {

enum class ColumnType
{
    Int64,
    Double,
    Text,
    Blob
};

ColumnType ColumnTypeFromSqlType(int sqlType);
ColumnType ParseColumnType(const std::string& type);
std::string_view ColumnTypeToString(ColumnType columnType);

struct Relation
{
    [[nodiscard]] bool operator==(const Relation&) const = default;

    std::vector<std::string> columns;
    std::string delimiter;
    std::string matchCondition;
};

struct AttributeInfo
{
    [[nodiscard]] bool operator==(const AttributeInfo&) const = default;

    ColumnType type = ColumnType::Blob;
    std::optional<Relation> relation{std::nullopt};
};

// attribute_name -> attribute_info
using AttributesInfo = std::unordered_map<std::string, AttributeInfo>;

struct ScalingInfo
{
    [[nodiscard]] bool operator==(const ScalingInfo&) const = default;

    double x = 1;
    double y = 1;
    double z = 1;
};

struct Database;

struct TableInfo
{
    TableInfo() = default;
    TableInfo(const std::string& name, const Database& db);

    const std::string& GetSqlQuery() const;
    [[nodiscard]] bool operator==(const TableInfo&) const = default;

    std::string name;
    std::string primaryKey;
    std::string geometryColumn;
    GeometryType geometryType = GeometryType::Point;
    Dimension dimension = Dimension::XY;
    SpatialIndex spatialIndex = SpatialIndex::None;

    AttributesInfo attributes;
    ScalingInfo scaling;

private:
    mutable std::string m_sqlQuery;
};

// table_name -> table_info
using TablesInfo = std::unordered_map<std::string, TableInfo>;

} // namespace SpatialiteDatasource
