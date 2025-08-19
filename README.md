<p align="center" style="padding-top:2em">
  <img src="doc/rts4ds.png?raw=true">
</p>
<p align="center">
  <i>Powering real-time strategy games on your Nintendo DS</i>
</p>

## About

RTS4DS is a game engine that enables real-time strategy games on the Nintendo DS. The engine requires game assets -- i.e., graphics, sounds, music, maps, descriptions of structures and units etc. -- and sets up the entire strategy game accordingly on the hardware of the handheld. Enormous battles can take place on the two small screens, with touch input helping you to direct your troops and turn the battle in your favour. When we say enormous battles, they are truly that for the Nintendo DS hardware: up to 300 structures, 150 units, 150 projectiles, and 150 explosions occurring simultaneously and without slowdown. Enormous and enormously fun.

Development of the RTS4DS engine began in early 2007 with the aim of being able to run custom real-time strategy games on the Nintendo DS that were not unlike the classic "Dune II - The Building of a Dynasty". Many improvements and enhancements beyond the first generation RTS games were implemented in the engine almost immediately: selection boxes, auto-commands, rallying points, build queues, hotkeys, and objectives more diverse in nature. Getting that all to work on the small Nintendo DS, however, took careful planning and innovative solutions. RTS4DS truly pushes the limits of the hardware -- and successfully. Additionally, the game engine supports two Nintendo DS peripherals: the Rumble Pak for jolting and shaking the handheld during explosions and the Guitar Grip from Guitar Hero for additional hotkeys. (Of course, one has to choose between these peripherals as they do not both fit into the GBA-slot of the Nintendo DS simultaneously; we prefer the Rumble Pak ourselves.)

In late 2007, Sander Stolk posted the news that he was looking for developers to create original real-time strategy games on top of his engine. Soon after he met Lee David Ash at Violation Entertainment. Lee had already planned a top-down hovertank shooter for the platform, and the two developers began Ulterior Warzone, using prerendered 3D models for most of the graphics.


## Ulterior Warzone ![](logo_uw.bmp?raw=true)

With the 20th anniversary of the Nintendo DS handheld, we have released a demo version of Ulterior Warzone, our attempt at a full game development project. Ulterior Warzone was to be a futuristic real-time strategy game for the Nintendo DS handheld, powered by the RTS4DS engine created by Sander Stolk, and developed by Violation Entertainment. Claiming to be "the biggest war on the DS", the game features over 100 maps, half an hour of prerendered cinematics and music, full 'talkie' story-driven campaigns and skirmish battles that can easily average over an hour each. Hundreds of units and structures can be interacted with across large complex maps, with intertwined objectives for singleplayer scenarios that complement an intricate storyline narrative, and dozens of different units and structures available for multiple factions.

Until now, Ulterior Warzone could be considered vapourware, but the demo version has now finally been released along with base content and the RTS4DS engine source code, which we're releasing as open-source and moddable. The demo is now compiled as a single self-contained binary file (.nds) that runs on the Nintendo DS but also on Nintendo DSi and Nintendo 3DS through backwards compatibility on these devices. Moreover, the build is compatible with the majority of Nintendo DS emulators. We are happy that everyone can now play (and even mod and develop with) Ulterior Warzone under a creative commons license.

