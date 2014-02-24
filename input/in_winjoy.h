
// x_winjoy.h -- Support for Windows joysticks

void IN_WinJoyInit (void);


void IN_WinJoyCommands (void);
// opportunity for devices to stick commands on the script buffer

void IN_WinJoyMove (usercmd_t *cmd);
// add additional movement on top of the keyboard move cmd
