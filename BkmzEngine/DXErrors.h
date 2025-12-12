#pragma once

#include <Windows.h>
#include <comdef.h>   // for _com_error
#include <string>
#include <stdexcept>

#if defined(DEBUG) || defined(_DEBUG)
#define DX_CALL(x) CheckHr((x), L#x, TEXT(__FILE__), __LINE__)
#else
#define DX_CALL(x) (x)
#endif


inline void CheckHr(
    HRESULT hr,
    const wchar_t *expr,
    const wchar_t *file,
    int line)
{
    if (SUCCEEDED(hr))
        return;

    // Turn HRESULT into a human-readable message
    _com_error err(hr);
    const wchar_t *errMsg = err.ErrorMessage(); // e.g. "The parameter is incorrect."

    std::wstring msg;
    msg.reserve(512);

    msg += L"DirectX call failed!\n\n";
    msg += L"Expression:\n  ";
    msg += expr;
    msg += L"\n\nFile:\n  ";
    msg += file;
    msg += L"\n\nLine:\n  ";
    msg += std::to_wstring(line);
    msg += L"\n\nHRESULT:\n  0x";

    wchar_t hrHex[16];
    swprintf_s(hrHex, L"%08X", static_cast<unsigned>(hr));
    msg += hrHex;

    msg += L"\n\nMessage:\n  ";
    msg += errMsg;

    MessageBoxW(nullptr, msg.c_str(), L"DX12 Error", MB_OK | MB_ICONERROR);

    // Also useful when running under debugger:
    OutputDebugStringW(msg.c_str());
    OutputDebugStringW(L"\n");

    // Throw to unwind / break execution
    throw std::runtime_error("HRESULT failure");
}
