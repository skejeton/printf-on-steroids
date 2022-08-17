
@pushd %~dp0

@if not exist bin\ mkdir bin

@if not exist bin\enet.lib (
    @pushd lib\enet
    cmake CMakeLists.txt
    msbuild ALL_BUILD.vcxproj
    @mkdir ..\..\bin
    @copy Debug\enet.* ..\..\bin\*
    @popd
)

if not exist bin\imgui.lib (
    @mkdir bin\imgui
    @pushd bin\imgui
    clang -g -c ..\..\lib\imgui\*.cpp
    lib /out:..\imgui.lib *.o
    @popd
)

set md=-Wl,-nodefaultlib:libcmt -D_DLL -lmsvcrt -Xlinker /NODEFAULTLIB:MSVCRTD
set libs=-lbin\imgui.lib

set enet_flags=-Ilib/enet/include -lbin\enet.lib -lWinmm -lWs2_32
set server_in=src\server\Main.cpp src\server\SokolImpl.cpp
set client_in=src\client\Main.c

clang %server_in% -oserver.exe -g -Isrc -Ilib %enet_flags% %libs% %md%
clang %client_in% -oclient.exe -g -Isrc -Ilib -Iinc %enet_flags% %md%

@popd