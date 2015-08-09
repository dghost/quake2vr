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

option(OVR_DYNAMIC					"Dynamically load the Oculus library" OFF)
option(OPENAL						"Build with OpenAL support" ON)
option(STEAMVR						"Build with SteamVR support" ON)

include (CheckLibraryExists)
find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(ZLIB REQUIRED)


if (NOT OVR_DYNAMIC)
  # if OVRDIR is undefined, set it to the in-source external directory
  # otherwise, it will respect the user preference and look in that directory
  if (NOT DEFINED ENV{OVRDIR})
    set (ENV{OVRDIR} ${CMAKE_CURRENT_SOURCE_DIR}/external/OculusSDK/LibOVR/)
  endif (NOT DEFINED ENV{OVRDIR})
  find_package(OVR)
endif (NOT OVR_DYNAMIC)

if (STEAMVR)
	if (NOT DEFINED ENV{SteamworksDIR})
	  set (ENV{SteamworksDIR} ${CMAKE_CURRENT_SOURCE_DIR}/external/steam/)
	endif (NOT DEFINED ENV{SteamworksDIR})
	find_package(Steamworks)
endif (STEAMVR)

if (MINGW)
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif (MINGW)

include_directories (${SDL2_INCLUDE_DIR} ${GLEW_INCLUDE_DIR})

set(COMMON_HEADERS
	qcommon/crc.h
	qcommon/glob.h
	qcommon/qcommon.h
	qcommon/qfiles.h
	qcommon/shared/game.h
	qcommon/shared/q_shared.h
	qcommon/vid_modes.h
	qcommon/wildcard.h
	qcommon/zip/unzip.h
	qcommon/zip/zconf.h
	qcommon/zip/zlib.h
	qcommon/zip/ioapi.h
)

set(COMMON_SOURCES 
  qcommon/cmd.c
  qcommon/cmodel.c
  qcommon/common.c
  qcommon/crc.c
  qcommon/cvar.c
  qcommon/files.c
  qcommon/glob.c
  qcommon/md4.c
  qcommon/net_chan.c
  qcommon/pmove.c
  qcommon/shared/m_flash.c
  qcommon/shared/q_shared.c
  qcommon/stable.c
  qcommon/sset.c
  qcommon/wildcard.c
  qcommon/murmur3/murmur3.c
  qcommon/zip/ioapi.c
  qcommon/zip/unzip.c
)

set(CLIENT_UI_SOURCES
  client/ui/ui_backend.c
  client/ui/ui_game_credits.c
  client/ui/ui_game_saveload.c
  client/ui/ui_game.c
  client/ui/ui_main.c
  client/ui/ui_mp_addressbook.c
  client/ui/ui_mp_dmoptions.c
  client/ui/ui_mp_download.c
  client/ui/ui_mp_joinserver.c
  client/ui/ui_mp_playersetup.c
  client/ui/ui_mp_startserver.c
  client/ui/ui_multiplayer.c
  client/ui/ui_options_controls.c
  client/ui/ui_options_effects.c
  client/ui/ui_options_interface.c
  client/ui/ui_options_keys.c
  client/ui/ui_options_screen.c
  client/ui/ui_options_sound.c
  client/ui/ui_options_vr_advanced.c
  client/ui/ui_options_vr_ovr.c
  client/ui/ui_options_vr_svr.c
  client/ui/ui_options_vr.c
  client/ui/ui_options.c
  client/ui/ui_quit.c
  client/ui/ui_subsystem.c
  client/ui/ui_video_advanced.c
  client/ui/ui_video.c
)

set(CLIENT_SOUND_SOURCES
  client/sound/ogg.c
  client/sound/openal.c
  client/sound/qal.c
  client/sound/sound.c
  client/sound/wave.c
)

set(CLIENT_RENDERER_SOURCES
  client/renderer/r_alias_md2.c
  client/renderer/r_alias_misc.c
  client/renderer/r_alias.c
  client/renderer/r_antialias.c
  client/renderer/r_backend.c
  client/renderer/r_beam.c
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
  client/renderer/r_postprocess.c
  client/renderer/r_shaderobjects.c
  client/renderer/r_sky.c
  client/renderer/r_sprite.c
  client/renderer/r_stereo.c
  client/renderer/r_surface.c
  client/renderer/r_vao.c
  client/renderer/r_vbo.c
  client/renderer/r_vlights.c
  client/renderer/r_vr_ovr.c
  client/renderer/r_vr_svr.c
  client/renderer/r_vr.c
  client/renderer/r_warp.c
)

set(CLIENT_VR_SOURCES
  client/vr/vr_main.c
  client/vr/vr_ovr.c
  client/vr/vr_steamvr.cpp
  client/vr/vr_svr.c
)

set(CLIENT_BASE_SOURCES
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
) 

set(CLIENT_HEADERS
	client/adivtab.h
	client/anorms.h
	client/cinematic.h
	client/client.h
	client/console.h
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
	client/renderer/include/r_vr_ovr.h
	client/renderer/include/r_vr_svr.h
	client/renderer/include/r_vr.h
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
	client/vr/include/vr_ovr.h
	client/vr/include/vr_steamvr.h
	client/vr/include/vr_svr.h
	client/vr/include/vr.h
)

