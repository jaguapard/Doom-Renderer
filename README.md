# Doom Rendering
This is a fully 3D classical Doom software level renderer, written from scratch using SDL2. Currently, there are no plans on implementing enemies, things, weapons, and anything else not directly relevant for graphics.

# Dependencies and compilation
The project is written for Visual Studio 2022 and corresponding version of it's inbuilt compiler.
The project uses SDL2 and self-made libraries. SDL2 can be downloaded from their site, self-made ones are included in the  `src/bob` directory

# How to run?
You will need a commercial Doom 2 WAD file in the root of the project directory (if you are running from Visual Studio), or in the same directory as the compiled exe. The program will load MAP01 automatically, maps can be changed by inputting their number with keyboard (i.e. pressing 1, 5 will warp you to MAP15). Map numbered less than 10 have a leading 0 in their name (MAP02, MAP05, etc).

You will also need all the textures in PNG format unpacked and put into `data/graphics` directory.