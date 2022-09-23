at

https://github.com/FreeRDP/FreeRDP/blob/master/client/Windows/cli/wfreerdp.c#L73

replace

args = CommandLineToArgvW(cmd, &argc);

to

args = hook_CommandLineToArgvW(cmd, &argc);