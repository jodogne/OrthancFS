CPP = g++
CPPFLAGS = -Wall -Werror -ldl `pkg-config fuse --libs` `pkg-config fuse --cflags`

default: src/orthancfs.cpp include/orthancfs.h
	$(CPP) $(CPPFLAGS) -o ofs src/orthancfs.cpp
	
clean:
	rm ofs
	
test:
	./ofs http://localhost:8042 omount/
