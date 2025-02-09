@for %%I in (.) do @set CurrentDirectory=%%~nI%%~xI
@setlocal
@if "%CurrentDirectory%" == "scripts" @cd ..
@if "%CurrentDirectory%" == "run" @cd ..
@if not "%CurrentDirectory%" == "build" @if not exist "build" mkdir build
@if not "%CurrentDirectory%" == "build" @cd build

cl /ZI /Fe:web_server.exe ..\source\start.c user32.lib gdi32.lib opengl32.lib Ws2_32.lib

@endlocal