﻿// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "KeyChord.h"

#include "KeyChord.g.cpp"

using VirtualKeyModifiers = winrt::Windows::System::VirtualKeyModifiers;

namespace winrt::Microsoft::Terminal::Control::implementation
{
    static VirtualKeyModifiers modifiersFromBooleans(bool ctrl, bool alt, bool shift, bool win)
    {
        VirtualKeyModifiers modifiers = VirtualKeyModifiers::None;
        WI_SetFlagIf(modifiers, VirtualKeyModifiers::Control, ctrl);
        WI_SetFlagIf(modifiers, VirtualKeyModifiers::Menu, alt);
        WI_SetFlagIf(modifiers, VirtualKeyModifiers::Shift, shift);
        WI_SetFlagIf(modifiers, VirtualKeyModifiers::Windows, win);
        return modifiers;
    }

    KeyChord::KeyChord(bool ctrl, bool alt, bool shift, bool win, int32_t vkey, int32_t scanCode) noexcept :
        _modifiers{ modifiersFromBooleans(ctrl, alt, shift, win) },
        _vkey{ vkey },
        _scanCode{ scanCode }
    {
    }

    KeyChord::KeyChord(const VirtualKeyModifiers modifiers, int32_t vkey, int32_t scanCode) noexcept :
        _modifiers{ modifiers },
        _vkey{ vkey },
        _scanCode{ scanCode }
    {
    }

    VirtualKeyModifiers KeyChord::Modifiers() noexcept
    {
        return _modifiers;
    }

    void KeyChord::Modifiers(VirtualKeyModifiers const& value) noexcept
    {
        _modifiers = value;
    }

    int32_t KeyChord::Vkey() noexcept
    {
        return _vkey;
    }

    void KeyChord::Vkey(int32_t value) noexcept
    {
        _vkey = value;
    }

    int32_t KeyChord::ScanCode() noexcept {
        return _scanCode;
    }

    void KeyChord::ScanCode(int32_t value) noexcept {
        _scanCode = value;
    }
}
