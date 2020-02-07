rem @echo off
setlocal
set PATCH_PATH=%CD%\chromium_patches
set TARGET_PATH=%CD%\..

call :APPLY "%PATCH_PATH%", "%TARGET_PATH%"
call :APPLY "%PATCH_PATH%\third_party\skia", "%TARGET_PATH%\third_party\skia"

exit /B %ERRORLEVEL%

:APPLY
echo FIRST: %~1
echo SECOND: %~2
for %%F in ("%~1\*.patch") do @(
	pushd %~2
	echo Applying %%F
	git apply --ignore-whitespace --whitespace=nowarn %%F
	popd
)
exit /B 0