if (NOT OVR_FOUND)
  add_definitions(-DOCULUS_DYNAMIC)
  set(CLIENT_VR_SOURCES
	${CLIENT_VR_SOURCES}
	client/vr/oculus_dynamic/oculus_dynamic.c
	)
  set(CLIENT_HEADERS
    ${CLIENT_HEADERS}
	client/vr/oculus_dynamic/oculus_dynamic.h
	client/vr/oculus_dynamic/ovr_dynamic_funcs.h
  )  
endif (NOT OVR_FOUND)

set(CLIENT_SOURCES
  ${CLIENT_BASE_SOURCES}
  ${CLIENT_RENDERER_SOURCES}
  ${CLIENT_SOUND_SOURCES}
  ${CLIENT_UI_SOURCES}
  ${CLIENT_VR_SOURCES}
)

set(SERVER_SOURCES 
  server/sv_ccmds.c
  server/sv_ents.c
  server/sv_game.c
  server/sv_init.c
  server/sv_main.c
  server/sv_send.c
  server/sv_user.c
  server/sv_world.c
)
set(SERVER_HEADERS
  server/server.h
)

set(SDL_SOURCES
  backends/sdl2/gl_sdl2.c
  backends/sdl2/in_sdl2.c
  backends/sdl2/in_sdlcont.c
  backends/sdl2/snd_sdl2.c
  backends/sdl2/sys_sdl2.c
  backends/sdl2/vid_sdl2.c
)

set(SDL_HEADERS
  backends/sdl2/in_sdlcont.h
  backends/sdl2/sdl2quake.h
)

set(UNIX_SOURCES
  backends/unix/net_udp.c
  backends/unix/qsh_unix.c
)

set(UNIX_HEADERS
)

set(CLIENT_BASE
  ${COMMON_SOURCES}
  ${SERVER_SOURCES}	
  ${CLIENT_SOURCES}
  ${COMMON_HEADERS}
  ${SERVER_HEADERS}
  ${CLIENT_HEADERS}
)

if (OPENAL)
  add_definitions(-DUSE_OPENAL)
endif (OPENAL)

if (UNIX)
  set(BACKEND_SOURCES
    ${SDL_SOURCES}
    ${UNIX_SOURCES} 
  )
  set(BACKEND_HEADERS
    ${SDL_HEADERS} 
    ${UNIX_HEADERS}
  )
  set(BACKEND
   ${BACKEND_SOURCES}
   ${BACKEND_HEADERS}
  )
endif (UNIX)

source_group("Common" FILES ${COMMON_SOURCES})
source_group("Common\\Headers" FILES ${COMMON_HEADERS})
source_group("Client" FILES ${CLIENT_BASE_SOURCES})
source_group("Client\\Renderer" FILES ${CLIENT_RENDERER_SOURCES})
source_group("Client\\Sound" FILES ${CLIENT_SOUND_SOURCES})
source_group("Client\\UI" FILES ${CLIENT_UI_SOURCES})
source_group("Client\\VR" FILES ${CLIENT_VR_SOURCES})
source_group("Client\\Headers" FILES ${CLIENT_HEADERS})
source_group("Server" FILES ${SERVER_SOURCES})
source_group("Server\\Headers" FILES ${SERVER_HEADERS})
source_group("Backend" FILES ${BACKEND_SOURCES})
source_group("Backend\\Headers" FILES ${BACKEND_HEADERS})

add_executable(quake2vr ${CLIENT_BASE} ${BACKEND})

target_link_libraries(quake2vr ${SDL2_LIBRARY} ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${GLEW_LIBRARIES} ${ZLIB_LIBRARIES})

# check 64 bit
# this will break on non-x86 processors
if( CMAKE_SIZEOF_VOID_P EQUAL 4 )
	set( ARCH "x86" )
else()
	set( ARCH "x64" )
endif()

message ( STATUS "Building for ${ARCH}" )

if (OVR_FOUND)
  message ( STATUS "Building with Oculus Rift support..." )
  include_directories (${OVR_INCLUDE_DIR} ${OVR_CAPI_INCLUDE_DIR})  
  target_link_libraries(quake2vr ${OVR_LIBRARY})
endif (OVR_FOUND)

if (STEAMWORKS_FOUND)
  message ( STATUS "Building with SteamVR support..." )
  if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  	# ugly hack to work around the fact that SteamVR's headers check for GNUC
  	add_definitions(-DGNUC)
  endif ()
  include_directories (${STEAMWORKS_INCLUDE_DIR})
  target_link_libraries (quake2vr ${STEAMWORKS_LIBRARY})
else ()
  add_definitions(-DNO_STEAM)
endif (STEAMWORKS_FOUND)

target_compile_options(quake2vr PRIVATE -fvisibility=hidden)

set_target_properties(quake2vr PROPERTIES XCODE_ATTRIBUTE_OTHER_CFLAGS[variant=Debug] "-D_DEBUG" )

add_subdirectory(game)
add_subdirectory(game_mp)

add_custom_target(data ALL
  COMMAND ${CMAKE_SOURCE_DIR}/misc/mkpk3.sh "${CMAKE_BINARY_DIR}"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/misc/
)