**Find the game at [the official Ulterior Warzone webpage](https://www.violationentertainment.com/uw/) of Violation Entertainment.**


>📄 When running the game on the Nintendo DS, DSi, or 3DS, please use a good homebrew launcher that supports argv (e.g., the [NDS homebrew menu](https://github.com/devkitPro/nds-hb-menu)). Doing so helps RTS4DS find the location of itself (i.e., the .nds binary file) in which the assets for the RTS to be played are stored and from which they need to be retrieved during gameplay. If running the game from a flashcard, please note that the binary file may need to be patched first with the necessary operations for accessing the filesystem of that specific flashcard (see [DLDI patching](https://www.chishm.com/DLDI/)). If you are having trouble with loading up the game on EZ-Flash V, you can find another version of nds-hb-menu attached [here](https://github.com/devkitPro/nds-hb-menu/issues/16) specifically for EZ-Flash V flashcards that boot into Moonshell or, from the latest EZ-Flash V firmware, EZ5Shell. Emulators such as [DeSmuME](https://desmume.org/) tend to take care of launching the binary file in the right manner, including passing argv and taking care of filesystem access. Saving your game will work on original hardware, though emulators may not support it. In case of the latter, it should be possible to use emulator save states instead.


## Source code 📝

### Compiling

The current version of RTS4DS has been compiled with [devkitPro](https://github.com/devkitPro/installer/releases/tag/v3.0.3). Installing their tools and libraries should provide an environment in which one can compile RTS4DS. In order to compile it for demos of the game Ulterior Warzone, please first copy [Ulterior Warzone demo1](https://github.com/LDAsh72/uw/) assets into 'fs/uw_demo1/rts4ds/uw_demo1/' and run ```make uw_demo1``` in a command prompt or, if it is the second demo, copy its assets into 'fs/uw_demo2/rts4ds/uw_demo2/' and run ```make uw_demo2```, etc. If one simply runs ```make```, without any arguments, assets from the 'fs/default' folder are used in the compilation process. Make sure to run ```make clean``` successfully before switching compilation to another assets directory. If you are interested in making your own RTS using this engine, I suggest taking a look at the assets from Ulterior Warzone and the documentation offered on their configuration.

### Libraries

RTS4DS uses a number of libraries, most of which are maintained by [devkitPro](https://github.com/devkitPro/):
* [`libnds`](https://github.com/devkitPro/libnds) - for addressing the hardware of the Nintendo DS handheld (v1.7.1)
* [`libfilesystem`](https://github.com/devkitPro/libfilesystem) - for supporting the NitroFS filesystem, typically appended to NDS executables
* [`libfat`](https://github.com/devkitPro/libfat) - for supporting filesystems available on the Nintendo DS flashcarts
* [`maxmod`](https://github.com/devkitPro/maxmod) - for playing mod files, in order to support music

Beyond devkitPro, one other library is used by RTS4DS:
* [`giflib`](https://giflib.sourceforge.net/) - for reading animated GIFs, in order to support cutscenes and other animations

### Directory structure

* `doc` - stores files for documentation purposes
* `fs` - storage for assets of different RTS games
    * `default` - default subfolder used in compilation
    * `uw_demo1` - subfolder intended for compiling the first demo of the RTS "Ulterior Warzone"
    * `uw_demo2` - subfolder intended for compiling the second demo of the RTS "Ulterior Warzone"
* `source` - stores the source code of RTS4DS
    * `astar` - contains code for A* pathfinding, developed by sverx specifically for RTS4DS
    * `gif` - contains `giflib` (see section Libraries above)

### Main source folder

A brief overview of the files in the main folder for source code is in order here. 
The file `main.c` contains the entry point for execution of RTS4DS, initializing the Nintendo DS hardware and starting the game. The file `game.c` keeps track of the current game state and delegates logic and drawing to different files accordingly (e.g., to `intro.c`, `menu_main.c`, `menu_settings.c`, `cutscene_briefing.c`). Each game state will at least offer an *init* function to set up that game state, a *load graphics* function, a *draw* function, a *draw background* function, and a *do logic* function. Unlike the aforementioned game states, ingame is delegated over two files: `playscreen.c` and `infoscreen.c`. The former takes care of the map, structures, units, projectiles, etc. (using `environment.c`, `structures.c`, `units.c`, `projectiles.c`); the latter displays a radar, a build queue, and hotkeys that one can assign for selecting specific units or structures. For those who are interested in nifty use of Nintendo DS hardware, noteworthy are `sprites.c` (utilizing the 3D engine of the Nintendo DS to render beyond the 128 sprite limit of the 2D engine), `animation.c` (rendering entire cutscenes by combining animated GIFs with music and subtitles), `rumble.c` (for rumble support during explosions with the Rumble Pak accessory), and `screenshot.c` (for taking screenshots and writing them to the filesystem as BMPs).


## Credits and thanks 👍

Only two people worked on the RTS4DS source code itself. Since the start of development of Ulterior Warzone, however, the two of us essentially formed part of the team that developed that game and we did not think of our work as strictly separate ventures. The RTS4DS engine would not have evolved as far as it did without an original real-time strategy game to take full advantage of its features, to playtest it, and to demand improvements or additional features to improve the players' experience. Moreover, working the many years with Lee David Ash in order to realise an actual game as opposed to only a game engine was thrilling and rewarding. The credits listed below are therefore those for Ulterior Warzone as a whole rather than singling out the RTS4DS engine underlying it.

* Sander Stolk -- *lead programmer*
* Claudio Gabriele Quercia -- *additional coder*
* Lee David Ash -- *lead artist*
* Bojana Nedeljkovic, Daniel Schneider -- *additional art*

A thank you to the following people: 
* Michael Noland (joat), Jason Rogers (dovoto), Dave Murphy (WinterMute), and fincs for the `devkitPro` tools and libraries
* Rafael Vuijk (DarkFader), Dave Murphy (WinterMute), and Alexei Karpenko for the Nintendo DS rom tool included in the devkitPro toolchain
* Gershon Elber, Eric S. Raymond, and Toshio Kuratomi for the `giflib` library
* Michael Chisholm (chishm) for the DLDI interface and patching mechanism
* the gbadev community (no longer around, but [archived](https://web.archive.org/web/20220104163926/https://forum.gbadev.org/)) for their advice and valuable know-how
* "retrohead", "LiraNuna", "strager", "Dwedit", "aaron", "strager", "Teddemans", "Devil_Spawn", "Takieda", "Tommy T", "Flying Arrow", Justin Nance for their encouragement, advice, help, and friendship

Special thanks go out to my parents and to my wife, Fenja, for their support, encouragements, patience, and love.


## License

This code is copyrighted by [Sander Stolk](https://orcid.org/0000-0003-2254-6613)
and released under the [MIT](https://spdx.org/licenses/MIT) license.

