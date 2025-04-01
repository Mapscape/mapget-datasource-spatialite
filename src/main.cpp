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

#include "Datasource.h"
#include "ConfigLoader.h"

#include <mapget/log.h>

#include <sqlite3.h>
#include <spatialite.h>

#include <boost/scope_exit.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <fstream>

namespace po = boost::program_options;

int main(int argc, char** argv)
{
    auto newLogger = mapget::log().clone("msds");
    mapget::log().swap(*newLogger);

    std::filesystem::path mapPath{}, configPath{};
    uint16_t port{0};
    bool isVerbose{false}, isNoAttributes{false}, isAttributes{false};

    po::options_description description{"Allowed options"};
    description.add_options()
        ("help,h", "produce help message")
        ("map,m", po::value(&mapPath), "path to a spatialite database to use")
        ("port,p", po::value(&port), "http server port")
        ("config,c", po::value(&configPath), "path to a datasource config in json format (will retrieve the info from the db if not provided)")
        ("attributes", po::bool_switch(&isAttributes), "enable features attributes (enabled by default)")
        ("no-attributes", po::bool_switch(&isNoAttributes), "disable features attributes ")
        ("verbose,v", po::bool_switch(&isVerbose), "enable debug logs");

    po::variables_map vm;
    try 
    {
        po::store(po::parse_command_line(argc, argv, description), vm);

        if (vm.contains("help"))
        {
            std::cout << description << std::endl;
            return 1;
        }

        if (isAttributes && isNoAttributes)
        {
            throw std::runtime_error("Conflicting options were provided");
        }

        po::notify(vm);
    }
    catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << description << std::endl;
        return -1;
    }

    if (isVerbose)
        mapget::log().set_level(spdlog::level::debug);
    
    spatialite_initialize();
    BOOST_SCOPE_EXIT(void) {
        spatialite_shutdown();
    } BOOST_SCOPE_EXIT_END

    SpatialiteDatasource::OverrideOptions options;
    if (vm.contains("port"))
    {
        options.port = port;
    }
    if (vm.contains("map"))
    {
        options.mapPath = mapPath;
    }
    if (isAttributes)
    {
        options.disableAttributes = false;
    }
    else if (isNoAttributes)
    {
        options.disableAttributes = true;
    }

    SpatialiteDatasource::Datasource datasource = configPath.empty() ? 
        SpatialiteDatasource::CreateDatasourceDefaultConfig(options) : SpatialiteDatasource::CreateDatasource(configPath, options);
    datasource.Run();

    return 0;
}
