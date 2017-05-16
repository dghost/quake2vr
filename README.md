### Quake II VR - v2.0.0 (alpha)
2017/05/15

[Luke Groeninger](https://github.com/dghost) - Quake II VR project  
[Malcolm Smith](https://github.com/mscoder610) - This release (CV1 + partial Touch support + other updates)

This is a Quake II engine mod to add full support for Virtual Reality HMD's such as the Oculus Rift.
This mod is based on [KMQuake II](http://www.markshan.com/knightmare/) and incorporates work from [RiftQuake](https://github.com/phoboslab/Quakespasm-Rift/).  
This version was created from the [libovr-1.xx branch of Quake II VR](https://github.com/q2vr/quake2vr/tree/libovr-1.xx).

Note: This version targets the Oculus SDK only, not Steam VR. It may work with the Vive in conjunction with Revive, but it hasn't been tested.

#### Features
New in this version:

- Oculus CV1 (consumer version) support with LibOVR 1.12
- Partial Oculus Touch support. Touch can be used as a gamepad input. Additionally, if "VR Controller Support" is enabled in the game options (VR section) (enabled by default for right-hand aiming), the Touch controllers can be used to aim weapons (orientation only). Positional weapon tracking is not supported.
- VR Comfort Turning (enabled by default, 45 degree increments). Can be disabled in game options, VR section. There are also two new console commands "comfortturn_left" and "comfortturn_right", so comfort turning can be used in conjunction with more aimmodes; bound to DPad Left and Right by default.
- Experimental VR auto-crouch feature (disabled by default), which lets you crouch in real-life to trigger in-game crouch. May be buggy. To enable, type 'vr_autocrouch 1' at the console
- VR Supersampling (Render Target Multiplier, Pixels per Display Pixel Override) can be set explicitly in game options (VR, Advanced).

Other features:

- Projected HUD/2D UI elements
- Decoupled view and aiming
- Full configuration of VR settings through menus
- Native support for the Oculus Rift
- Xbox 360 / Xinput compatible gamepad support
- Regular / 2D mode if VR is disabled, or no Rift is detected
- Multiplayer should work (deathmatch or CTF)

#### Suggested VR configurations (aim modes, etc.)
There's a lot of VR configuration options. Depending on how you want to play the game, here are my suggestions:

- Seated or Standing (one direction / 180 degrees) with Oculus Touch:   
AimMode = Decoupled View/Aiming; VR Controller Support = on. Comfort Turning on/off depending on user preference.
- Seated or Standing (one direction / 180 degrees) with Gamepad:   
AimMode = Decoupled View/Aiming; VR Controller Support = disabled. Comfort Turning on (since the gamepad stick will change your aim direction not your look direction, keeping Comfort Turning on lets you change your view direction via snap turns by pressing gamepad DPad Left / Right)
- Seated or Standing, 360 degrees, with Gamepad or Oculus Touch:   
AimMode = Decoupled View/Aiming; VR Controller Support = on. Comfort Turning on/off depending on user preference.
- Seated or Standing, 360 degrees, with Gamepad:  
AimMode = Decoupled View/Aiming; VR Controller Support = disabled. Comfort Turning on/off depending on user preference.
- Seated, with Mouse Aiming:  
AimMode = DeadZone w/ Aim Move, or DeadZone with View Move.

#### Known Issues
- Oculus SDK support only
- No positional weapon tracking w/ Oculus Touch, only orientation
- Positional tracking only affects your head / camera position. Your body doesn't move, which also means that positional tracking that doesn't affect where your weapon fires from.

#### Requirements
- Full version of [Quake II](http://store.steampowered.com/app/2320/) or the demo version below
- Oculus Rift
- Windows (tested with 8.1 and 10 - it may work with older versions too)
- [32bit MSVC 2012 runtime](http://dgho.st/aXN5)

#### Downloads
Note: A clean install is suggested, I haven't tested installing this package over an existing Quake 2 VR installation.

- Playable Quake II demo package w/ VR support (66MB): Quake2VR-2.0.0-shareware.zip  
- Quake II VR Binaries package (34MB): Quake2VR-2.0.0-bin.zip  
- Quake II VR Binaries w/ HD Textures, Soundtrack + Multiplayer Mods (561MB): Quake2VR-2.0.0-full.zip  
All 3 packages are available here: [Link 1 (Mega)](https://mega.nz/#F!U8dRCYCA!msfdpEQfpj8Qec95gQ2sHA)  
Also, the binaries-only package is available from the GitHub releases page: [Q2VR Releases on GitHub](https://github.com/q2vr/quake2vr/releases/)

#### Instructions

##### Shareware Version
1. Download the shareware package and unzip it to your preferred directory.
2. Optionally download the high resolution texture package and unzip it to the same folder.
3. Run `quake2vr`.

##### Full Version Instructions
1. Download binary package and unzip it to your preferred directory.
2. Optionally download the extra's package and unzip it to the same folder.
3. Copy the following files from your Quake II `baseq2` folder into the new `baseq2` folder:
    - `pak0.pak` (overwrite it if you downloaded the shareware version)
    - `players\` (optional - only if you want to play multiplayer. overwrite it if you downloaded the shareware version)
    - `videos\` (optional - only if you want cinematics)
4. If you have the CD audio soundtrack in .ogg format you can optionally place these files in `baseq2\music\`. Music files need to be named `track02.ogg` to `track11.ogg`.
5. Run `quake2vr` or choose one of the links provided in the extras pack. 

You can alternately just copy the contents of the extracted `Quake2VR` directory on top of an existing Quake II installation, however there is always the risk that this breaks the original installation.

If your Oculus Rift is connected and powered on it will enable support automatically at launch, but you can also enable Rift support by accessing the console using ~ and entering `vr_enable`. When it initializes it will attempt to locate the display that the Rift is configured as and use it, but if that fails it will default to the primary monitor.

##### Aim mode warnings
Aim modes that allow the mouse to adjust the view pitch are currently broken in multiplayer. These are denoted with an \* in the VR settings menu, and are aim modes 2 and 4 if you set them through the cvar's.

##### Modifications and Add-On's
As this is based on KMQuake II, it supports any modification or add-on that is compatible with KMQuake II. The extra's package includes both high resolution model and world textures, and and I highly recommend turning on some of the advanced video options.

#### More Info
- [Project Homepage](https://github.com/q2vr/Quake2VR/)
- [Latest Releases](https://github.com/q2vr/Quake2VR/releases)
- [Release Notes](https://github.com/q2vr/Quake2VR/wiki/Changelog)
- [FAQs](https://github.com/q2vr/Quake2VR/wiki/FAQs)
- [Bug Tracker](https://github.com/q2vr/Quake2VR/issues)

#### Acknowledgements

Quake II VR uses the following third-party libraries:

- [GLEW](http://glew.sourceforge.net)
- [SDL2](https://www.libsdl.org/index.php)
- [minizip](http://www.winimage.com/zLibDll/minizip.html)
- [stb](https://github.com/nothings/stb)

Quake II VR uses modified versions of the following third-party libraries:

- Peter Scott's [C Port](https://github.com/PeterScott/murmur3) of [Murmur3](https://code.google.com/p/smhasher/wiki/MurmurHash3), with fixes for MSVC/C89 compatibility
- nf_string_table from [nflibs](https://github.com/niklasfrykholm/nflibs), with fixes for MSVC/C89 compatibility


Thanks go out to:

- [id Software](http://www.idsoftware.com), for releasing such an awesome game 16 years ago and then being generous enough to release the source code to the community.
- [OculusVR](https://www.oculus.com), for bring VR back and making this project possible.
- [Knightmare](http://www.markshan.com/knightmare/) for KMQuake II. The general cleanliness of the code base and significant graphical upgrades responsible for this turning out as well as it did.
- [Dominic Szablewski](https://github.com/phoboslab) and [John Marshall](https://github.com/swax) for their work on RiftQuake. It served as the basis for this project, and in some places their code remains.
- Jim Waurzyniak for taking the time and effort to host a collection of high-resolution assets for Quake II.
- [Jared Stafford](https://jspenguin.org) for getting Linux support up and running
- [Daniel Wolf](https://github.com/Nephatrine) for various bugfixes and code cleanup.

