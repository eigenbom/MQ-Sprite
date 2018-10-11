<p align="center">
  <img width="64" height="64" src="https://github.com/eigenbom/MQ-Sprite/raw/origin/master/icons/application_icon_64.png">
</p>

## Synopsis

MQ Sprite is a tool for editing and managing game sprites. It was built during the development of [MoonQuest](https://www.playmoonquest.com) and has been recently updated with a cleaner UI and new features. The editor has been designed for games that use low resolution pixel art.

Download the beta: [MQSprite.zip (Windows, 32-bit)](https://github.com/eigenbom/MQ-Sprite/releases/download/beta/MQSprite.zip)

## Features

MQ Sprite is primarily aimed at managing sprites and adding metadata (such as anchors, pivots, and arbitrary JSON). MQ Sprite is best suited for custom game engines or engines without these tools.

The main features of MQ Sprite are:

* Previews of all sprites in a project;
* Folders for organising sprites;
* Basic pixel art tools (pencil, eraser, colour pick, flood fill, copy-paste, undo-redo);
* An animation editor supporting multiple animations per sprite;
* Anchors and pivots for defining attachment points in a sprite (such as the handle of a sword);
* Drop-shadow and onion-skinning;
* A simple save format (A single JSON file + PNG images for all frames);
* Bundled Python scripts for manipulating the save files in your toolchain; and
* A general properties window for adding arbitrary meta-data (in JSON format);

![Screenshot](https://github.com/eigenbom/MQ-Sprite/raw/origin/master/screenshots/screenshot_kyrise.png "Screenshot")

## What MQ Sprite Isn't

MQ Sprite isn't a full-fledged sprite editor and only supports basic sprite editing. For more complex sprites a tool like [ASEprite](https://www.aseprite.org/) will do a much better task. (A script for importing .ase files into MQ Sprite is coming soon.)

MQ Sprite is primarily a sprite browser and metadata editor. If you are using a game engine like Unity, Game Maker, or Godot, then you *won't need this tool*, as those systems already offer similar functionality.

## Build Instructions

MQ Sprite requires Qt5 and can be built directly from within QtCreator. It has no other dependencies.

## Toolchain

(This section will document the Python scripts.)

## Credits

MQ Sprite was built by [Benjamin Porter](https://twitter.com/eigenbom).

[Gentleface Icons](http://www.gentleface.com/free_icon_set.html) are licensed under Creative Commons Attribution-NonCommercial.

Sprites in example project: [Kyrise's Free 16x16 RPG Icon Pack | Graphics made by Kyrise](https://kyrise.itch.io/)

Paint tool palette courtesy of Davit Masia.

Icon adapted from Moon Phase Outline icon by [Dave Gandy](https://www.flaticon.com/authors/dave-gandy) from [flaticon.com](www.flaticon.com) is licensed by Creative Commons BY 3.0.
