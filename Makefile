LIBRARIES=-lc bin/libenet.a
FLAGS?=-O3
INCLUDES=-Ilib/enet/include -Isrc -Ilib -Iinc


./bin/libenet.a:
	mkdir -p bin
	$(CC) $(FLAGS) -c lib/enet/*.c -Ilib/enet/include
	$(AR) rcs bin/libenet.a *.o
	rm *.o

./bin/libserver.a: 
	mkdir -p bin
	$(CXX) lib/imgui/*.cpp -c
	$(CXX) $(FLAGS) -Isrc -Ilib ./src/server/SokolImpl.cpp -c
	$(AR) rcs bin/libserver.a *.o
	rm *.o

%.test.out: %.c
	$(CC) $(INCLUDES) $(FLAGS) $< -o $@ && $@

all: ./bin/libenet.a ./bin/libserver.a
	mkdir -p bin
	$(CC) src/ClientMain.c -lpthread $(FLAGS) $(LIBRARIES) $(INCLUDES) -o bin/printf2_client
	$(CXX) src/ServerMain.cpp bin/libserver.a -lGL -lXcursor -lXi -lX11 $(FLAGS) $(LIBRARIES) $(INCLUDES) -o bin/printf2_server
	$(CC) src/ClientLibrary.c $(FLAGS) $(INCLUDES) -c
	$(LD) -r *.o bin/libenet.a -o Out.o
	objcopy --keep-global-symbols=GlobalSymbols.txt Out.o Out.o
	$(AR) rcs bin/libprintf2.a Out.o
	rm Out.o

package: all
	mkdir -p package
	mkdir -p package/GUI
	cp bin/printf2_server package/GUI/Printf2Monitor
	mkdir -p package/GUI/data
	cp data/Roboto.ttf package/GUI/data
	mkdir -p package/Library
	cp bin/libprintf2.a package/Library
	cp inc/P2.h package/Library

clean:
	rm -rf bin
	rm -rf package