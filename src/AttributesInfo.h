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
    std::vector<std::string> columns;
    std::string delimiter;
    std::string matchCondition;
};

struct AttributeInfo
{
    ColumnType type;
    std::optional<Relation> relation;
};

// attribute_name -> attribute_info
using AttributesInfo = std::unordered_map<std::string, AttributeInfo>;

// table_name -> attributes_info
using TablesAttributesInfo = std::unordered_map<std::string, AttributesInfo>;

} // namespace SpatialiteDatasource
