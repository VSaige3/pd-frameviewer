INCLUDE_PATH = ./include/

all:
	gcc -o scraper.exe scraper.c scraper.h $(INCLUDE_PATH)curses.h
	gcc -c frameviewer.h frameviewer.c
	gcc frameviewer.o -L./lib/ -l:pdcurses.a -o frameviewer.exe

