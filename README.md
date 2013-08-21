# Quake II for Oculus Rift

This is a Quake II engine mod to add full support for the Oculus Rift. 

This mod is based on [KMQuake II](http://www.markshan.com/knightmare/) and incorporates work from [RiftQuake](https://github.com/phoboslab/Quakespasm-Rift/).

##Requirements:
- Full version of [Quake II](http://store.steampowered.com/app/2320/)
- Windows
- Oculus Rift

##Downloads:
- Base Quake2-Rift package (10MB): [Download 1](http://dgho.st/nCiz) [Download 2](https://docs.google.com/uc?export=download&id=0B3vzoY9q6d-wSzh0YzZoVG9zZHc)
- Optional extras - HD textures, mission pack support (431MB): [Download 1](http://dgho.st/FvNC) [Download 2](https://docs.google.com/uc?export=download&id=0B3vzoY9q6d-wQ3BaQTNkUmhIYU0)
- Combined package (441MB): [Download 1](http://dgho.st/hI1A) [Download 2](https://docs.google.com/uc?export=download&id=0B3vzoY9q6d-wV2Zha1pMR1Rwc1U)

##Instructions:
1. Download binary package and unzip it to your preferred directory.
2. Optionally download the high resolution texture package and unzip it to the same folder.
3. Copy the following files from your Quake II `baseq2` folder into the new `baseq2` folder:
    - `pak0.pak`
    - `pak1.pak`
    - `pak2.pak`
    - `maps.lst`
    - `videos\` (optional - only if you want cinematics)
4. If you have the CD audio soundtrack in .ogg format you can optionally place these files in `baseq2\music\`. Music files need to be named `track02.ogg` to `track11.ogg`.
5. Run `kmquake2`. 

You can alternately just copy the contents of the extracted `Quake2-Rift` directory on top of an existing Quake II installation, however there is always the risk that this breaks the original installation.

If your Oculus Rift is connected and powered on it will enable support automatically at launch, but you can also enable Rift support by accessing the console using ~ and entering `vr_enable`. When it initializes it will attempt to locate the display that the Rift is configured as and use it, but if that fails it will default to the primary monitor.

As this is based on KMQuake II, it supports a large selection of advanced graphical settings and add-on's. I am mirroring a package with [Jim Waurzyniak's high resolution texture pack](http://www-personal.umich.edu/~jimw/q2/) and I highly recommend turning on some of the advanced video options, but if you want a relatively pure experience the defaults settings work very well.

## Console Commands:
You can access the console at any time using the ~ key and use the following commands:

- `vr_enable` - Enable Oculus Rift support
- `vr_disable` - Disable Oculus Rift support
- `vr_reset_home` - Resets the "home" position for the Rift in case it drifts.

Additionally, any of these commands can be bound to a key if you desire using the `bind` command.

## Additional variables:
In addition to the console commands, a variety of variables exist that can be used to adjust your experience. Where possible they are compatible with RiftQuake settings. These are the settings supported:

- `vr_aimmode` - Sets the current aim mode. Default is mode 6. Supported aim modes are:
    1. Head aiming 
    2. Head aiming + mouse pitch
    3. Mouse aiming
    4. Mouse aiming + mouse pitch
    5. Decoupled aiming within a deadzone. Aim pitch is relative to head.
    6. Decoupled aiming within a deadzone. Aim pitch is absolute.  
- `vr_aimmode_deadzone` - Sets the horizontal area of the mode 5/6 deadzone in degrees. Default is 30 degrees.
- `vr_autoenable` - Automatically enable the Rift when possible.
- `vr_crosshair` - Enables or disables the crosshair. Default is on, set to 0 to disable.
- `vr_crosshair_brightness` - Sets the brightness of the crosshair. Default is 75%.
- `vr_crosshair_size` - Sets the size of the crosshair. Minimum is 1 pixel, maximum is 5 pixels. Default is 3 pixels.
- `vr_hud_depth` - Sets the projected depth of the HUD and all 2D UI elements. Minimum is 0.25, maximum is 250. Default is 3.
- `vr_hud_fov` - Sets the horizontal field of view of the HUD. Default is 65 degrees.
- `vr_hud_transparency` - Enables or disables transparency on the 2D UI elements. Default is disabled, set to 1 to enable.
- `vr_ipd` - Set the prefered inter-pupillary distance in millimeters. Default is the value set in your user profile, or 64mm if not set.
- `vr_motionprediction` - Sets the amount of time used for motion prediction. Set to zero to disable, maximum time is 75ms. Default is 40ms.
- `vr_ovr_chromatic` - Enables or disables chromatic aberration correction. Default is on, set to 0 to disable.
- `vr_ovr_driftcorrection` - Enables or disables magnetic drift correction. If available it will use the calibration information set in your user profile, otherwise it will perform an automatic calibration. Default is off, set to 1 to enable.
- `vr_ovr_scale` - Sets the distortion compensation scaling factor. Minimum is 1.0, maximum is 2.0. Default is the value necessary to achieve a full-width image.

##Errata:

- KMQuake II (and Quake II) was designed to work with older GPU's and as such significant number of the graphical effects are processed executed on the CPU instead of on the GPU. While this generally means it runs well on modest hardware, the downside is that a few of the enhanced graphics options can cause performance problems at high resolutions even on systems with high end GPU's. If you encounter performance problem, try adjusting the distortion scale (values above ~1.25 are nearly impossible to differentiate), turning the resolution down, or disabling the bloom effect. 
- KMQuake II has an issue where messing around with the console during the loading screen either halts loading until you hit the escape key a couple times, or causes the program to crash due to something in the sound subsystem. I haven't been able to figure out the cause yet, so in the mean time try to avoid using the console during the loading screen.
- Since the 2D UI elements are projected at a fixed point in space, it can cause nausea if you move your head around too much when the game is not running. Beware of sneezing while using the console or while the game is loading a map. I hope to have a fix for this in a future release.
- Setting the character to left-handed causes the weapon model to be rendered incorrectly. Don't do it unless you want a headache. This might be fixed in a later release.
- View models appear very large. This is due to them using the original meshes, and unfortunately there doesn't appear to be an easy way to scale the meshes programmatically.
- Linux is unsupported at this time, although it might be in the future. The changes necessary to support the Oculus Rift guarantee that it is currently broken. If I have time I'll tidy it up for a future release, or if someone wants to submit a patch for it I would gladly merge it. 
- OS X is unsupported and will likely never be supported. The official source code for Quake 2 never included proper OS X support, and the KMQuake II source doesn't support it either.

#Build Instructions:

1. Clone or copy the repository into a directory of your choice.
2. Copy the `LibOVR` folder from the Oculus Rift SDK into the root directory of your repository.
3. Open `quake2.sln` in Visual Studio. If you are using Visual Studio 2012 and you want to submit pull requests I would appreciate it if you do not upgrade the solution.
4. Build it for your desired profile.

#Acknowledgements:

Thanks go out to:

- id Software, for releasing such an awesome game 16 years ago and then being generous enough to release the source code to the community.
- Knightmare and [KMQuake II](http://www.markshan.com/knightmare/). The general cleanliness of the code base and significant graphical upgrades responsible for this turning out as well as it did.
- [Dominic Szablewski](https://github.com/phoboslab) and [John Marshall](https://github.com/swax) for their work on RiftQuake. It served as the basis for this project, and in some places their code remains.
- Jim Waurzyniak for taking the time and effort to host a collection of [high-resolution assets for Quake II](http://www-personal.umich.edu/~jimw/q2/).