#include "dxscreen.h"
#include "hook.h"

#include <d3d9.h>


struct THookCtx {
    void* PresentFun = nullptr;
    void* PresentExFun = nullptr;
    TScreenCallback Callback;
};
static THookCtx* HookCtx = new THookCtx();

void GetDX9Screenshot(IDirect3DDevice9* device) {
    HRESULT hr;

    IDirect3DSurface9* backbuffer;
    hr = device->GetRenderTarget(0, &backbuffer);
    if (FAILED(hr)) {
        return;
    }

    D3DSURFACE_DESC desc;
    hr = backbuffer->GetDesc(&desc);
    if (FAILED(hr)) {
        return;
    }

    IDirect3DSurface9* buffer;
    hr = device->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format,
                                             D3DPOOL_SYSTEMMEM, &buffer, nullptr);
    if (FAILED(hr)) {
        return;
    }

    hr = device->GetRenderTargetData(backbuffer, buffer);
    if (FAILED(hr)) {
        return;
    }

    backbuffer->Release();

    D3DLOCKED_RECT rect;
    hr = buffer->LockRect(&rect, NULL, D3DLOCK_READONLY);
    if (FAILED(hr)) {
        return;
    }

    QImage screenShotImg = IntArrayToQImage((char*)rect.pBits, desc.Height, desc.Width);
    HookCtx->Callback(screenShotImg);

    buffer->Release();
}

static HRESULT STDMETHODCALLTYPE HookPresent(IDirect3DDevice9* device,
                CONST RECT* srcRect, CONST RECT* dstRect,
                HWND overrideWindow, CONST RGNDATA* dirtyRegion)
{
    UnHook(HookCtx->PresentFun);
    UnHook(HookCtx->PresentExFun);
    GetDX9Screenshot(device);
    return device->Present(srcRect, dstRect, overrideWindow, dirtyRegion);
}

static HRESULT STDMETHODCALLTYPE HookPresentEx(IDirect3DDevice9Ex* device,
                CONST RECT* srcRect, CONST RECT* dstRect,
                HWND overrideWindow, CONST RGNDATA* dirtyRegion,
                DWORD flags)
{
    UnHook(HookCtx->PresentFun);
    UnHook(HookCtx->PresentExFun);
    GetDX9Screenshot(device);
    return device->PresentEx(srcRect, dstRect, overrideWindow, dirtyRegion, flags);
}

void MakeDX9Screen(const TScreenCallback& callback,
                   uint64_t presentOffset,
                   uint64_t presentExOffset)
{
    HMODULE dx9module = GetSystemModule("d3d9.dll");
    if (!dx9module) {
        return;
    }

    HookCtx->Callback = callback;
    HookCtx->PresentFun = (void*)((uintptr_t)dx9module + (uintptr_t)presentOffset);
    Hook(HookCtx->PresentFun, HookPresent);
    HookCtx->PresentExFun = (void*)((uintptr_t)dx9module + (uintptr_t)presentExOffset);
    Hook(HookCtx->PresentFun, HookPresentEx);
}