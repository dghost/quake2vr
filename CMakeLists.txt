cmake_minimum_required (VERSION 2.8)

IF(DEFINED CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of
build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug
Release RelWithDebInfo MinSizeRel.")
ELSE()
   SET(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build,
options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release
RelWithDebInfo MinSizeRel.")
ENDIF()

project (quake2vr)
set (quake2vr_VERSION_MAJOR 1)
set (quake2vr_VERSION_MINOR 0)

set (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/external/")


include (CheckLibraryExists)
find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(ZLIB REQUIRED)

if (MINGW)
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif (MINGW)

include_directories (${SDL2_INCLUDE_DIR} ${GLEW_INCLUDE_DIR})

#check_library_exists (SDL2_image IMG_Load "" HAVE_SDL2_IMAGE)

#if (NOT HAVE_SDL2_IMAGE)
#  message (SEND_ERROR "Could not find SDL2_image")
#endif (NOT HAVE_SDL2_IMAGE)
option(OVR_DYNAMIC                "Dynamically load the Oculus library" ON)

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
  qcommon/zip/ioapi.c
  qcommon/zip/unzip.c
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

set(BASE_HEADERS
	backends/sdl2/in_sdlcont.h
	backends/sdl2/sdl2quake.h
	client/adivtab.h
	client/anorms.h
	client/asm_i386.h
	client/block16.h
	client/block8.h
	client/cinematic.h
	client/client.h
	client/console.h
	client/include/altypes.h
	client/include/codec.h
	client/include/ogg.h
	client/include/os_types.h
	client/include/vorbisfile.h
	client/input.h
	client/keys.h
	client/particles.h
	client/q2palette.h
	client/ref.h
	client/renderer/include/anorms.h
	client/renderer/include/anormtab.h
	client/renderer/include/qgl.h
	client/renderer/include/r_alias.h
	client/renderer/include/r_cin.h
	client/renderer/include/r_local.h
	client/renderer/include/r_model.h
	client/renderer/include/r_normals.h
	client/renderer/include/r_stereo.h
	client/renderer/include/r_vr.h
	client/renderer/include/r_vr_ovr.h
	client/renderer/include/r_vr_svr.h
	client/renderer/include/vlights.h
	client/renderer/include/warpsin.h
	client/screen.h
	client/sound/include/AL/al.h
	client/sound/include/AL/alc.h
	client/sound/include/AL/alconflict.h
	client/sound/include/AL/alext.h
	client/sound/include/AL/alunconflict.h
	client/sound/include/AL/efx-creative.h
	client/sound/include/AL/efx-presets.h
	client/sound/include/AL/efx.h
	client/sound/include/alConfigMob.h
	client/sound/include/alConfigMobDefs_inl.h
	client/sound/include/local.h
	client/sound/include/qal.h
	client/sound/include/sound.h
	client/sound/include/vorbis.h
	client/ui/include/ui_local.h
	client/vid.h
	client/vr/include/vr.h
	client/vr/include/vr_ovr.h
	client/vr/include/vr_steamvr.h
	client/vr/include/vr_svr.h
	client/vr/oculus_dynamic.h
	client/vr/ovr_dynamic_funcs.h
	qcommon/crc.h
	qcommon/glob.h
        qcommon/zip/ioapi.h
	qcommon/zip/unzip.h
	qcommon/zip/zconf.h
	qcommon/zip/zlib.h
	qcommon/qcommon.h
	qcommon/qfiles.h
	qcommon/shared/q_shared.h
	qcommon/vid_modes.h
	qcommon/wildcard.h
	server/server.h
)

set(UNIX_SOURCES
  backends/unix/net_udp.c
  backends/unix/qsh_unix.c
)


set(UNIX_HEADERS
	backends/unix/glw_unix.h
)


if (OVR_DYNAMIC)
  add_definitions(-DOCULUS_DYNAMIC)
  set(BASE_SOURCES
	${BASE_SOURCES}
	client/vr/oculus_dynamic.c
	)
endif (OVR_DYNAMIC)

if (UNIX)
add_definitions(-DNO_STEAM -DUSE_OPENAL)
add_executable(quake2vr ${BASE_SOURCES} ${UNIX_SOURCES} ${BASE_HEADERS} ${UNIX_HEADERS})
endif (UNIX)

target_link_libraries(quake2vr ${SDL2_LIBRARY} ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${GLEW_LIBRARIES} ${ZLIB_LIBRARIES})
target_compile_options(quake2vr PRIVATE -fvisibility=hidden)

if (!OVR_DYNAMIC)
target_link_libraries(quake2vr -L${quake2vr_SOURCE_DIR} ovr)
endif (!OVR_DYNAMIC)

add_subdirectory(game)
add_subdirectory(game_mp)

add_custom_target(data ALL
  COMMAND ${CMAKE_SOURCE_DIR}/misc/mkpk3.sh "${CMAKE_BINARY_DIR}"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/misc/
)
