./bin/libenet.a:
	$(CC) -c lib/enet/*.c -Ilib/enet/include
	$(AR) rcs bin/libenet.a *.o
	rm *.o

./bin/libimgui.a:
	$(CC) lib/imgui/*.cpp -c
	$(AR) rcs bin/libimgui.a *.o
	rm *.o

