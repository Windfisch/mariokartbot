//#define FREEBSD
#define LINUX

#ifdef FREEBSD
#define JOYSTICK_PATCHEDINPUTPLUGIN
#else
#define JOYSTICK_UINPUT
#endif
