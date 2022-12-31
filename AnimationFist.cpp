#include "AnimationFist.h"
#include <Windows.h>
#include <thread>
#include <CRGBA.h>
#include <RenderWare.h>
#include <CSprite2d.h>
#include "Config.h"
#include "patch.h"

extern Config g_Config;
extern AnimationFist* g_pFist;

patch::callHook g_chDrawFistIcon{0x58D988};

AnimationFist::AnimationFist(IDirect3DDevice9* pDevice)
    : m_pDevice(pDevice) {
    m_pDevice->AddRef();
    g_chDrawFistIcon.installHook(drawFistIcon);
    loadsDec();
}
AnimationFist::~AnimationFist() {
    unloadsDec();
    g_chDrawFistIcon.uninstallHook();
    m_pDevice->Release();
}

void AnimationFist::changeNameFist(const std::string& name) {
    if (g_pFist->m_stateLoad != LOAD_STATE::SUCCEEDED)
        return;

    g_Config["fileName"] = name;
    g_Config.saveFile();
    if (m_pCurrentDec && *m_pCurrentDec)
        (*m_pCurrentDec)->Invalidate();

    if (auto p = findPosDecoder(name);
        p != -1) {
        m_pCurrentDec = &m_decoders[p];
        m_bInitCurDec = true;
    } else {
        m_pCurrentDec = nullptr;
    }
}

void AnimationFist::Initialize() {
    if (m_pCurrentDec && *m_pCurrentDec)
        (*m_pCurrentDec)->Initialize(m_pDevice);
}
void AnimationFist::Invalidate() {
    if (m_pCurrentDec && *m_pCurrentDec)
        (*m_pCurrentDec)->Invalidate();
}

void __fastcall AnimationFist::drawFistIcon(CSprite2d* sprite, void* trash, CRect* rect, CRGBA* color) {
    if (g_pFist->m_stateLoad == LOAD_STATE::SUCCEEDED && (g_pFist->m_pCurrentDec && *g_pFist->m_pCurrentDec)) {
        if (g_pFist->m_bInitCurDec) {
            (*g_pFist->m_pCurrentDec)->Initialize(g_pFist->m_pDevice);
            g_pFist->m_bInitCurDec = false;
        }
        auto rgba = color->ToRwRGBA();
        ((int(__cdecl*)(CRect*, RwRGBA*, RwRGBA*, RwRGBA*, RwRGBA*))0x727420)(rect, &rgba, &rgba, &rgba, &rgba);

        *(IDirect3DTexture9**)(sprite->m_pTexture->raster + 1) = (*g_pFist->m_pCurrentDec)->ProcessPlay();
        RwEngineInstance->dOpenDevice.fpRenderStateSet(rwRENDERSTATETEXTURERASTER, sprite->m_pTexture->raster);

        RwIm2DRenderPrimitive(rwPRIMTYPETRIFAN, CSprite2d::maVertices, 4);
        RwEngineInstance->dOpenDevice.fpRenderStateSet(rwRENDERSTATETEXTURERASTER, 0);
    } else {
        g_chDrawFistIcon.callOriginal<decltype(drawFistIcon)>(sprite, trash, rect, color);
    }
}

void AnimationFist::threadLoadFiles(AnimationFist* pthis, size_t pos) {
    volatile auto* n = new WebPDecoder(("models\\AnimationFist\\" + pthis->m_fileNames[pos]).c_str(), pthis->m_pDevice);
    pthis->m_decoders[pos] = (WebPDecoder*)n;

    if (--(pthis->m_nRunThreadsCount) == 0) {
        pthis->m_stateLoad = LOAD_STATE::SUCCEEDED;
    }
}

void AnimationFist::loadsDec() {
    HANDLE dir;
    WIN32_FIND_DATAA file_data;
    if (m_stateLoad == LOAD_STATE::BUSY) return;
    if (m_decoders.size() > 0) return;

    m_stateLoad = LOAD_STATE::BUSY;

    // »щем файлы и сохран€ем файлы
    dir = FindFirstFileA("models\\AnimationFist\\*.webp", &file_data);
    if (dir == INVALID_HANDLE_VALUE) {
        m_stateLoad = LOAD_STATE::FAIL;
        return;
    }
    do {
        if (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;
        m_fileNames.emplace_back(file_data.cFileName);
    } while (FindNextFileA(dir, &file_data));
    FindClose(dir);

    auto sz = m_fileNames.size();
    if (sz == 0) {
        m_stateLoad = LOAD_STATE::EMPTY;
        return;
    }
    std::sort(m_fileNames.begin(), m_fileNames.end());

    m_decoders.resize(sz);
    m_nRunThreadsCount = sz;
    for (size_t i{}; i < sz; ++i) {
        std::thread thr(threadLoadFiles, this, i);
        thr.detach();
    }

    // »щем среди файлов тот самый
    if (auto p = findPosDecoder(g_Config["fileName"].get<std::string>());
        p != -1) {
        m_pCurrentDec = &m_decoders[p];
        m_bInitCurDec = true;
    } else {
        m_pCurrentDec = nullptr;
    }
}
void AnimationFist::unloadsDec() {
    m_fileNames.clear();
    for (auto& f : m_decoders) {
        f->Invalidate();
        delete f;
        f = nullptr;
    }
    m_decoders.clear();
}

size_t AnimationFist::findPosDecoder(const std::string& name) {
    size_t low{}, high{m_fileNames.size() - 1};
    while (low <= high) {
        size_t mid = (low + high) / 2;
        auto&  guess = m_fileNames[mid];
        if (guess > name) {
            high = mid - 1;
        } else if (guess < name) {
            low = mid + 1;
        } else {
            return mid;
        }
    }
    return -1;
}