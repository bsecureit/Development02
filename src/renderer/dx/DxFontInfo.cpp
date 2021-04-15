// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "precomp.h"

#include "DxFontInfo.h"

#include "unicode.hpp"

#include <VersionHelpers.h>

static constexpr std::wstring_view FALLBACK_FONT_FACES[] = { L"Consolas", L"Lucida Console", L"Courier New" };

using namespace Microsoft::Console::Render;

DxFontInfo::DxFontInfo() noexcept :
    _familyName(),
    _weight(DWRITE_FONT_WEIGHT_NORMAL),
    _style(DWRITE_FONT_STYLE_NORMAL),
    _stretch(DWRITE_FONT_STRETCH_NORMAL),
    _didFallback(false)
{
}

DxFontInfo::DxFontInfo(std::wstring_view familyName,
                       unsigned int weight,
                       DWRITE_FONT_STYLE style,
                       DWRITE_FONT_STRETCH stretch) :
    DxFontInfo(familyName, static_cast<DWRITE_FONT_WEIGHT>(weight), style, stretch)
{
}

DxFontInfo::DxFontInfo(std::wstring_view familyName,
                       DWRITE_FONT_WEIGHT weight,
                       DWRITE_FONT_STYLE style,
                       DWRITE_FONT_STRETCH stretch) :
    _familyName(familyName),
    _weight(weight),
    _style(style),
    _stretch(stretch),
    _didFallback(false)
{
}

std::wstring_view DxFontInfo::GetFamilyName() const noexcept
{
    return _familyName;
}

void DxFontInfo::SetFamilyName(const std::wstring_view familyName)
{
    _familyName = familyName;
}

DWRITE_FONT_WEIGHT DxFontInfo::GetWeight() const noexcept
{
    return _weight;
}

void DxFontInfo::SetWeight(const DWRITE_FONT_WEIGHT weight) noexcept
{
    _weight = weight;
}

DWRITE_FONT_STYLE DxFontInfo::GetStyle() const noexcept
{
    return _style;
}

void DxFontInfo::SetStyle(const DWRITE_FONT_STYLE style) noexcept
{
    _style = style;
}

DWRITE_FONT_STRETCH DxFontInfo::GetStretch() const noexcept
{
    return _stretch;
}

void DxFontInfo::SetStretch(const DWRITE_FONT_STRETCH stretch) noexcept
{
    _stretch = stretch;
}

bool DxFontInfo::GetFallback() const noexcept
{
    return _didFallback;
}

void DxFontInfo::SetFromEngine(const std::wstring_view familyName,
                               const DWRITE_FONT_WEIGHT weight,
                               const DWRITE_FONT_STYLE style,
                               const DWRITE_FONT_STRETCH stretch)
{
    _familyName = familyName;
    _weight = weight;
    _style = style;
    _stretch = stretch;
}

// Routine Description:
// - Attempts to locate the font given, but then begins falling back if we cannot find it.
// - We'll try to fall back to Consolas with the given weight/stretch/style first,
//   then try Consolas again with normal weight/stretch/style,
//   and if nothing works, then we'll throw an error.
// Arguments:
// - dwriteFactory - The DWrite factory to use
// - localeName - Locale to search for appropriate fonts
// Return Value:
// - Smart pointer holding interface reference for queryable font data.
[[nodiscard]] Microsoft::WRL::ComPtr<IDWriteFontFace1> DxFontInfo::ResolveFontFaceWithFallback(gsl::not_null<IDWriteFactory1*> dwriteFactory,
                                                                                               std::wstring& localeName)
{
    // First attempt to find exactly what the user asked for.
    _didFallback = false;
    auto face = _FindFontFace(dwriteFactory, localeName);

    if (!face)
    {
        // If we missed, try looking a little more by trimming the last word off the requested family name a few times.
        // Quite often, folks are specifying weights or something in the familyName and it causes failed resolution and
        // an unexpected error dialog. We theoretically could detect the weight words and convert them, but this
        // is the quick fix for the majority scenario.
        // The long/full fix is backlogged to GH#9744
        // Also this doesn't count as a fallback because we don't want to annoy folks with the warning dialog over
        // this resolution.
        while (!face && !_familyName.empty())
        {
            const auto lastSpace = _familyName.find_last_of(UNICODE_SPACE);

            // value is unsigned and npos will be greater than size.
            // if we didn't find anything to trim, leave.
            if (lastSpace >= _familyName.size())
            {
                break;
            }

            // trim string down to just before the found space
            // (space found at 6... trim from 0 for 6 length will give us 0-5 as the new string)
            _familyName = _familyName.substr(0, lastSpace);

            // Try to find it with the shortened family name
            face = _FindFontFace(dwriteFactory, localeName);
        }

        // Alright, if our quick shot at trimming didn't work either...
        // move onto looking up a font from our hardcoded list of fonts
        // that should really always be available.
        if (!face)
        {
            for (const auto fallbackFace : FALLBACK_FONT_FACES)
            {
                _familyName = fallbackFace;
                face = _FindFontFace(dwriteFactory, localeName);

                if (face)
                {
                    _didFallback = true;
                    break;
                }

                _familyName = fallbackFace;
                _weight = DWRITE_FONT_WEIGHT_NORMAL;
                _stretch = DWRITE_FONT_STRETCH_NORMAL;
                _style = DWRITE_FONT_STYLE_NORMAL;
                face = _FindFontFace(dwriteFactory, localeName);

                if (face)
                {
                    _didFallback = true;
                    break;
                }
            }
        }
    }

    THROW_HR_IF_NULL(E_FAIL, face);

    return face;
}

