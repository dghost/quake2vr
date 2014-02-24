
// in_xinput.h -- Support for Xinput controllers

void IN_XinputInit (void);


void IN_XinputCommands (void);
// opportunity for devices to stick commands on the script buffer

void IN_XinputMove (usercmd_t *cmd);
// add additional movement on top of the keyboard move cmd
