# Copyright (c) 2025 NavInfo Europe B.V.

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import sys
import os

if len(sys.argv) < 3:
    print("Error: less arguments than expected")
    print("Usage: generate_config_schema_header.py schema.yaml header.h")
    exit(-1)

schema_path = sys.argv[1]
header_path = sys.argv[2]

schema = ''
with open(schema_path, 'r') as schema_file:
    schema = schema_file.read()

os.makedirs(os.path.dirname(header_path), exist_ok=True)
with open(header_path, 'w') as header_file:
    header_file.write(f'''#pragma once

namespace SpatialiteDatasource {{

inline constexpr const char* ConfigSchema = R"YAML({schema})YAML";

}} // namespace SpatialiteDatasource
''')

