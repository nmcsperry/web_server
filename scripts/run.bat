@for %%I in (.) do @set CurrentDirectory=%%~nI%%~xI
@setlocal
@if "%CurrentDirectory%" == "scripts" @cd ..
@if "%CurrentDirectory%" == "build" @cd ..
@if not "%CurrentDirectory%" == "run" @if not exist "run" mkdir run
@if not "%CurrentDirectory%" == "run" @cd run

..\build\web_server.exe

@endlocal