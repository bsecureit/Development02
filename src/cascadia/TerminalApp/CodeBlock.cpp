// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "CodeBlock.h"
#include <LibraryResources.h>

#include "CodeBlock.g.cpp"
#include "RequestRunCommandsArgs.g.cpp"

namespace winrt
{
    namespace MUX = Microsoft::UI::Xaml;
    namespace WUX = Windows::UI::Xaml;
    using IInspectable = Windows::Foundation::IInspectable;
}

namespace winrt::TerminalApp::implementation
{
    CodeBlock::CodeBlock(const winrt::hstring& initialCommandlines) :
        Commandlines(initialCommandlines)
    {
        InitializeComponent();
    }
    void CodeBlock::_playPressed(const Windows::Foundation::IInspectable&,
                                 const Windows::UI::Xaml::Input::TappedRoutedEventArgs&)
    {
        auto args = winrt::make_self<RequestRunCommandsArgs>(Commandlines());
        RequestRunCommands.raise(*this, *args);
    }
}
