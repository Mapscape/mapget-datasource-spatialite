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

#pragma once

#include "Database.h"

#include "TableInfo.h"

#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <optional>
#include <unordered_map>

namespace SpatialiteDatasource {

/**
 * @brief Used for options that can be overriden by command line options
 */
struct OverrideOptions
{
    std::optional<std::filesystem::path> mapPath;
    std::optional<uint16_t> port;
    std::optional<bool> disableAttributes;
};

/**
 * @brief Mandatory options for the datasource
 */
struct DatasourceOptions
{
    std::filesystem::path mapPath;
    uint16_t port;
};

/**
 * @brief Represents datasource config
 */
class ConfigLoader
{
public:
    /**
    * @brief Construct a new Config Loader object
    * 
    * @param config YAML config
    * @param options Options that will override the options from the config
    */
    ConfigLoader(const YAML::Node& config, const OverrideOptions& options);

    /**
     * @brief Get the Datasource options
     */
    [[nodiscard]] const DatasourceOptions& GetDatasourceOptions() const;

    /**
     * @brief Generate mapget datasource config
     * 
     * @param database Spatialite database
     * @return nlohmann::json Mapget datasource config in JSON format
     */
    [[nodiscard]] nlohmann::json GenerateDatasourceConfig(const Database& database) const;

    /**
     * @brief Load tables info from the config and the database
     * 
     * @param database Spatialite database
     */
    [[nodiscard]] TablesInfo LoadTablesInfo(const Database& database) const;
private:
    const YAML::Node m_config;
    const bool m_loadRemainingLayersFromDb;
    bool m_disableAttributes;
    DatasourceOptions m_datasourceOptions;
    std::unordered_map<std::string, YAML::Node> m_layerConfigByTable;
};

} // namespace SpatialiteDatasource