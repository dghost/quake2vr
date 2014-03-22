
// in_sdlcont.h -- Support for SDL2 gamepad controllers

void IN_ControllerInit (void);


void IN_ControllerCommands (void);
// opportunity for devices to stick commands on the script buffer

void IN_ControllerMove (usercmd_t *cmd);
// add additional movement on top of the keyboard move cmd
