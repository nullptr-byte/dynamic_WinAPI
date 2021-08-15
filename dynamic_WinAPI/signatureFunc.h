#pragma once
#include <Windows.h>

using _Sleep = VOID(*)(
    _In_ DWORD dwMilliseconds
);

using _GetTickCount = DWORD(*)(
    VOID
);