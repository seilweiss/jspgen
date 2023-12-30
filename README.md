# jspgen
jspgen is a command-line tool for generating JSP files to use in mods of SpongeBob SquarePants: Battle for Bikini Bottom. It takes in a RenderWare DFF file and outputs a JSP file, ready to be imported into the JSPINFO layer of a HIP/HOP archive.

At the moment, jspgen does not support multiple DFFs.

## Background
In BFBB's HIP/HOP archives, the level models (terrain, buildings, etc.) are split into two parts:
* RenderWare DFF files, which make up all the geometry for the level. These are stored in the "BSP" layers.
* A JSP file, which contains the collision tree and rendering flags for the level. This is stored in the "JSPINFO" layer.

JSP is a custom file format created by Jason Hoerner at Heavy Iron Studios as a replacement for the RenderWare BSP format, which was used in the previous game Scooby-Doo! Night of 100 Frights. It contains two sections:
* A BSP tree, which is a K-D tree with overlap regions (a.k.a. Bounding Interval Hierarchy). The tree's leaf nodes contain special flags that specify which triangles are solid, visible in the world, able to be stood on by the player, should receive shadows, etc.
* A JSP node list, which contains rendering flags that specify which "nodes" (objects within the DFFs, a.k.a. atomics) should render to the Z-buffer or use backface culling.

On GameCube, the JSP contains a third section containing a list of all the triangles' vertices from each DFF file. This might due to GameCube DFFs using native data, causing triangles/indices to not be accessible in a platform-independent way (although, PS2 DFFs also use native data but PS2 JSPs don't contain this section for some reason).

BFBB also supports BSP in the same way as Scooby (i.e. one BSP file in the BSP layer and no JSPINFO layer). The downside is none of the new triangle and rendering flags are present in BSP (things like no-stand, force shadows, and disabling Z-buffer write aren't supported).

More info: https://heavyironmodding.org/wiki/EvilEngine/JSP

## Usage
    jspgen -p <platform> <input .dff path> <output .jsp path>

* `-p <platform>` - Target platform
  * gc - GameCube
  * ps2 - PlayStation 2
  * xbox - Xbox
* `<input .dff path>` - Path to existing RenderWare DFF file
* `<output .jsp path>` - Path of JSP file to create

Example:

    jspgen -p gc test.dff test.jsp

## Guide for Modders
This guide assumes you have some basic experience with [Industrial Park](https://heavyironmodding.org/wiki/Industrial_Park_(level_editor)) and importing custom models. I recommend reading [this guide](https://heavyironmodding.org/wiki/Essentials_Series/Custom_Models) first if you've never done it before.

### Creating a level model

BFBB's level models are stored as RenderWare DFF files. These are the same type of DFF files as all the other models in the game, so in theory you could for example use spongebob_bind.dff as a level model (not saying you should, but... it's possible).

When creating a level model, I recommend using [Blender](https://www.blender.org/) with the [DragonFF](https://github.com/Parik27/DragonFF) addon installed. You can make your own custom level model, or, if you want to edit an existing level's model, you can go to the BSP layer in any HOP archive, export the JSP asset as a .dff file, import it into Blender, and make any edits you want to make.

When you have finished your model in Blender, export it as a .dff file using DragonFF.

**Important: Make sure to set the Version to GTA VC (v3.4.0.3) in the export dialog. BFBB only supports this version.**

### Generating the JSP

Once you have your DFF file ready, you will need to download jspgen if you haven't already. Go to the [Releases](https://github.com/seilweiss/jspgen/releases) page and download the latest jspgen.exe.

Then, open a Command Prompt and `cd` to the folder you downloaded jspgen.exe to. For example:

    cd "C:\Users\<your name>\Downloads"

Then, run jspgen with `-p <platform>` (whichever platform you are modding the game on) followed by the path to your DFF file then the path you want to save the JSP file at. For example:

    jspgen -p gc "C:\Modding\BFBB\my_model.dff" "C:\Modding\BFBB\my_model.jsp"

If all goes well, the JSP file will be created at the path you specified. The Command Prompt window should also display some stats about the newly created JSP. For example:

    Branch nodes: 17751
    Triangles: 33539
    Max BSP depth reached: 31

If an error occurs or the program exits without displaying anything, the DFF is likely unsupported:
* DFF files with native data are unsupported. If you exported a DFF from the GameCube or PS2 version of a Heavy Iron game it likely has native data, so try exporting from the Xbox version instead.
* Make sure you exported the DFF as the right version in Blender: GTA VC (v3.4.0.3).
* If you get the error "RwStream error: Failed to open file", check your paths. Surround them with quotes if they contain spaces ("My Model.dff").

**Please note: jspgen currently only builds JSPs for SpongeBob SquarePants: Battle for Bikini Bottom. Other Heavy Iron games are not supported.**

### Importing into Industrial Park

Once you have the DFF and JSP ready, open Industrial Park. We will import both files into the level.

#### .HOP file
If you're editing an existing level, open that level's .HOP file. If you're creating a level from scratch, make sure the .HOP file contains at least one BSP layer followed by one JSPINFO layer (blank.HOP from IndustrialPark-EditorFiles should already have these setup).

Go to the BSP layer(s) and JSPINFO layer and delete the assets currently there, if any.

Go to the first BSP layer and click Import. Click Import Raw Data and select your DFF file. Set Asset Type to JSP. Then click OK.

Go to the JSPINFO layer and click Import. Click Import Raw Data and select your JSP file. Set Asset Type to JSP. Then click OK.

If necessary, import the textures used by your DFF into the TEXTURE layer (see [this guide](https://heavyironmodding.org/wiki/Essentials_Series/Custom_Models#Import_Textures) for instructions). Make sure "Append .RW3" is enabled in the Import Textures dialog.

#### .HIP file
After importing the DFF, JSP, and textures, open the .HIP file.

In the DEFAULT layer, find the Environment (ENV) asset or create one from a template (blank.HIP from IndustrialPark-EditorFiles should already have one).

Select the Environment asset and click Edit Data. Set BSP to your JSP asset in the JSPINFO layer (copy and paste its name).

**Important: If you ever rename your JSP, make sure the reference to it is updated in the Environment asset.**

### Testing the level

Save the .HIP and .HOP files, boot up the game, and load into your level. Hopefully, your custom level model shows up and has collision! Hopefully the performance is also decent; the JSP's collision tree should be fairly optimized for levels around the same size and detail as BFBB's vanilla levels.

Note: If you edited an existing level's model, it may look or behave slightly different. This is because some JSP features aren't supported yet:
* All objects have collision enabled by default.
* No-stand isn't supported, so the player may be able to stand on objects they couldn't before.
* Some objects may not receive shadows.
* All objects will render with Z-buffering enabled and backface-culling by default.
* All material indices are currently set to -1, which may cause problems with Surface Mapper (MAPR) assets. This may cause things like OOB surfaces to not work correctly.

### Troubleshooting
* Battle for Bikini Bottom is currently the only supported game.
* Make sure to export the DFF from Blender using version GTA VC (v3.4.0.3).
* If jspgen is throwing "RwStream error: Failed to open file", check your JSP or DFF paths. Surround them with quotes if they contain spaces ("My Model.dff").
* Make sure to import your assets to the right files and layers.
* Make sure the layers are ordered correctly (JSPINFO must be after BSP layers).
* Check the Environment asset in your .HIP file and ensure the JSP asset is referenced correctly.

If you need more help, feel free to DM me @ `seil` on Discord. I also recommend joining the [Heavy Iron Modding](https://discord.gg/9eAE6UB) Discord server.