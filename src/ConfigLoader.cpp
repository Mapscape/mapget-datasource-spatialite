// Copyright (c) 2025 NavInfo Europe B.V.

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

#include "ConfigLoader.h"

#include "ConfigSchema.h"

#include <cerberus-cpp/validator.hh>
#include <boost/algorithm/string/case_conv.hpp>
#include <fmt/ranges.h>

#include <sstream>

namespace SpatialiteDatasource {
namespace {

template <class Head, class... Tail>
[[nodiscard]] YAML::Node GetNode(const YAML::Node& node, const Head& headPath, const Tail&... tailPath)
{
    const auto child = node[headPath];
    if constexpr (sizeof...(tailPath) == 0)
    {
        return child;
    }
    else
    {
        if (child)
        {
            return GetNode(child, tailPath...);
        }
        return child;
    }
}

template <class T, class String>
[[nodiscard]] std::decay_t<T> GetValueOrDefault(const YAML::Node& node, const String& key, const T& defaultValue)
{
    if (const auto child = node[key]; child)
    {
        return child.template as<std::decay_t<T>>();
    }
    return defaultValue;
}

TableInfo& EmplaceTableInfo(TablesInfo& tablesInfo, const std::string& tableName, const Database& database)
{
    const auto emplaceResult = tablesInfo.emplace(
            std::piecewise_construct, std::forward_as_tuple(tableName), std::forward_as_tuple(tableName, database));
    return emplaceResult.first->second;
}

[[nodiscard]] ScalingInfo ParseScalingConfig(const YAML::Node& config, ScalingInfo defaultScaling = {})
{
    ScalingInfo result = defaultScaling;
    for (auto it = config.begin(); it != config.end(); ++it)
    {
        const auto projections = it->first.as<std::string_view>();
        const auto value = it->second.as<double>();
        for (auto projection : projections)
        {
            switch (projection)
            {
            case 'x':
                result.x = value;
                break;
            case 'y':
                result.y = value;
                break;
            case 'z':
                result.z = value;
                break;
            [[unlikely]] default:
                throw std::runtime_error{fmt::format("Unknown projection '{}' in '{}'", projection, projections)};
            }
        }
    }
    return result;
}

[[nodiscard]] AttributeInfo ParseAttributeInfo(const YAML::Node& attributeDescription, const Database& database)
{
    AttributeInfo attribute;
    std::optional<ColumnType> type;
    if (const auto typeNode = attributeDescription["type"]; typeNode)
    {
        type = ParseColumnType(typeNode.as<std::string>());
    }

    if (const auto relationNode = attributeDescription["relation"]; relationNode)
    {
        auto& relation = attribute.relation.emplace();
        relation.delimiter = GetValueOrDefault<std::string>(relationNode, "delimiter", "|");
        relation.matchCondition = relationNode["matchCondition"].as<std::string>();
        const auto relatedColumns = relationNode["relatedColumns"];
        if (relatedColumns.size() == 1)
        {
            auto columnName = relatedColumns[0].as<std::string>();
            auto tableName = columnName.substr(0, columnName.find('.'));
            if (!type.has_value())
            {
                type = database.GetColumnType(tableName, columnName);
            }
            relation.columns.emplace_back(std::move(columnName));
        }
        else 
        {
            if (!type.has_value())
            {
                type = ColumnType::Text;
            }
            for (const auto& column : relatedColumns)
            {
                relation.columns.emplace_back(column.as<std::string>());
            }
        }
    }
    attribute.type = type.value();
    return attribute;
}

void LogTablesInfo(const TablesInfo& info)
{
    std::string log = "Loaded attributes config:";
    constexpr auto Indent = 2;
    for (const auto& [table, tableInfo] : info)
    {
        log += fmt::format("\n{0:{1}}{2}:", "", Indent * 1, table);
        
        log += fmt::format("\n{0:{1}}{2}:", "", Indent * 2, "coordinatesScaling");
        const auto& scaling = tableInfo.scaling;
        log += fmt::format("\n{0:{1}}x: {2}", "", Indent * 3, scaling.x);
        log += fmt::format("\n{0:{1}}y: {2}", "", Indent * 3, scaling.y);
        log += fmt::format("\n{0:{1}}z: {2}", "", Indent * 3, scaling.z);

        log += fmt::format("\n{0:{1}}{2}:", "", Indent * 2, "attributes");
        for (const auto& [attribute, attributeInfo] : tableInfo.attributes)
        {
            log += fmt::format("\n{0:{1}}{2}({3})", "", Indent * 3, attribute, ColumnTypeToString(attributeInfo.type));
            if (attributeInfo.relation.has_value())
            {
                const auto& relation = attributeInfo.relation.value();
                log += fmt::format(":\n{0:{1}}columns: {2}\n{0:{1}}matchCondition: {3}\n{0:{1}}delimiter: '{4}'", 
                    "", Indent * 4, 
                    fmt::join(relation.columns, ", "),
                    relation.matchCondition,
                    relation.delimiter);
            }
        }
    }
    mapget::log().debug(log);
}

} // anonymous namespace


ConfigLoader::ConfigLoader(const YAML::Node& config, const OverrideOptions& options)
    : m_config{config}
    , m_loadRemainingLayersFromDb{GetValueOrDefault(m_config, "loadRemainingLayersFromDb", true)}
{
    cerberus::Validator validator{YAML::Load(ConfigSchema)};
    if (!validator.validate(m_config))
    {
        std::stringstream sstream;
        sstream << validator;
        throw std::runtime_error{sstream.str()};
    }

    if (options.mapPath.has_value())
    {
        m_datasourceOptions.mapPath = *options.mapPath;
    }
    else
    {
        const auto pathNode = GetNode(m_config, "map", "path");
        if (!pathNode)
        {
            throw std::runtime_error{"The mandatory option '--map' was neither provided nor specified in the config"};
        }
        m_datasourceOptions.mapPath = pathNode.as<std::string>();
    }

    const auto loadOverrideOption = [this]<class T, class Str>(
        const Str& yamlKey, const std::optional<T>& option, T defaultValue)
    {
        if (option.has_value())
        {
            return *option;
        }
        
        if (const auto portNode = m_config[yamlKey]; portNode)
        {
            return portNode.template as<T>();
        }
            
        return defaultValue;
    };

    m_datasourceOptions.port = loadOverrideOption("datasourcePort", options.port, static_cast<uint16_t>(0));
    m_disableAttributes = loadOverrideOption("disableAttributes", options.disableAttributes, false);

    if (const auto layers = m_config["layers"]; layers)
    {
        for (const auto& layer : layers)
        {
            const auto tableName = boost::to_lower_copy(layer["table"].as<std::string>());
            m_layerConfigByTable.emplace(tableName, layer);
        }
    }
}

[[nodiscard]] const DatasourceOptions& ConfigLoader::GetDatasourceOptions() const
{
    return m_datasourceOptions;
}

[[nodiscard]] nlohmann::json ConfigLoader::GenerateDatasourceConfig(const Database& database) const
{
    nlohmann::json infoJson;
    const auto mapIdNode = GetNode(m_config, "map", "name");
    infoJson["mapId"] = mapIdNode ? mapIdNode.as<std::string>() : m_datasourceOptions.mapPath.filename().string();
    infoJson["layers"] = nlohmann::json::object();
    auto& layers = infoJson["layers"];
    const auto addLayer = [&layers] (const std::string& layerName, const std::string& tableName)
    {
        layers[layerName] = {{"featureTypes", nlohmann::json::array({nlohmann::json::object({
            {"name", tableName},
            {"uniqueIdCompositions", nlohmann::json::array({nlohmann::json::array({nlohmann::json::object({
                {"partId", "id"},
                {"datatype", "I32"}
        })})})}})})}};
    };

    for (const auto& [tableName, layer] : m_layerConfigByTable)
    {
        addLayer(GetValueOrDefault(layer, "name", tableName), tableName);
    }

    if (m_loadRemainingLayersFromDb)
    {
        const auto tables = database.GetTablesNames();
        for (const auto& table : tables)
        {
            if (!m_layerConfigByTable.contains(boost::to_lower_copy(table)))
                addLayer(table, table);
        }
    }
    
    mapget::log().debug("Datasource info:\n{}", infoJson.dump(2));
    return infoJson;
}

[[nodiscard]] TablesInfo ConfigLoader::LoadTablesInfo(const Database& database) const
{
    TablesInfo tablesInfo;
    ScalingInfo defaultScaling{};
    const auto globalScaling = GetNode(m_config, "global", "coordinatesScaling");
    if (globalScaling)
    {
        defaultScaling = ParseScalingConfig(globalScaling);
    }

    for (const auto& [tableName, layer] : m_layerConfigByTable)
    {
        auto& tableInfo = EmplaceTableInfo(tablesInfo, tableName, database);

        if (const auto scalingConfig = layer["coordinatesScaling"]; scalingConfig)
        {
            tableInfo.scaling = ParseScalingConfig(scalingConfig, defaultScaling);            
        }
        else
        {
            tableInfo.scaling = defaultScaling;
        }
        
        if (!m_disableAttributes)
        {
            if (GetValueOrDefault(layer, "loadRemainingAttributesFromDb", true))
            {
                database.FillTableAttributes(tableInfo);
            }

            if (const auto& attributes = layer["attributes"]; attributes)
            {
                for (const auto attribute : attributes)
                {
                    tableInfo.attributes[attribute["name"].as<std::string>()] = ParseAttributeInfo(attribute, database);
                }
            }
        }
    }

    if (m_loadRemainingLayersFromDb)
    {
        auto tableNames = database.GetTablesNames();
        for (auto& tableName : tableNames)
        {
            boost::to_lower(tableName);
            if (tablesInfo.contains(tableName))
                continue;

            auto& tableInfo = EmplaceTableInfo(tablesInfo, tableName, database);
            tableInfo.scaling = defaultScaling;
            database.FillTableAttributes(tableInfo);
        }
    }

    if (mapget::log().level() == spdlog::level::debug)
    {
        LogTablesInfo(tablesInfo);
    }

    return tablesInfo;
}

} // namespace SpatialiteDatasource