// Routine Description:
// - Locates a suitable font face from the given information
// Arguments:
// - dwriteFactory - The DWrite factory to use
// - localeName - Locale to search for appropriate fonts
// Return Value:
// - Smart pointer holding interface reference for queryable font data.
[[nodiscard]] Microsoft::WRL::ComPtr<IDWriteFontFace1> DxFontInfo::_FindFontFace(gsl::not_null<IDWriteFactory1*> dwriteFactory, std::wstring& localeName)
{
    Microsoft::WRL::ComPtr<IDWriteFontFace1> fontFace;

    Microsoft::WRL::ComPtr<IDWriteFontCollection> fontCollection;
    THROW_IF_FAILED(dwriteFactory->GetSystemFontCollection(&fontCollection, false));

    UINT32 familyIndex;
    BOOL familyExists;
    THROW_IF_FAILED(fontCollection->FindFamilyName(_familyName.data(), &familyIndex, &familyExists));

    // If the system collection missed, try the files sitting next to our binary.
    if (!familyExists)
    {
        auto&& nearbyCollection = NearbyCollection(dwriteFactory);

        // May be null on OS below Windows 10. If null, just skip the attempt.
        if (nearbyCollection)
        {
            nearbyCollection.As(&fontCollection);
            THROW_IF_FAILED(fontCollection->FindFamilyName(_familyName.data(), &familyIndex, &familyExists));
        }
    }

    if (familyExists)
    {
        Microsoft::WRL::ComPtr<IDWriteFontFamily> fontFamily;
        THROW_IF_FAILED(fontCollection->GetFontFamily(familyIndex, &fontFamily));

        Microsoft::WRL::ComPtr<IDWriteFont> font;
        THROW_IF_FAILED(fontFamily->GetFirstMatchingFont(GetWeight(), GetStretch(), GetStyle(), &font));

        Microsoft::WRL::ComPtr<IDWriteFontFace> fontFace0;
        THROW_IF_FAILED(font->CreateFontFace(&fontFace0));

        THROW_IF_FAILED(fontFace0.As(&fontFace));

        // Retrieve metrics in case the font we created was different than what was requested.
        _weight = font->GetWeight();
        _stretch = font->GetStretch();
        _style = font->GetStyle();

        // Dig the family name out at the end to return it.
        _familyName = _GetFontFamilyName(fontFamily.Get(), localeName);
    }

    return fontFace;
}

// Routine Description:
// - Retrieves the font family name out of the given object in the given locale.
// - If we can't find a valid name for the given locale, we'll fallback and report it back.
// Arguments:
// - fontFamily - DirectWrite font family object
// - localeName - The locale in which the name should be retrieved.
//              - If fallback occurred, this is updated to what we retrieved instead.
// Return Value:
// - Localized string name of the font family
[[nodiscard]] std::wstring DxFontInfo::_GetFontFamilyName(gsl::not_null<IDWriteFontFamily*> const fontFamily,
                                                          std::wstring& localeName)
{
    // See: https://docs.microsoft.com/en-us/windows/win32/api/dwrite/nn-dwrite-idwritefontcollection
    Microsoft::WRL::ComPtr<IDWriteLocalizedStrings> familyNames;
    THROW_IF_FAILED(fontFamily->GetFamilyNames(&familyNames));

    // First we have to find the right family name for the locale. We're going to bias toward what the caller
    // requested, but fallback if we need to and reply with the locale we ended up choosing.
    UINT32 index = 0;
    BOOL exists = false;

    // This returns S_OK whether or not it finds a locale name. Check exists field instead.
    // If it returns an error, it's a real problem, not an absence of this locale name.
    // https://docs.microsoft.com/en-us/windows/win32/api/dwrite/nf-dwrite-idwritelocalizedstrings-findlocalename
    THROW_IF_FAILED(familyNames->FindLocaleName(localeName.data(), &index, &exists));

    // If we tried and it still doesn't exist, try with the fallback locale.
    if (!exists)
    {
        localeName = L"en-us";
        THROW_IF_FAILED(familyNames->FindLocaleName(localeName.data(), &index, &exists));
    }

    // If it still doesn't exist, we're going to try index 0.
    if (!exists)
    {
        index = 0;

        // Get the locale name out so at least the caller knows what locale this name goes with.
        UINT32 length = 0;
        THROW_IF_FAILED(familyNames->GetLocaleNameLength(index, &length));
        localeName.resize(length);

        // https://docs.microsoft.com/en-us/windows/win32/api/dwrite/nf-dwrite-idwritelocalizedstrings-getlocalenamelength
        // https://docs.microsoft.com/en-us/windows/win32/api/dwrite/nf-dwrite-idwritelocalizedstrings-getlocalename
        // GetLocaleNameLength does not include space for null terminator, but GetLocaleName needs it so add one.
        THROW_IF_FAILED(familyNames->GetLocaleName(index, localeName.data(), length + 1));
    }

    // OK, now that we've decided which family name and the locale that it's in... let's go get it.
    UINT32 length = 0;
    THROW_IF_FAILED(familyNames->GetStringLength(index, &length));

    // Make our output buffer and resize it so it is allocated.
    std::wstring retVal;
    retVal.resize(length);

    // FINALLY, go fetch the string name.
    // https://docs.microsoft.com/en-us/windows/win32/api/dwrite/nf-dwrite-idwritelocalizedstrings-getstringlength
    // https://docs.microsoft.com/en-us/windows/win32/api/dwrite/nf-dwrite-idwritelocalizedstrings-getstring
    // Once again, GetStringLength is without the null, but GetString needs the null. So add one.
    THROW_IF_FAILED(familyNames->GetString(index, retVal.data(), length + 1));

    // and return it.
    return retVal;
}

