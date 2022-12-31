#pragma once

#include <string>
#include <vector>
#include <atomic>
#include "WebPDecoder.h"

class AnimationFist {
public:
    enum class LOAD_STATE {
        FAIL = -1,

        SUCCEEDED = 0,
        EMPTY,
        BUSY,
    };

    struct stWebPFileData {
        std::string fileName;
        WebPDecoder* pDecoder;
    };

    inline auto getCountFiles() const noexcept {
        return m_fileNames.size();
    }
    inline void getFileData(stWebPFileData& fileData, size_t pos) {
        fileData.fileName = m_fileNames[pos];
        fileData.pDecoder = m_decoders[pos];
    }
    inline auto getState() const noexcept {
        return m_stateLoad;
    }

    AnimationFist(IDirect3DDevice9* pDevice);
    ~AnimationFist();

    void Initialize();
    void Invalidate();

    void changeNameFist(const std::string& name);

    inline void reloadDec() {
        unloadsDec();
        loadsDec();
    }

private:

    static void __fastcall drawFistIcon(class CSprite2d* sprite, void* trash, class CRect* rect, class CRGBA* color);

    LOAD_STATE m_stateLoad = LOAD_STATE::SUCCEEDED;
    
    void loadsDec();
    void unloadsDec();

    size_t findPosDecoder(const std::string& name);

    IDirect3DDevice9*         m_pDevice;
    bool                      m_bInitCurDec{};
    WebPDecoder**             m_pCurrentDec{};
    std::vector<std::string>  m_fileNames;
    std::vector<WebPDecoder*> m_decoders;

    std::atomic<uint32_t> m_nRunThreadsCount;
    static void threadLoadFiles(AnimationFist* pthis, size_t pos);
};
