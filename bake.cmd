
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
set libs=-lbin\enet.lib -lbin\imgui.lib -lWinmm -lWs2_32
set lib_inc=-Ilib/enet/include -Ilib/imgui

clang -g server\Main.cpp server\SokolImpl.cpp -I. %lib_inc% %libs% %md%
rem clang -g src\MumboJumbo.c

@popd