# Doom Rendering
This is a fully 3D classical Doom software level renderer, written from scratch using SDL2. Currently, there are no plans on implementing enemies, things, weapons, and anything else not directly relevant for graphics.

# How to run?
You will need a commercial Doom 2 WAD file in the root of the project directory (if you are running from Visual Studio), or in the same directory as the compiled exe. The program will load MAP01 automatically, maps can be changed by inputting their number with keyboard (i.e. pressing 1, 5 will warp you to MAP15). Map numbered less than 10 have a leading 0 in their name (MAP02, MAP05, etc).

You will also need all the textures in PNG format unpacked and put into `data/graphics` directory.