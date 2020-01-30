@echo off
pushd ..
gn gen --ide=vs2019 --ninja-extra-args="-j8" --filters=//:HelloWorld;//skia/*;//third_party/blink/* --no-deps out\HW
popd