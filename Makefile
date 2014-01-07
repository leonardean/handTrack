all:
	rm -rf a.out
	g++  `pkg-config --libs --cflags opencv x11` main.cpp 

