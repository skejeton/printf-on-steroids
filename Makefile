./bin/libexternal.a:
	$(CC) -c lib/enet/*.c -Ilib/enet/include
	$(AR) rcs bin/libenet.a *.o
	rm *.o

./bin/libserver.a:
	$(CC) lib/imgui/*.cpp -c
	$(CXX) -I. server/SokolImpl.cpp -c
	$(AR) rcs bin/libserver.a *.o
	rm *.o

all: ./bin/libenet.a ./bin/libserver.a