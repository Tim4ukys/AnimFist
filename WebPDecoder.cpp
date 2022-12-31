#include "WebPDecoder.h"
#include <fstream>
#include <vector>
#include <webp/demux.h>
#include <iostream>

WebPDecoder::Frame::Frame(uint8_t* rw, int widht, int height, int tmstamp) : timestamp(tmstamp) {
    const size_t sz = widht * height * 4;
    raw = new uint8_t[sz];
    memcpy(raw, rw, sz);
}
WebPDecoder::Frame::~Frame() {
    Invalidate();
    delete[] raw;
}

inline void updateTexture(IDirect3DTexture9*& texture, uint8_t*& raw, const int& width, const int& height, const DWORD flags) {
    D3DLOCKED_RECT LockedRect;
    texture->LockRect(0, &LockedRect, nullptr, flags);
    auto surfaceData = static_cast<uint8_t*>(LockedRect.pBits);

    uint8_t* color = raw;
    for (int h{}; h < height; ++h) {
        for (int w{}; w < width; ++w) {
            auto* c = (uint8_t*)(surfaceData + (h * LockedRect.Pitch + w * 4));
            c[0] = color[2];
            c[1] = color[1];
            c[2] = color[0];
            c[3] = color[3];
            
            color += 4;
        }
    }
    texture->UnlockRect(0);
}

IDirect3DTexture9* WebPDecoder::getTitle(IDirect3DDevice9* pDevice) noexcept {
    if (!m_pTitle) {
        pDevice->CreateTexture(m_nWidth, m_nHeight, 1, 0,
                               D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_pTitle, nullptr);

        updateTexture(m_pTitle, m_frames[0]->raw, m_nWidth, m_nHeight, 0);
    }

    return m_pTitle;
}

void WebPDecoder::Frame::Initialize(int width, int height, IDirect3DDevice9* pDevice) {
    if (!pDevice || texture) return;

    pDevice->CreateTexture(width, height, 1, D3DUSAGE_DYNAMIC,
        D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, nullptr);

    updateTexture(texture, raw, width, height, D3DLOCK_DISCARD);
}
void WebPDecoder::Frame::Invalidate() {
    if (texture) {
        texture->Release();
        texture = nullptr;
    }
}

WebPDecoder::WebPDecoder(const char* fileName, IDirect3DDevice9* pDevice) {
    // readfile
    std::ifstream file{ fileName, std::ios_base::binary | std::ios_base::in | std::ios_base::ate };
    auto sz = size_t(file.tellg());
    std::vector<uint8_t> buff;
    buff.resize(sz);
    file.seekg(0);
    file.read((char*)buff.data(), sz);
    file.close();

    // readInfo
    WebPAnimDecoderOptions dec_options;
    dec_options.color_mode = MODE_ARGB;
    dec_options.use_threads = 0;
    WebPAnimDecoderOptionsInit(&dec_options);

    WebPData webp_data{ buff.data(), sz };
    WebPAnimDecoder* dec = WebPAnimDecoderNew(&webp_data, &dec_options);
    WebPAnimInfo anim_info;
    WebPAnimDecoderGetInfo(dec, &anim_info);
    m_nEndFrame = anim_info.frame_count;
    m_nWidth = anim_info.canvas_width;
    m_nHeight = anim_info.canvas_height;

    size_t i{};
    m_frames.resize(anim_info.frame_count);
    while (WebPAnimDecoderHasMoreFrames(dec)) {
        uint8_t* buf;
        int timestamp;
        WebPAnimDecoderGetNext(dec, &buf, &timestamp);

        m_frames[i++] = new Frame(buf, anim_info.canvas_width, anim_info.canvas_height, timestamp);
    }
    WebPAnimDecoderReset(dec);
    WebPAnimDecoderDelete(dec);
}
WebPDecoder::~WebPDecoder() {
    for (auto& f : m_frames)
        delete f;
    if (m_pTitle)
        m_pTitle->Release();
}

void WebPDecoder::Initialize(IDirect3DDevice9* pDevice) noexcept {
    for (auto& f : m_frames)
        f->Initialize(m_nWidth, m_nHeight, pDevice);
}
void WebPDecoder::Invalidate() noexcept {
    for (auto& f : m_frames)
        f->Invalidate();
}

IDirect3DTexture9* WebPDecoder::ProcessPlay() noexcept {
    if (auto tick = GetTickCount64();
        m_startTick + m_frames[m_nCurrentFrame]->getTimestamp() < tick) {
        if (++m_nCurrentFrame == m_nEndFrame)
            ResetCount();
    }
    return m_frames[m_nCurrentFrame]->getTexture();
}