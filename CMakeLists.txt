cmake_minimum_required (VERSION 2.8)
project (quake2vr)
set (quake2vr_VERSION_MAJOR 1)
set (quake2vr_VERSION_MINOR 0)

include (CheckLibraryExists)
include (FindOpenGL)

if (CMAKE_CROSSCOMPILING)
  FIND_PATH(SDL2_INCLUDE_DIRS SDL.h
    HINTS
    $ENV{SDL2DIR}
    PATH_SUFFIXES include/SDL2 include SDL2
    ONLY_CMAKE_FIND_ROOT_PATH
    )

  MESSAGE("SDL2_INCLUDE_DIRS: ${SDL2_INCLUDE_DIRS}")

  FIND_LIBRARY(SDL2_LIBRARY_TEMP
    NAMES SDL2
    HINTS
    $ENV{SDL2DIR}
    PATH_SUFFIXES lib64 lib
    ONLY_CMAKE_FIND_ROOT_PATH
    )

  FIND_LIBRARY(SDL2_MAIN_LIBRARY
    NAMES SDL2main
    HINTS
    $ENV{SDL2DIR}
    PATH_SUFFIXES lib64 lib
    ONLY_CMAKE_FIND_ROOT_PATH
    )


  set (SDL2_LIBRARIES ${SDL2_LIBRARY_TEMP} ${SDL2_MAIN_LIBRARY})
  if (MINGW)
    set (SDL2_LIBRARIES mingw32 ${SDL2_LIBRARIES})
  endif (MINGW)

  MESSAGE("SDL2_LIBRARIES: ${SDL2_LIBRARIES}")

else (CMAKE_CROSSCOMPILING)
  include (FindPkgConfig)
  pkg_search_module (SDL2 REQUIRED sdl2)
endif (CMAKE_CROSSCOMPILING)

if (MINGW)
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif (MINGW)

include_directories (${SDL2_INCLUDE_DIRS})

#check_library_exists (SDL2_image IMG_Load "" HAVE_SDL2_IMAGE)

#if (NOT HAVE_SDL2_IMAGE)
#  message (SEND_ERROR "Could not find SDL2_image")
#endif (NOT HAVE_SDL2_IMAGE)

set(BASE_SOURCES
  client/renderer/r_alias.c
  client/renderer/r_alias_md2.c
  client/renderer/r_alias_misc.c
  client/renderer/r_antialias.c
  client/renderer/r_backend.c
  client/renderer/r_beam.c
  client/renderer/r_postprocess.c
  client/renderer/r_draw.c
  client/renderer/r_entity.c
  client/renderer/r_fbo.c
  client/renderer/r_fog.c
  client/renderer/r_fragment.c
  client/renderer/r_glstate.c
  client/renderer/r_image.c
  client/renderer/r_light.c
  client/renderer/r_main.c
  client/renderer/r_misc.c
  client/renderer/r_model.c
  client/renderer/r_particle.c
  client/renderer/r_stereo.c
  client/renderer/r_shaderobjects.c
  client/renderer/r_sky.c
  client/renderer/r_sprite.c
  client/renderer/r_surface.c
  client/renderer/r_vao.c
  client/renderer/r_vbo.c
  client/renderer/r_vlights.c
  client/renderer/r_vr.c
  client/renderer/r_vr_ovr.c
  client/renderer/r_vr_svr.c
  client/renderer/r_warp.c
  client/sound/ogg.c
  client/sound/openal.c
  client/sound/qal.c
  client/sound/sound.c
  client/sound/wave.c
  client/ui/ui_backend.c
  client/ui/ui_game.c
  client/ui/ui_game_credits.c
  client/ui/ui_game_saveload.c
  client/ui/ui_main.c
  client/ui/ui_mp_addressbook.c
  client/ui/ui_mp_dmoptions.c
  client/ui/ui_mp_download.c
  client/ui/ui_mp_joinserver.c
  client/ui/ui_mp_playersetup.c
  client/ui/ui_mp_startserver.c
  client/ui/ui_multiplayer.c
  client/ui/ui_options.c
  client/ui/ui_options_controls.c
  client/ui/ui_options_effects.c
  client/ui/ui_options_interface.c
  client/ui/ui_options_keys.c
  client/ui/ui_options_screen.c
  client/ui/ui_options_sound.c
  client/ui/ui_options_vr.c
  client/ui/ui_options_vr_advanced.c
  client/ui/ui_options_vr_ovr.c
  client/ui/ui_options_vr_svr.c
  client/ui/ui_quit.c
  client/ui/ui_subsystem.c
  client/ui/ui_video.c
  client/ui/ui_video_advanced.c
  client/vr/vr_main.c
  client/vr/vr_ovr.c
  client/vr/vr_steamvr.cpp
  client/vr/vr_svr.c
  qcommon/glob.c
  backends/sdl2/gl_sdl2.c
  backends/sdl2/in_sdl2.c
  backends/sdl2/in_sdlcont.c
  backends/sdl2/snd_sdl2.c
  backends/sdl2/sys_sdl2.c
  backends/sdl2/vid_sdl2.c
  client/cl_cin.c
  client/cl_cinematic.c
  client/cl_console.c
  client/cl_download.c
  client/cl_effects.c
  client/cl_ents.c
  client/cl_event.c
  client/cl_input.c
  client/cl_inv.c
  client/cl_keys.c
  client/cl_lights.c
  client/cl_main.c
  client/cl_parse.c
  client/cl_particle.c
  client/cl_pred.c
  client/cl_screen.c
  client/cl_string.c
  client/cl_tempent.c
  client/cl_utils.c
  client/cl_view.c
  client/x86.c
  qcommon/cmd.c
  qcommon/cmodel.c
  qcommon/common.c
  qcommon/crc.c
  qcommon/cvar.c
  qcommon/files.c
  qcommon/md4.c
  qcommon/net_chan.c
  qcommon/pmove.c
  qcommon/shared/m_flash.c
  qcommon/shared/q_shared.c
  qcommon/wildcard.c
  server/sv_ccmds.c
  server/sv_ents.c
  server/sv_game.c
  server/sv_init.c
  server/sv_main.c
  server/sv_send.c
  server/sv_user.c
  server/sv_world.c
)
set(LINUX_SOURCES
  backends/linux/net_udp.c
  backends/linux/q_shlinux.c
  backends/unix/zip/unzip.c
  backends/unix/zip/ioapi.c
)
add_definitions(-DNO_STEAM -DUSE_SYSTEM_JPEGLIB)
add_executable(quake2vr ${BASE_SOURCES} ${LINUX_SOURCES})
target_link_libraries(quake2vr ${SDL2_LIBRARIES} ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} -L${quake2vr_SOURCE_DIR} GLEW ogg vorbis vorbisfile jpeg ovr z)
