#ifndef _OSMAND_SKIA_WP8_THREADING_STUBS_H_
#define _OSMAND_SKIA_WP8_THREADING_STUBS_H_

BOOL __forceinline TerminateThread(HANDLE /*hThread*/, DWORD /*dwExitCode*/)
{
    return TRUE;
}

BOOL __forceinline GetProcessAffinityMask(HANDLE /*hProcess*/, PDWORD_PTR lpProcessAffinityMask, PDWORD_PTR lpSystemAffinityMask)
{
    if(lpProcessAffinityMask)
        *lpProcessAffinityMask = 0x1;
    if(lpSystemAffinityMask)
        *lpSystemAffinityMask = 0x1;
    return TRUE;
}

DWORD_PTR __forceinline SetThreadAffinityMask(HANDLE /*hThread*/, DWORD_PTR /*dwThreadAffinityMask*/)
{
    return 0x1;
}

#endif // _OSMAND_SKIA_WP8_THREADING_STUBS_H_