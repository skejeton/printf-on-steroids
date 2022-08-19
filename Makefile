CC=clang
CXX=clang++
AR=ar

./bin/libenet.a:
	$(CC) -c lib/enet/*.c -Ilib/enet/include
	$(AR) rcs bin/libenet.a *.o
	rm *.o

./bin/libserver.a:
	$(CXX) lib/imgui/*.cpp -c
	$(CXX) -Isrc -Ilib ./src/server/SokolImpl.cpp -c
	$(AR) rcs bin/libserver.a *.o
	rm *.o

all: ./bin/libenet.a ./bin/libserver.a