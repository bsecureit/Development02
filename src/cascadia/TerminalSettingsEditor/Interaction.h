﻿// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "Interaction.g.h"
#include "InteractionPageNavigationState.g.h"
#include "Utils.h"

namespace winrt::Microsoft::Terminal::Settings::Editor::implementation
{
    struct InteractionPageNavigationState : InteractionPageNavigationStateT<InteractionPageNavigationState>
    {
    public:
        InteractionPageNavigationState(Model::GlobalAppSettings settings) :
            _Globals{ settings } {}

        GETSET_PROPERTY(Model::GlobalAppSettings, Globals, nullptr)
    };

    struct Interaction : InteractionT<Interaction>
    {
        Interaction();

        void OnNavigatedTo(winrt::Windows::UI::Xaml::Navigation::NavigationEventArgs e);

        GETSET_PROPERTY(Editor::InteractionPageNavigationState, State, nullptr);
    };
}

namespace winrt::Microsoft::Terminal::Settings::Editor::factory_implementation
{
    BASIC_FACTORY(Interaction);
}
