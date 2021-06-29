@echo off
rem Run this from src folder (above beacon)
gn gen --ide=vs2019 --ninja-extra-args="-j8" --filters=//beacon/* --no-deps --args="v8_use_external_startup_data=false" out\Debug
