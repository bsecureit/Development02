﻿// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "AppKeyBindings.h"
#include "KeyChordSerialization.h"

#include "AppKeyBindings.g.cpp"

using namespace winrt::Microsoft::Terminal;
using namespace winrt::TerminalApp;

namespace winrt::TerminalApp::implementation
{
    void AppKeyBindings::SetKeyBinding(const TerminalApp::ShortcutAction& action,
                                       const Settings::KeyChord& chord)
    {
        _keyShortcuts[chord] = action;
    }

    Microsoft::Terminal::Settings::KeyChord AppKeyBindings::GetKeyBinding(TerminalApp::ShortcutAction const& action)
    {
        for (auto& kv : _keyShortcuts)
        {
            if (kv.second == action)
            {
                return kv.first;
            }
        }
        return { nullptr };
    }

    bool AppKeyBindings::TryKeyChord(const Settings::KeyChord& kc)
    {
        const auto keyIter = _keyShortcuts.find(kc);
        if (keyIter != _keyShortcuts.end())
        {
            const auto action = keyIter->second;
            return _DoAction(action);
        }
        return false;
    }

    bool AppKeyBindings::_DoAction(ShortcutAction action)
    {
        switch (action)
        {
        case ShortcutAction::CopyText:
        {
            auto args = CopyTextArgs();
            args.TrimWhitespace(true);
            auto eventArgs = CopyTextEventArgs(args);
            _CopyTextHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::CopyTextWithoutNewlines:
        {
            auto args = CopyTextArgs();
            args.TrimWhitespace(false);
            auto eventArgs = CopyTextEventArgs(args);
            _CopyTextHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::PasteText:
        {
            auto eventArgs = PasteTextEventArgs();
            _PasteTextHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::NewTab:
        {
            auto eventArgs = NewTabEventArgs();
            _NewTabHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::DuplicateTab:
        {
            auto eventArgs = DuplicateTabEventArgs();
            _DuplicateTabHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::OpenSettings:
        {
            auto eventArgs = OpenSettingsEventArgs();
            _OpenSettingsHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }

        case ShortcutAction::NewTabProfile0:
        {
            auto args = NewTabWithProfileArgs();
            args.ProfileIndex(0);
            auto eventArgs = NewTabWithProfileEventArgs(args);
            _NewTabWithProfileHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::NewTabProfile1:
        {
            auto args = NewTabWithProfileArgs();
            args.ProfileIndex(1);
            auto eventArgs = NewTabWithProfileEventArgs(args);
            _NewTabWithProfileHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::NewTabProfile2:
        {
            auto args = NewTabWithProfileArgs();
            args.ProfileIndex(2);
            auto eventArgs = NewTabWithProfileEventArgs(args);
            _NewTabWithProfileHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::NewTabProfile3:
        {
            auto args = NewTabWithProfileArgs();
            args.ProfileIndex(3);
            auto eventArgs = NewTabWithProfileEventArgs(args);
            _NewTabWithProfileHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::NewTabProfile4:
        {
            auto args = NewTabWithProfileArgs();
            args.ProfileIndex(4);
            auto eventArgs = NewTabWithProfileEventArgs(args);
            _NewTabWithProfileHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::NewTabProfile5:
        {
            auto args = NewTabWithProfileArgs();
            args.ProfileIndex(5);
            auto eventArgs = NewTabWithProfileEventArgs(args);
            _NewTabWithProfileHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::NewTabProfile6:
        {
            auto args = NewTabWithProfileArgs();
            args.ProfileIndex(6);
            auto eventArgs = NewTabWithProfileEventArgs(args);
            _NewTabWithProfileHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::NewTabProfile7:
        {
            auto args = NewTabWithProfileArgs();
            args.ProfileIndex(7);
            auto eventArgs = NewTabWithProfileEventArgs(args);
            _NewTabWithProfileHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::NewTabProfile8:
        {
            auto args = NewTabWithProfileArgs();
            args.ProfileIndex(8);
            auto eventArgs = NewTabWithProfileEventArgs(args);
            _NewTabWithProfileHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }

        case ShortcutAction::NewWindow:
        {
            auto eventArgs = NewWindowEventArgs();
            _NewWindowHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::CloseWindow:
        {
            auto eventArgs = CloseWindowEventArgs();
            _CloseWindowHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::CloseTab:
        {
            auto eventArgs = CloseTabEventArgs();
            _CloseTabHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::ClosePane:
        {
            auto eventArgs = ClosePaneEventArgs();
            _ClosePaneHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }

        case ShortcutAction::ScrollUp:
        {
            auto eventArgs = ScrollUpEventArgs();
            _ScrollUpHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::ScrollDown:
        {
            auto eventArgs = ScrollDownEventArgs();
            _ScrollDownHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::ScrollUpPage:
        {
            auto eventArgs = ScrollUpPageEventArgs();
            _ScrollUpPageHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::ScrollDownPage:
        {
            auto eventArgs = ScrollDownPageEventArgs();
            _ScrollDownPageHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }

        case ShortcutAction::NextTab:
        {
            auto eventArgs = NextTabEventArgs();
            _NextTabHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::PrevTab:
        {
            auto eventArgs = PrevTabEventArgs();
            _PrevTabHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }

        case ShortcutAction::SplitVertical:
        {
            auto eventArgs = SplitVerticalEventArgs();
            _SplitVerticalHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::SplitHorizontal:
        {
            auto eventArgs = SplitHorizontalEventArgs();
            _SplitHorizontalHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }

        case ShortcutAction::SwitchToTab0:
        {
            auto eventArgs = SwitchToTabEventArgs();
            eventArgs.TabIndex(0);
            _SwitchToTabHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::SwitchToTab1:
        {
            auto eventArgs = SwitchToTabEventArgs();
            eventArgs.TabIndex(1);
            _SwitchToTabHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::SwitchToTab2:
        {
            auto eventArgs = SwitchToTabEventArgs();
            eventArgs.TabIndex(2);
            _SwitchToTabHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::SwitchToTab3:
        {
            auto eventArgs = SwitchToTabEventArgs();
            eventArgs.TabIndex(3);
            _SwitchToTabHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::SwitchToTab4:
        {
            auto eventArgs = SwitchToTabEventArgs();
            eventArgs.TabIndex(4);
            _SwitchToTabHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::SwitchToTab5:
        {
            auto eventArgs = SwitchToTabEventArgs();
            eventArgs.TabIndex(5);
            _SwitchToTabHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::SwitchToTab6:
        {
            auto eventArgs = SwitchToTabEventArgs();
            eventArgs.TabIndex(6);
            _SwitchToTabHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::SwitchToTab7:
        {
            auto eventArgs = SwitchToTabEventArgs();
            eventArgs.TabIndex(7);
            _SwitchToTabHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::SwitchToTab8:
        {
            auto eventArgs = SwitchToTabEventArgs();
            eventArgs.TabIndex(8);
            _SwitchToTabHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::ResizePaneLeft:
        {
            auto eventArgs = ResizePaneEventArgs();
            eventArgs.Direction(Direction::Left);
            _ResizePaneHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::ResizePaneRight:
        {
            auto eventArgs = ResizePaneEventArgs();
            eventArgs.Direction(Direction::Right);
            _ResizePaneHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::ResizePaneUp:
        {
            auto eventArgs = ResizePaneEventArgs();
            eventArgs.Direction(Direction::Up);
            _ResizePaneHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::ResizePaneDown:
        {
            auto eventArgs = ResizePaneEventArgs();
            eventArgs.Direction(Direction::Down);
            _ResizePaneHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::MoveFocusLeft:
        {
            auto eventArgs = MoveFocusEventArgs();
            eventArgs.Direction(Direction::Left);
            _MoveFocusHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::MoveFocusRight:
        {
            auto eventArgs = MoveFocusEventArgs();
            eventArgs.Direction(Direction::Right);
            _MoveFocusHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::MoveFocusUp:
        {
            auto eventArgs = MoveFocusEventArgs();
            eventArgs.Direction(Direction::Up);
            _MoveFocusHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        case ShortcutAction::MoveFocusDown:
        {
            auto eventArgs = MoveFocusEventArgs();
            eventArgs.Direction(Direction::Down);
            _MoveFocusHandlers(*this, eventArgs);
            return eventArgs.Handled();
        }
        default:
            return false;
        }
        return false;
    }

    // Method Description:
    // - Takes the KeyModifier flags from Terminal and maps them to the WinRT types which are used by XAML
    // Return Value:
    // - a Windows::System::VirtualKeyModifiers object with the flags of which modifiers used.
    Windows::System::VirtualKeyModifiers AppKeyBindings::ConvertVKModifiers(Settings::KeyModifiers modifiers)
    {
        Windows::System::VirtualKeyModifiers keyModifiers = Windows::System::VirtualKeyModifiers::None;

        if (WI_IsFlagSet(modifiers, Settings::KeyModifiers::Ctrl))
        {
            keyModifiers |= Windows::System::VirtualKeyModifiers::Control;
        }
        if (WI_IsFlagSet(modifiers, Settings::KeyModifiers::Shift))
        {
            keyModifiers |= Windows::System::VirtualKeyModifiers::Shift;
        }
        if (WI_IsFlagSet(modifiers, Settings::KeyModifiers::Alt))
        {
            // note: Menu is the Alt VK_MENU
            keyModifiers |= Windows::System::VirtualKeyModifiers::Menu;
        }

        return keyModifiers;
    }

}
