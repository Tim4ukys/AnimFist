#include "dllmain.h"
#include "AnimationFist.h"
#include "base64.h"

Config g_Config{std::move(base64_decode("QW5pbWF0aW9uRmlzdEJ5VGltNHVreXMuanNvbg=="))}; // AnimationFistByTim4ukys.json

HINSTANCE g_hDLL;
AnimationFist* g_pFist;

patch::callHook g_callHook{0x7F681A};
typedef void(__cdecl* _InitPresentHK)();
_InitPresentHK InitPresentHK = nullptr;
void           InitPresentHook() {
    D3D9Hook::initHooks();
    Menu::initMenu(g_hDLL, (LPDIRECT3DDEVICE9)RwD3D9GetCurrentD3DDevice());
    g_pFist = new AnimationFist((LPDIRECT3DDEVICE9)RwD3D9GetCurrentD3DDevice());

    return g_callHook.callOriginal<void(__cdecl)()>();
}
std::ofstream conout("CONOUT$", std::ios::out);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);

        std::cout.rdbuf(conout.rdbuf());

        // AnimationFistByTim4ukys.ASI
        if (hinstDLL != GetModuleHandleA(base64_decode("QW5pbWF0aW9uRmlzdEJ5VGltNHVreXMuQVNJ").c_str())) {
            TerminateProcess(GetCurrentProcess(), -1);
            return FALSE;
        }

        g_hDLL = hinstDLL;
        g_callHook.installHook(&InitPresentHook);
        //InitPresentHK = (_InitPresentHK)g_callHook.getOriginal();
        plugin::Events::shutdownRwEvent += []() {
            delete g_pFist;
            Menu::uninitMenu();
        };
        break;
    case DLL_PROCESS_DETACH:
        D3D9Hook::destroyHooks();
        break;
    }
    return TRUE;
}