@echo off
setlocal
set "taskProjectRoot=%~dp0..\Mary"
set "taskTestDir=%TEMP%\xtrade-request-protocol-%RANDOM%"
mkdir "%taskTestDir%"
pushd "%taskTestDir%"
cl /MP /nologo /w /EHsc /std:c++20 /Zc:__cplusplus /utf-8 /MDd /DPROTOBUF_USE_DLLS /I"%taskProjectRoot%\request" /I"%taskProjectRoot%\include\debug\protobuf" /I"%taskProjectRoot%\include\debug\libevent" /I"%taskProjectRoot%\include\debug\rapidjson" /FeRequestProtocolTests.exe "%~dp0RequestProtocolTests.cpp" "%taskProjectRoot%\request\request.cpp" "%taskProjectRoot%\request\request.pb.cc" "%taskProjectRoot%\request\v1\market.pb.cc" "%taskProjectRoot%\request\RequestCenter.cpp" "%taskProjectRoot%\request\MarketTypes.cpp" /link /LIBPATH:"%taskProjectRoot%\libs\debug" libprotobufd.lib abseil_dll.lib ws2_32.lib
set "taskCompileResult=%errorlevel%"
popd
if not "%taskCompileResult%"=="0" exit /b %taskCompileResult%
set "PATH=%taskProjectRoot%\libs\debug;%PATH%"
"%taskTestDir%\RequestProtocolTests.exe"
exit /b %errorlevel%
