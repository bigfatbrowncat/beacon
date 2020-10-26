#!/bin/sh
gn gen --ninja-extra-args="-j8" --filters=//my_example/* --no-deps --args="v8_use_external_startup_data=false is_component_build=false is_debug=false symbol_level=1 treat_warnings_as_errors=false" out/Release
