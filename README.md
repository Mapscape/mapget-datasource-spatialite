# mapget-datasource-spatialite

![build status](https://github.com/Mapscape/mapget-datasource-spatialite/actions/workflows/ci.yaml/badge.svg)

This project represents a datasource for [Mapget](https://github.com/ndsev/mapget) that allows viewing [Spatialite](https://www.gaia-gis.it/fossil/libspatialite/index) databases in web browser with [Erdblick](https://github.com/ndsev/erdblick)

Usage:
```
  -h [ --help ]          produce help message
  -m [ --map ] arg       path to a spatialite database to use
  -p [ --port ] arg (=0) http server port
  -c [ --config ] arg    path to a datasource config in json format (will retrieve the info from the db if not provided)
  --no-attributes        do not add any attributes to features
  -v [ --verbose ]       enable debug logs
```

Config file is optional, the format is described in [config_description.jsonc](config_description.jsonc) file

With Mapget and Erdblick it can be used like this:
```
mapget-datasource-spatialite -m /path/to/spatialite-db -c /path/to/config/json -p <datasource_port>
mapget serve -w /path/to/built/erdblick -d 127.0.0.1:<datasource_port> -p <erdblick_port>
```
Erdblick will be available at 127.0.0.1:<erdblick_port>

## Build

Prerequisites:
- C++20 compatible compiler
- conan >= 2.0
- cmake >= 3.25

How to build:
```
conan install . -s build_type=Release --build=missing
cmake --preset conan-release
cmake --build --preset conan-release
```
