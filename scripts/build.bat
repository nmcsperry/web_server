@mkdir build
@pushd build
cl /ZI /Fe:web_server.exe ..\source\start.c user32.lib gdi32.lib opengl32.lib Ws2_32.lib
@popd