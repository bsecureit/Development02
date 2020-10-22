// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "HighlightedTextControl.h"
#include "CommandPalette.g.h"
#include "FilteredCommand.g.h"
#include "../../cascadia/inc/cppwinrt_utils.h"

// fwdecl unittest classes
namespace TerminalAppLocalTests
{
    class FilteredCommandTests;
};

namespace winrt::TerminalApp::implementation
{
    struct FilteredCommand : FilteredCommandT<FilteredCommand>
    {
        FilteredCommand() = default;
        FilteredCommand(Microsoft::Terminal::Settings::Model::Command const& command);

        void UpdateFilter(winrt::hstring const& filter);

        WINRT_CALLBACK(PropertyChanged, Windows::UI::Xaml::Data::PropertyChangedEventHandler);
        OBSERVABLE_GETSET_PROPERTY(Microsoft::Terminal::Settings::Model::Command, Command, _PropertyChangedHandlers);
        OBSERVABLE_GETSET_PROPERTY(winrt::hstring, Filter, _PropertyChangedHandlers);
        OBSERVABLE_GETSET_PROPERTY(winrt::TerminalApp::HighlightedText, HighlightedName, _PropertyChangedHandlers);

    private:
        winrt::TerminalApp::HighlightedText _computeHighlightedName();
        Windows::UI::Xaml::Data::INotifyPropertyChanged::PropertyChanged_revoker _commandChangedRevoker;

        friend class TerminalAppLocalTests::FilteredCommandTests;
    };

    enum class CommandPaletteMode
    {
        ActionMode = 0,
        TabSearchMode,
        TabSwitchMode,
        CommandlineMode
    };

    struct CommandPalette : CommandPaletteT<CommandPalette>
    {
        CommandPalette();

        Windows::Foundation::Collections::IObservableVector<winrt::TerminalApp::FilteredCommand> FilteredActions();

        void SetCommands(Windows::Foundation::Collections::IVector<Microsoft::Terminal::Settings::Model::Command> const& actions);
        void SetKeyBindings(Microsoft::Terminal::TerminalControl::IKeyBindings bindings);

        void EnableCommandPaletteMode();

        void SetDispatch(const winrt::TerminalApp::ShortcutActionDispatch& dispatch);

        bool OnDirectKeyEvent(const uint32_t vkey, const uint8_t scanCode, const bool down);

        void SelectNextItem(const bool moveDown);

        // Tab Switcher
        void EnableTabSwitcherMode(const bool searchMode, const uint32_t startIdx);
        void OnTabsChanged(const Windows::Foundation::IInspectable& s, const Windows::Foundation::Collections::IVectorChangedEventArgs& e);

        WINRT_CALLBACK(PropertyChanged, Windows::UI::Xaml::Data::PropertyChangedEventHandler);
        OBSERVABLE_GETSET_PROPERTY(winrt::hstring, NoMatchesText, _PropertyChangedHandlers);
        OBSERVABLE_GETSET_PROPERTY(winrt::hstring, SearchBoxPlaceholderText, _PropertyChangedHandlers);
        OBSERVABLE_GETSET_PROPERTY(winrt::hstring, PrefixCharacter, _PropertyChangedHandlers);
        OBSERVABLE_GETSET_PROPERTY(winrt::hstring, ControlName, _PropertyChangedHandlers);
        OBSERVABLE_GETSET_PROPERTY(winrt::hstring, ParentCommandName, _PropertyChangedHandlers);

    private:
        friend struct CommandPaletteT<CommandPalette>; // for Xaml to bind events

        Windows::Foundation::Collections::IVector<Microsoft::Terminal::Settings::Model::Command> _allCommands{ nullptr };
        Windows::Foundation::Collections::IVector<Microsoft::Terminal::Settings::Model::Command> _currentNestedCommands{ nullptr };
        Windows::Foundation::Collections::IObservableVector<winrt::TerminalApp::FilteredCommand> _filteredActions{ nullptr };
        Windows::Foundation::Collections::IVector<Microsoft::Terminal::Settings::Model::Command> _nestedActionStack{ nullptr };

        winrt::TerminalApp::ShortcutActionDispatch _dispatch;

        Windows::Foundation::Collections::IVector<Microsoft::Terminal::Settings::Model::Command> _commandsToFilter();

        bool _lastFilterTextWasEmpty{ true };

        void _filterTextChanged(Windows::Foundation::IInspectable const& sender,
                                Windows::UI::Xaml::RoutedEventArgs const& args);
        void _previewKeyDownHandler(Windows::Foundation::IInspectable const& sender,
                                    Windows::UI::Xaml::Input::KeyRoutedEventArgs const& e);
        void _keyDownHandler(Windows::Foundation::IInspectable const& sender,
                             Windows::UI::Xaml::Input::KeyRoutedEventArgs const& e);
        void _keyUpHandler(Windows::Foundation::IInspectable const& sender,
                           Windows::UI::Xaml::Input::KeyRoutedEventArgs const& e);

        void _selectedCommandChanged(Windows::Foundation::IInspectable const& sender,
                                     Windows::UI::Xaml::RoutedEventArgs const& args);

        void _updateUIForStackChange();

        void _rootPointerPressed(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs const& e);
        void _backdropPointerPressed(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs const& e);

        void _listItemClicked(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::Controls::ItemClickEventArgs const& e);

        void _updateFilteredActions();

        std::vector<winrt::TerminalApp::FilteredCommand> _collectFilteredActions();

        static int _getWeight(const winrt::hstring& searchText, const winrt::hstring& name);
        void _close();

        CommandPaletteMode _currentMode;
        void _switchToMode(CommandPaletteMode mode);

        std::wstring _getTrimmedInput();
        void _evaluatePrefix();

        Microsoft::Terminal::TerminalControl::IKeyBindings _bindings;

        // Tab Switcher
        Windows::Foundation::Collections::IVector<Microsoft::Terminal::Settings::Model::Command> _allTabActions{ nullptr };
        uint32_t _switcherStartIdx;
        void _anchorKeyUpHandler();

        winrt::Windows::UI::Xaml::Controls::ListView::SizeChanged_revoker _sizeChangedRevoker;

        void _dispatchCommand(const Microsoft::Terminal::Settings::Model::Command& command);
        void _dispatchCommandline();
        void _dismissPalette();
    };
}

namespace winrt::TerminalApp::factory_implementation
{
    BASIC_FACTORY(FilteredCommand);
    BASIC_FACTORY(CommandPalette);
}
