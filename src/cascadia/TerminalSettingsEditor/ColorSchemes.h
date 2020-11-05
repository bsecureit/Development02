﻿// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "ColorTableEntry.g.h"
#include "ColorSchemes.g.h"
#include "ColorSchemesPageNavigationState.g.h"
#include "Utils.h"

namespace winrt::Microsoft::Terminal::Settings::Editor::implementation
{
    struct ColorSchemesPageNavigationState : ColorSchemesPageNavigationStateT<ColorSchemesPageNavigationState>
    {
    public:
        ColorSchemesPageNavigationState(Model::GlobalAppSettings settings) :
            _Globals{ settings } {}

        GETSET_PROPERTY(Model::GlobalAppSettings, Globals, nullptr);
    };

    struct ColorSchemes : ColorSchemesT<ColorSchemes>
    {
        ColorSchemes();

        void OnNavigatedTo(winrt::Windows::UI::Xaml::Navigation::NavigationEventArgs e);

        void ColorSchemeSelectionChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::Controls::SelectionChangedEventArgs const& args);
        void ColorPickerChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::Controls::ColorChangedEventArgs const& args);

        GETSET_PROPERTY(Editor::ColorSchemesPageNavigationState, State, nullptr);
        GETSET_PROPERTY(Windows::Foundation::Collections::IObservableVector<winrt::Microsoft::Terminal::Settings::Editor::ColorTableEntry>, CurrentColorTable, nullptr);
        GETSET_PROPERTY(Windows::Foundation::Collections::IObservableVector<winrt::hstring>, ColorSchemeList, nullptr);

        WINRT_CALLBACK(PropertyChanged, Windows::UI::Xaml::Data::PropertyChangedEventHandler);
        OBSERVABLE_GETSET_PROPERTY(winrt::Microsoft::Terminal::Settings::Model::ColorScheme, CurrentColorScheme, _PropertyChangedHandlers, nullptr);

    private:
        void _UpdateColorTable(const winrt::Microsoft::Terminal::Settings::Model::ColorScheme& colorScheme);
        void _UpdateColorSchemeList();
    };

    struct ColorTableEntry : ColorTableEntryT<ColorTableEntry>
    {
    public:
        ColorTableEntry(uint8_t index, Windows::UI::Color color);

        WINRT_CALLBACK(PropertyChanged, Windows::UI::Xaml::Data::PropertyChangedEventHandler);
        OBSERVABLE_GETSET_PROPERTY(winrt::hstring, Name, _PropertyChangedHandlers);
        OBSERVABLE_GETSET_PROPERTY(IInspectable, Index, _PropertyChangedHandlers);
        OBSERVABLE_GETSET_PROPERTY(Windows::UI::Color, Color, _PropertyChangedHandlers);
    };
}

namespace winrt::Microsoft::Terminal::Settings::Editor::factory_implementation
{
    BASIC_FACTORY(ColorSchemes);
}
