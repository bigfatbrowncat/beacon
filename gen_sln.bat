@echo off
pushd ..
gn gen --ide=vs2019 --ninja-extra-args="-j8" --filters=//:HelloWorld;//skia/*;//third_party/blink/* --no-deps --args="v8_use_external_startup_data=false" out\HW
popd