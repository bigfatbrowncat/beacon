@echo off
setlocal
set PATCH_PATH=%CD%\chromium_patches
set TARGET_PATH=%CD%\..
for %%F in ("%PATCH_PATH%\*.patch") do @(
	pushd %TARGET_PATH%
	echo Applying %%F
	git apply --ignore-whitespace --whitespace=nowarn %%F
	popd
)
