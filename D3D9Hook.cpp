#include "D3D9Hook.h"
#include "plugin.h"
#include "imgui_impl_dx9.h"
#include "Menu.h"
#include "AnimationFist.h"

extern AnimationFist* g_pFist;

D3D9Hook::stHookData D3D9Hook::m_hkPresent;
D3D9Hook::stHookData D3D9Hook::m_hkResetDevice;

void D3D9Hook::initHooks() {
    PVOID hookPrs = reinterpret_cast<void*>(getDeviceAddress(17));
    PVOID hookRes = reinterpret_cast<void*>(getDeviceAddress(16));

    m_hkPresent.m_pDetour = new PLH::x86Detour(UINT64(hookPrs), UINT64(&HookedPresent), &m_hkPresent.m_uiJump);
    m_hkResetDevice.m_pDetour = new PLH::x86Detour(UINT64(hookRes), UINT64(&HookedReset), &m_hkResetDevice.m_uiJump);

    m_hkPresent.m_pDetour->hook();
    m_hkResetDevice.m_pDetour->hook();
}

void D3D9Hook::destroyHooks() {
    if (m_hkPresent.m_pDetour) {
        m_hkPresent.m_pDetour->unHook();
        delete m_hkPresent.m_pDetour;
    }
    if (m_hkResetDevice.m_pDetour) {
        m_hkResetDevice.m_pDetour->unHook();
        delete m_hkResetDevice.m_pDetour;
    }
}

NOINLINE HRESULT __stdcall D3D9Hook::HookedPresent(IDirect3DDevice9* pDevice, const RECT* pSrcRect, const RECT* pDestRect, HWND hDestWindow, const RGNDATA* pDirtyRegion) {
    if (pDevice) {
        Menu::render(); 
    }
    return ((HRESULT(__stdcall*)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*))D3D9Hook::m_hkPresent.m_uiJump)(pDevice, pSrcRect, pDestRect, hDestWindow, pDirtyRegion);
}

NOINLINE HRESULT __stdcall D3D9Hook::HookedReset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentParams) {
    if (!pDevice)
        return ((HRESULT(__stdcall*)(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS * pPresentParams)) D3D9Hook::m_hkResetDevice.m_uiJump)(pDevice, pPresentParams);

    static bool objectCreated = false;
    if (objectCreated) {
        ImGui_ImplDX9_InvalidateDeviceObjects();
        g_pFist->Invalidate();
        objectCreated = false;
    }

    HRESULT ret =
        ((HRESULT(__stdcall*)(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS * pPresentParams)) D3D9Hook::m_hkResetDevice.m_uiJump)(pDevice, pPresentParams);

    if (ret == D3D_OK) {
        ImGui_ImplDX9_CreateDeviceObjects();
        g_pFist->Initialize();
        objectCreated = true;
    }

    return ret;
}

DWORD D3D9Hook::findDevice(DWORD len) {
    DWORD dwObjBase = NULL;
    char  infoBuf[MAX_PATH]{};
    GetSystemDirectoryA(infoBuf, MAX_PATH);
    strcat_s(infoBuf, MAX_PATH, "\\d3d9.dll");
    dwObjBase = reinterpret_cast<DWORD>(LoadLibraryA(infoBuf));
    while (dwObjBase++ < dwObjBase + len) {
        if ((*reinterpret_cast<WORD*>(dwObjBase + 0x00)) == 0x06C7 &&
            (*reinterpret_cast<WORD*>(dwObjBase + 0x06)) == 0x8689 &&
            (*reinterpret_cast<WORD*>(dwObjBase + 0x0C)) == 0x8689) {
            dwObjBase += 2;
            break;
        }
    }
    return dwObjBase;
}
DWORD D3D9Hook::getDeviceAddress(int VTableIndex) {
    PDWORD VTable;
    *reinterpret_cast<DWORD*>(&VTable) = *reinterpret_cast<DWORD*>(findDevice(0x128000));
    return VTable[VTableIndex];
};