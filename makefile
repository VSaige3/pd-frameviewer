INCLUDE_PATH = ./include/

all:
	gcc -o scraper.exe ./src/scraper.c ./src/scraper.h
	gcc -I$(INCLUDE_PATH) -c ./src/frameviewer.h ./src/frameviewer.c
	gcc frameviewer.o -L./lib/ -l:pdcurses.a -o frameviewer.exe

