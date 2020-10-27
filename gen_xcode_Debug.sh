#!/bin/sh
gn gen --ide=xcode --ninja-extra-args="-j8" --filters=//my_example/* --no-deps --args="v8_use_external_startup_data=false treat_warnings_as_errors=false" out/Debug
