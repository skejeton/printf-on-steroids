
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
set libs=-lbin\imgui.lib -lWinmm -lWs2_32
set lib_inc=-Ilib/imgui -Ilib\thread\src

set enet_flags=-Ilib/enet/include -lbin\enet.lib
set server_in=server\Main.cpp server\SokolImpl.cpp
set client_in=client\Main.c

clang %server_in% -oserver.exe -g -I. %enet_flags% %lib_inc% %libs% %md%
clang %client_in% -oclient.obj -g -I. %enet_flags% -c

@popd