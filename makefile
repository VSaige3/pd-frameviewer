all:
	gcc -o scraper.exe scraper.c scraper.h
	gcc -c frameviewer.h frameviewer.c
	gcc frameviewer.o -l:pdcurses.a -o frameviewer.exe
