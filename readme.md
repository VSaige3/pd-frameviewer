Frameviewer
==========

Frameviewer is a small command line program/gui that lets you see some frame 
data for skills. Currently supports viewing total duration, startup frames, 
and active frames (for melee attacks and non-auto shields).

Usage
----------

To use the frameviewer, run it in a maximized console window. If the window is 
too small it will fail to set up curses and exit. This will look for a file called 
"config.dat" to find the path to gsdata, and your skill entry method. On first run 
you'll have to provide this program with the path to your gsdata file 
(this is in "/Assets/Data/gstorage/gsdata_en.dat" from your phantom dust dump).
    YOU MUST DUMP THE GAME TO USE THIS PROGRAM
    (or ask somebody who has for a gsdata file)
If you entered it wrong or wish to change it just edit the contents of "config.dat". 
The first line of config is the gsdata path, the second is your entry method ('h' for
hex skill index, 's' for skill name).

Navigation
-----------

Use WASD to navigate menus, use space to select

Notes
----------

Anything in the "./fdata" directory is treated as an animation length file. Put the results of 
Here is what each is:

* pc00a: Player
* pc01a: Edgar
* pc02a: Freia
* pc03a: Meister
* pc04a: Chunky
* pc05a: Cuff Button
* pc06a: pH
* pc07a: Know
* pc08a: Tsubutaki
* pc09a: JD
* pc10a: Sammah

* enm00a: Ommato
* enm01a: Scoto
* enm02a: Claustro
* enm03a: Catoptro
* enm04a: Mechano
* enm05a: Gyne
* enm06a: Euroto
* enm07a: Hedono
* enm08a: Anthro
* enm09a: Andro
* enm10a: Vestio
* enm11a: Partheno
* enm12a: Guard
* enm13a: Ceno
* enm14a: Germano
* enm15a: Belono

Framescraper
=============

This tool is used to grab animation lengths from the alr files. Does not produce
readable files, they are to be used with the framviewer. If you must know the format,
they are just a list of packed floats representing the length of each animation.

Usage
-------------

To use the scraper, drag any number of alr files over the executable, or run it with 
them as command line arguments. Note that not every alr will work, only ones from "player/..."
(these are the model files for playable characters). Then place these fdata files in the "./fdata" directory.