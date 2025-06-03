INCLUDE_PATH = "./include/"

all:
	gcc -o scraper.exe scraper.c scraper.h -I$(INCLUDE_PATH)
	gcc -c frameviewer.h frameviewer.c
	gcc frameviewer.o -L./lib/ -l:pdcurses.a -o frameviewer.exe

