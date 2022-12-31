#pragma once

#include <polyhook2/Detour/x86Detour.hpp>
#include <d3d9.h>
#include <Windows.h>

class D3D9Hook {
public:

    static void initHooks();
    static void destroyHooks();

private:

    static NOINLINE HRESULT __stdcall HookedPresent(IDirect3DDevice9* pDevice, CONST RECT* pSrcRect, CONST RECT* pDestRect, HWND hDestWindow, CONST RGNDATA* pDirtyRegion);
    static NOINLINE HRESULT __stdcall HookedReset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentParams);

    struct stHookData {
        uint64_t m_uiJump{};
        PLH::x86Detour* m_pDetour{};
    };
    static stHookData m_hkPresent;
    static stHookData  m_hkResetDevice;

    static DWORD findDevice(DWORD len);
    static DWORD getDeviceAddress(int VTableIndex);
};

