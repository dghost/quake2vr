### Quake II VR

This is a Quake II engine mod to add full support for Virtual Reality HMD's such as the Oculus Rift.

This mod is based on [KMQuake II](http://www.markshan.com/knightmare/) and incorporates work from [RiftQuake](https://github.com/phoboslab/Quakespasm-Rift/).

####Features
- Projected HUD/2D UI elements
- Decoupled view and aiming
- Full configuration of VR settings through menus
- Native support for the Oculus Rift using libOVR 0.2.5
- Xbox 360 / Xinput compatible gamepad support

####Requirements
- Full version of [Quake II](http://store.steampowered.com/app/2320/) or the demo version below
- Windows XP or higher
- Oculus Rift
- Pretty much any video card that supports DirectX 9 or higher
- [32bit MSVC 2012 runtime](http://dgho.st/aXN5)

####Downloads
Note: I've made several changes to both names of and what files get bundled with the executable and in the extras package for this release. If you run into any issues while upgrading, I suggest a clean install and copying saved games.

######Unstable Packages w/ DK2 support (v1.9.3):

- Playable Quake II demo package w/ VR support (54MB): [Download 1](http://dgho.st/nSGn)
- Quake II VR Binaries package (35MB): [Download 1](http://dgho.st/s954)
- Quake II VR Binaries w/ HD Textures, Soundtrack + Multiplayer Mods (562MB): [Download 1](http://dgho.st/Wh3j)

######Stable Packages (v1.3.1):

- Playable shareware Quake II VR package (43MB): [Download 1](http://dgho.st/EIkT) [Download 2](https://docs.google.com/uc?export=download&id=0B3vzoY9q6d-wX2JpSl9CN1pRVkU)
- Quake II VR Binaries package (15MB): [Download 1](http://dgho.st/vYYS) [Download 2](https://docs.google.com/uc?export=download&id=0B3vzoY9q6d-wRC1LS2lSaU1CXzA)
- Optional extras - HD textures and mission pack support (448MB): [Download 1](http://dgho.st/at0i) [Download 2](https://docs.google.com/uc?export=download&id=0B3vzoY9q6d-wOTBYNUpUTXEyRk0)

####Instructions

#####Shareware Version
1. Download the shareware package and unzip it to your preferred directory.
2. Optionally download the high resolution texture package and unzip it to the same folder.
3. Run `quake2vr`.

#####Full Version Instructions
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

#####Aim mode warnings
Aim modes that allow the mouse to adjust the view pitch are currently broken in multiplayer. These are denoted with an \* in the VR settings menu, and are aim modes 2 and 4 if you set them through the cvar's.

#####Modifications and Add-On's
As this is based on KMQuake II, it supports any modification or add-on that is compatible with KMQuake II. The extra's package includes both high resolution model and world textures, and and I highly recommend turning on some of the advanced video options. That said, if you want a relatively pure experience the defaults settings will come pretty close to it.

####More Info
- [Project Homepage](https://github.com/q2vr/Quake2VR/)
- [Latest Releases](https://github.com/q2vr/Quake2VR/releases)
- [Release Notes](https://github.com/q2vr/Quake2VR/wiki/Changelog)
- [FAQs](https://github.com/q2vr/Quake2VR/wiki/FAQs)
- [Bug Tracker](https://github.com/q2vr/Quake2VR/issues)

####Acknowledgements

Thanks go out to:

- id Software, for releasing such an awesome game 16 years ago and then being generous enough to release the source code to the community.
- Knightmare and [KMQuake II](http://www.markshan.com/knightmare/). The general cleanliness of the code base and significant graphical upgrades responsible for this turning out as well as it did.
- [Dominic Szablewski](https://github.com/phoboslab) and [John Marshall](https://github.com/swax) for their work on RiftQuake. It served as the basis for this project, and in some places their code remains.
- Jim Waurzyniak for taking the time and effort to host a collection of high-resolution assets for Quake II.
- [Isla Schanuel](http://islaes.com/) for the idea of applying counter-rotation to the HUD to help reduce nausea.
- Karl B. for reporting bugs in multiplayer.
- [Nick Patrick](http://enpat.net) for a truly amazing job finding bugs in pre-release builds.
- [ubtri](https://www.youtube.com/user/ubtri) for a ton of ideas on how to improve it and for the multiplayer configuration.
- D at [eVRydayVR](https://www.youtube.com/channel/UCTlgIRnl3viQb1zCJqFJwqw) for doing the most amazing video walkthrough/review I've ever seen. Seriously, [go watch it](https://www.youtube.com/watch?v=ehoEJqZVFB0&list=UUTlgIRnl3viQb1zCJqFJwqw).
