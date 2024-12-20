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

#include "TableInfo.h"

#include <SQLiteCpp/Column.h>
#include <fmt/format.h>
#include <boost/algorithm/string/case_conv.hpp>
#include <stdexcept>

namespace SpatialiteDatasource {

ColumnType ColumnTypeFromSqlType(int sqlType)
{
    if (sqlType == SQLite::INTEGER)
        return ColumnType::Int64;
    if (sqlType == SQLite::FLOAT)
        return ColumnType::Double;
    if (sqlType == SQLite::TEXT)
        return ColumnType::Text;
    if (sqlType == SQLite::BLOB || sqlType == SQLite::Null)
        return ColumnType::Blob;
    throw std::runtime_error{fmt::format("Unknown SQL column type: {}", sqlType)};
}

ColumnType ParseColumnType(const std::string& type)
{
    auto typeLower = boost::algorithm::to_lower_copy(type);
    if (typeLower == "integer")
        return ColumnType::Int64;
    if (typeLower == "float")
        return ColumnType::Double;
    if (typeLower == "text")
        return ColumnType::Text;
    if (typeLower == "blob")
        return ColumnType::Blob;
    throw std::runtime_error{fmt::format("Can't parse attributes json: invalid attribute type '{}'", type)};
}

std::string_view ColumnTypeToString(ColumnType columnType)
{
    using namespace std::literals;
    switch (columnType)
    {
    case ColumnType::Int64:
        return "Int64"sv;
    case ColumnType::Double:
        return "Double"sv;
    case ColumnType::Text:
        return "Text"sv;
    case ColumnType::Blob:
        return "Blob"sv;
    default:
        throw std::runtime_error{fmt::format("Unknown column type: {}", static_cast<int>(columnType))};
    }
}

} // namespace SpatialiteDatasource