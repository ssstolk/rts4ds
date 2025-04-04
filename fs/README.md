# Filesystem and game assets

Each of the folders here is meant to store assets intended for a specific build of RTS4DS.

* *rts4ds* (default)\
Place game assets into the folder. See the Ulterior Warzone documentation on what is needed.
Compile the game afterwards by running ```make```.

* *uw_demo1*\
Copy assets from the Ulterior Warzone demo [`uw_demo1`](https://github.com/LDAsh72/uw/tree/main/base/uw_demo1) into the uw_demo1/uw_demo1 folder.
Compile the game afterwards by running ```make uw_demo1```.


Performing the steps above will incorporate those game assets into the resulting .nds file.
Alternatively, it is possible to let the engine read game assets from the filesystem instead. To do so, please compile the game without placing the asset files in their respective directories. The resulting .nds file will be able to find those same asset files if you place them in a directory with that name in the root folder of your filesystem.