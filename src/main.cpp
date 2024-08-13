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

#include "Datasource.h"
#include "Database.h"

#include <mapget/log.h>

#include <sqlite3.h>
#include <spatialite.h>

#include <boost/scope/defer.hpp>
#include <boost/program_options.hpp>

#include <iostream>

namespace po = boost::program_options;

int main(int argc, char** argv)
{
    auto newLogger = mapget::log().clone("msds");
    mapget::log().swap(*newLogger);

    std::filesystem::path mapPath, infoPath;
    uint16_t port;
    bool isVerbose;
    po::options_description description{"Allowed options"};
    description.add_options()
        ("help", "produce help message")
        ("map,m", po::value(&mapPath), "path to a spatialite database to use")
        ("port,p", po::value(&port)->default_value(0), "http server port")
        ("info,i", po::value(&infoPath), "path to a datasource info in json format")
        ("verbose,v", po::bool_switch(&isVerbose), "enable debug logs");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, description), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << description << std::endl;
        return 1;
    }

    if (isVerbose)
        mapget::log().set_level(spdlog::level::debug);
    
    spatialite_initialize();
    BOOST_SCOPE_DEFER []{
        spatialite_shutdown();
    };
    
    SpatialiteDatasource::Datasource ds{mapPath, infoPath, port};
    ds.Run();

    return 0;
}
