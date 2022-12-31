#pragma once
#include <d3d9.h>
#include <vector>

class WebPDecoder {
    class Frame {
        IDirect3DTexture9* texture = nullptr;
        int timestamp = 0;

    public:
        uint8_t* raw = nullptr;

        Frame() = delete;
        Frame(uint8_t* rw, int width, int height, int tmstamp);
        ~Frame();

        void Initialize(int width, int height, IDirect3DDevice9* pDevice);
        void Invalidate();

        inline auto getTexture() const noexcept {
            return texture;
        }
        inline auto getTimestamp() const noexcept {
            return timestamp;
        }
    };

    UINT64 m_startTick{};

    std::vector<Frame*> m_frames;
    size_t m_nCurrentFrame{};
    size_t m_nEndFrame;
    
    uint32_t m_nWidth;
    uint32_t m_nHeight;

    IDirect3DTexture9* m_pTitle{};

public:
    WebPDecoder() = delete;
    explicit WebPDecoder(const char* fileName, IDirect3DDevice9* pDevice);
    ~WebPDecoder();

    void Initialize(IDirect3DDevice9* pDevice) noexcept;
    void Invalidate() noexcept;
    inline void ResetCount() noexcept {
        m_startTick = GetTickCount64();
        m_nCurrentFrame = 0;
    }

    IDirect3DTexture9* getTitle(IDirect3DDevice9* pDevice) noexcept;
    IDirect3DTexture9* ProcessPlay() noexcept;
};

