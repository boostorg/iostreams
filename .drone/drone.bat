@REM Copyright 2022 Peter Dimov
@REM Distributed under the Boost Software License, Version 1.0.
@REM https://www.boost.org/LICENSE_1_0.txt

@ECHO ON
setlocal

set "LIBRARY=%~1"
set "DRONE_BUILD_DIR=%CD%"

set BOOST_BRANCH=develop
if "%DRONE_BRANCH%" == "master" set BOOST_BRANCH=master
cd .. || exit /b 1
git clone -b %BOOST_BRANCH% --depth 1 https://github.com/boostorg/boost.git boost-root
if errorlevel 1 exit /b 1
cd boost-root || exit /b 1
git submodule update --init tools/boostdep
if errorlevel 1 exit /b 1
xcopy /s /e /q /y "%DRONE_BUILD_DIR%" "libs\%LIBRARY%\"
if errorlevel 2 exit /b 1
python tools/boostdep/depinst/depinst.py %LIBRARY%
if errorlevel 1 exit /b 1
cmd /c bootstrap
if errorlevel 1 exit /b 1
b2 -d0 headers
if errorlevel 1 exit /b 1

set "ZLIB_SOURCE=%CD%\libs\%LIBRARY%\test\zlib-1.2.11"
set "BZIP2_SOURCE=%CD%\libs\%LIBRARY%\test\bzip2-1.0.8"

if not "%CXXSTD%" == "" set CXXSTD=cxxstd=%CXXSTD%
if not "%ADDRMD%" == "" set ADDRMD=address-model=%ADDRMD%
b2 -j3 libs/%LIBRARY%/test toolset=%TOOLSET% %CXXSTD% %ADDRMD% variant=debug,release embed-manifest-via=linker
exit /b %ERRORLEVEL%
