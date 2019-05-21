/*++
Copyright (c) Microsoft Corporation

Module Name:
- utils.hpp

Abstract:
- Helpful cross-lib utilities

Author(s):
- Mike Griese (migrie) 12-Jun-2018
--*/

namespace Microsoft::Console::Utils
{
    bool IsValidHandle(const HANDLE handle) noexcept;

    std::wstring GuidToString(const GUID guid);
    GUID GuidFromString(const std::wstring wstr);

    std::wstring ColorToHexString(const COLORREF color);
    COLORREF ColorFromHexString(const std::wstring wstr);

    void InitializeCampbellColorTable(gsl::span<COLORREF>& table);
    void Initialize256ColorTable(gsl::span<COLORREF>& table);
    void SetColorTableAlpha(gsl::span<COLORREF>& table, const BYTE newAlpha);

    template <typename T>
    T EndianSwap(T value);

    template <>
    constexpr uint16_t EndianSwap(uint16_t value)
    {
        return (value & 0xFF00) >> 8 |
               (value & 0x00FF) << 8;
    }

    template <>
    constexpr uint32_t EndianSwap(uint32_t value)
    {
        return (value & 0xFF000000) >> 24 |
               (value & 0x00FF0000) >>  8 |
               (value & 0x0000FF00) <<  8 |
               (value & 0x000000FF) <<  24;
    }

    template <>
    GUID EndianSwap(GUID value)
    {
        value.Data1 = EndianSwap(value.Data1);
        value.Data2 = EndianSwap(value.Data2);
        value.Data3 = EndianSwap(value.Data3);
        return value;
    }
}
