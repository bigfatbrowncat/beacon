@echo off
pushd ..
rem gn gen --ide=vs2019 --ninja-extra-args="-j8" --filters=//my_example/*;//skia/*;//third_party/blink/* --no-deps --args="v8_use_external_startup_data=false is_component_build=false is_debug=false symbol_level=1" out\Release
gn gen --ide=vs2019 --ninja-extra-args="-j8" --filters=//beacon/* --no-deps --args="v8_use_external_startup_data=false is_component_build=false is_debug=false symbol_level=1" out\Release
popd