// Routine Description:
// - Creates a DirectWrite font collection of font files that are sitting next to the running
//   binary (in the same directory as the EXE).
// Arguments:
// - dwriteFactory - The DWrite factory to use
// Return Value:
// - DirectWrite font collection. May be null if one cannot be created.
[[nodiscard]] const Microsoft::WRL::ComPtr<IDWriteFontCollection1>& DxFontInfo::NearbyCollection(gsl::not_null<IDWriteFactory1*> dwriteFactory) const
{
    // Magic static so we only attempt to grovel the hard disk once no matter how many instances
    // of the font collection itself we require.
    static const auto knownPaths = s_GetNearbyFonts();

    // The convenience interfaces for loading fonts from files
    // are only available on Windows 10+.
    // Don't try to look up if below that OS version.
    static const bool s_isWindows10OrGreater = IsWindows10OrGreater();

    if (s_isWindows10OrGreater && !_nearbyCollection)
    {
        // Factory3 has a convenience to get us a font set builder.
        ::Microsoft::WRL::ComPtr<IDWriteFactory3> factory3;
        THROW_IF_FAILED(dwriteFactory->QueryInterface<IDWriteFactory3>(&factory3));

        ::Microsoft::WRL::ComPtr<IDWriteFontSetBuilder> fontSetBuilder;
        THROW_IF_FAILED(factory3->CreateFontSetBuilder(&fontSetBuilder));

        // Builder2 has a convenience to just feed in paths to font files.
        ::Microsoft::WRL::ComPtr<IDWriteFontSetBuilder2> fontSetBuilder2;
        THROW_IF_FAILED(fontSetBuilder.As(&fontSetBuilder2));

        for (auto& p : knownPaths)
        {
            fontSetBuilder2->AddFontFile(p.c_str());
        }

        ::Microsoft::WRL::ComPtr<IDWriteFontSet> fontSet;
        THROW_IF_FAILED(fontSetBuilder2->CreateFontSet(&fontSet));

        THROW_IF_FAILED(factory3->CreateFontCollectionFromFontSet(fontSet.Get(), &_nearbyCollection));
    }

    return _nearbyCollection;
}

// Routine Description:
// - Digs through the directory that the current executable is running within to find
//   any TTF files sitting next to it.
// Arguments:
// - <none>
// Return Value:
// - Iterable collection of filesystem paths, one per font file that was found
[[nodiscard]] std::vector<std::filesystem::path> DxFontInfo::s_GetNearbyFonts()
{
    std::vector<std::filesystem::path> paths;

    // Find the directory we're running from then enumerate all the TTF files
    // sitting next to us.
    const std::filesystem::path module{ wil::GetModuleFileNameW<std::wstring>(nullptr) };
    const auto folder{ module.parent_path() };

    for (auto& p : std::filesystem::directory_iterator(folder))
    {
        if (p.is_regular_file())
        {
            auto extension = p.path().extension().wstring();
            std::transform(extension.begin(), extension.end(), extension.begin(), std::towlower);

            static constexpr std::wstring_view ttfExtension{ L".ttf" };
            if (ttfExtension == extension)
            {
                paths.push_back(p);
            }
        }
    }

    return paths;
}
