  int argc;
  if (PWSTR* argv = hook_CommandLineToArgvW(GetCommandLineW(), &argc))
  {
    if (argc)
    {
      argv += argc;
      do 
      {
        DbgPrint("[%x]: \"%S\"\n", argc, *--argv);
      } while (--argc);
    }
    LocalFree(argv);
  }
