#include <windows.h>

#define __UNKNOWN_APP    0 // abused for DLL
#define __CONSOLE_APP    1
#define __GUI_APP        2

#ifndef _APPTYPE
#define _APPTYPE __CONSOLE_APP
#endif

typedef void(*_PVFV)();

// C init
extern _PVFV __xi_a[];
extern _PVFV __xi_z[];
// C++ init
extern _PVFV __xc_a[];
extern _PVFV __xc_z[];
// C pre-terminators
extern _PVFV __xp_a[];
extern _PVFV __xp_z[];
// C terminators
extern _PVFV __xt_a[];
extern _PVFV __xt_z[];

extern void _setargv (void);
extern void term_atexit();

extern IMAGE_DOS_HEADER __ImageBase; // linker generated

extern int __ref_oldnames;

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "oldnames.lib")
#if MSVCRT_VERSION >= 140
#pragma comment(lib, "ucrtbase.lib")
#endif

#if _APPTYPE == __UNKNOWN_APP

extern BOOL WINAPI DllMain (HINSTANCE, DWORD, LPVOID);

BOOL WINAPI
_DllMainCRTStartup (HINSTANCE hDll, DWORD dwReason, LPVOID lpReserved)
{
    BOOL bRet;
    __ref_oldnames = 0; // drag in alternate definitions

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _initterm_e(__xi_a, __xi_z);
        _initterm(__xc_a, __xc_z);
    }

    bRet = DllMain (hDll, dwReason, lpReserved);

    if (dwReason == DLL_PROCESS_DETACH || dwReason == DLL_PROCESS_ATTACH && !bRet)
    {
        term_atexit();
        _initterm(__xp_a, __xp_z);
        _initterm(__xt_a, __xt_z);
    }
    return bRet;
}

BOOL WINAPI __DefaultDllMain (HINSTANCE hDll, DWORD dwReason, LPVOID lpReserved)
{
    return TRUE;
}

#ifdef _M_X64
__pragma(comment(linker, "/alternatename:DllMain=__DefaultDllMain"));
#else
__pragma(comment(linker, "/alternatename:_DllMain@12=___DefaultDllMain@12"));
#endif

#else // _APPTYPE != __UNKNOWN_APP

extern int    __argc;

#ifndef _UNICODE
extern char **__argv;
extern int main(int, char **, char **);
#else
extern wchar_t **__wargv;
extern int wmain(int, wchar_t **, wchar_t **);
#endif

#if MSVCRT_VERSION >= 140 // UCRT

#ifdef _M_X64
__pragma(comment(linker, "/alternatename:__set_app_type=_set_app_type"));
#else
__pragma(comment(linker, "/alternatename:___set_app_type=__set_app_type"));
#endif

enum _crt_argv_mode
{
    _crt_argv_no_arguments,
    _crt_argv_unexpanded_arguments,
    _crt_argv_expanded_arguments,
};

#ifndef _UNICODE
extern int _initialize_narrow_environment();
extern char **_get_initial_narrow_environment();
extern int _configure_narrow_argv(int);
extern char *_get_narrow_winmain_command_line();
#else // _UNICODE
extern int _initialize_wide_environment();
extern wchar_t **_get_initial_wide_environment();
extern int _configure_wide_argv(int);
extern wchar_t *_get_wide_winmain_command_line();
#endif

#else // MSVCRT_VERSION < 140

/* In MSVCRT.DLL, Microsoft's initialization hook is called __getmainargs(),
 * and it expects a further structure argument, (which we don't use, but pass
 * it as a dummy, with a declared size of zero in its first and only field).
 */
typedef struct _startupinfo { int mode; } _startupinfo;
#ifndef _UNICODE
extern int __getmainargs(int *argc, char ***argv, char ***penv, int glob, _startupinfo *info);
#else
extern int __wgetmainargs(int *argc, wchar_t ***argv, wchar_t ***penv, int glob, _startupinfo *info);
#endif

#endif // MSVCRT_VERSION < 140

/* The function mainCRTStartup() is the entry point for all
 * console/desktop programs.
 */
#if _APPTYPE == __CONSOLE_APP
  #ifndef _UNICODE
void mainCRTStartup(void)
  #else
void wmainCRTStartup(void)
  #endif
#else // desktop app
  #ifndef _UNICODE
void WinMainCRTStartup(void)
  #else
void wWinMainCRTStartup(void)
  #endif
#endif
{
    int nRet;
    __set_app_type(_APPTYPE);
    __ref_oldnames = 0; // drag in alternate definitions

#if MSVCRT_VERSION >= 140 // UCRT
  #ifndef _UNICODE
    _configure_narrow_argv(_crt_argv_unexpanded_arguments);
    _initialize_narrow_environment();
    char **envp = _get_initial_narrow_environment();
  #else
    _configure_wide_argv(_crt_argv_unexpanded_arguments);
    _initialize_wide_environment();
    wchar_t **wenvp = _get_initial_wide_environment();
  #endif
#else // MSVCRT_VERSION < 140
    /* The MSVCRT.DLL start-up hook requires this invocation
     * protocol...
     */
    _startupinfo start_info = { 0 };
  #ifndef _UNICODE
    char **envp = NULL;
    __getmainargs(&__argc, &__argv, &envp, 0, &start_info);
  #else
    wchar_t **wenvp = NULL;
    __wgetmainargs(&__argc, &__wargv, &wenvp, 0, &start_info);
  #endif
#endif // MSVCRT_VERSION < 140

    _initterm_e(__xi_a, __xi_z);
    _initterm(__xc_a, __xc_z);

#if _APPTYPE == __CONSOLE_APP
  #ifndef _UNICODE
    nRet = main(__argc, __argv, envp);
  #else
    nRet = wmain(__argc, __wargv, wenvp);
  #endif
#else // desktop app
    {
        STARTUPINFOA startupInfo;
        GetStartupInfoA(&startupInfo);
        int showWindowMode = startupInfo.dwFlags & STARTF_USESHOWWINDOW
                           ? startupInfo.wShowWindow : SW_SHOWDEFAULT;
  #ifndef _UNICODE
    #if MSVCRT_VERSION >= 140 // UCRT
        LPSTR lpszCommandLine = _get_narrow_winmain_command_line();
    #else
        LPSTR lpszCommandLine = GetCommandLineA();
    #endif
        nRet = WinMain((HINSTANCE)&__ImageBase, NULL, lpszCommandLine, showWindowMode);
  #else // _UNICODE
    #if MSVCRT_VERSION >= 140 // UCRT
        LPWSTR lpszCommandLine = _get_wide_winmain_command_line();
    #else
        LPWSTR lpszCommandLine = GetCommandLineW();
    #endif
        nRet = wWinMain((HINSTANCE)&__ImageBase, NULL, lpszCommandLine, showWindowMode);
  #endif
    }
#endif // desktop app

    term_atexit();
    _initterm(__xp_a, __xp_z);
    _initterm(__xt_a, __xt_z);

    ExitProcess(nRet);
}

#endif // _APPTYPE != __UNKNOWN_APP
