//Hookapi.hнд╪Ч
 
#include <windows.h>
 
#ifndef HOOKAPI_H
#define HOOKAPI_H
 
int HookAPIFunction(HMODULE hFromModule,
                     PSTR pszFunctionModule,
                     PSTR pszFunctionName,
                     PROC pfnNewProc);
 
#endif
