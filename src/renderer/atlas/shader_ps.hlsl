// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "dwrite.hlsl"
#include "shader_common.hlsl"

cbuffer ConstBuffer : register(b0)
{
    float4 backgroundColor;
    float2 backgroundCellSize;
    float2 backgroundCellCount;
    float4 gammaRatios;
    float enhancedContrast;
    float underlineWidth;
    float curlyLineHeight;
    float underlineCellOffset;
}

Texture2D<float4> background : register(t0);
Texture2D<float4> glyphAtlas : register(t1);

struct Output
{
    float4 color;
    float4 weights;
};

// clang-format off
Output main(PSData data) : SV_Target
// clang-format on
{
    float4 color;
    float4 weights;

    switch (data.shadingType)
    {
    case SHADING_TYPE_TEXT_BACKGROUND:
    {
        const float2 cell = data.position.xy / backgroundCellSize;
        color = all(cell < backgroundCellCount) ? background[cell] : backgroundColor;
        weights = float4(1, 1, 1, 1);
        break;
    }
    case SHADING_TYPE_TEXT_GRAYSCALE:
    {
        // These are independent of the glyph texture and could be moved to the vertex shader or CPU side of things.
        const float4 foreground = premultiplyColor(data.color);
        const float blendEnhancedContrast = DWrite_ApplyLightOnDarkContrastAdjustment(enhancedContrast, data.color.rgb);
        const float intensity = DWrite_CalcColorIntensity(data.color.rgb);
        // These aren't.
        const float4 glyph = glyphAtlas[data.texcoord];
        const float contrasted = DWrite_EnhanceContrast(glyph.a, blendEnhancedContrast);
        const float alphaCorrected = DWrite_ApplyAlphaCorrection(contrasted, intensity, gammaRatios);
        color = alphaCorrected * foreground;
        weights = color.aaaa;
        break;
    }
    case SHADING_TYPE_TEXT_CLEARTYPE:
    {
        // These are independent of the glyph texture and could be moved to the vertex shader or CPU side of things.
        const float blendEnhancedContrast = DWrite_ApplyLightOnDarkContrastAdjustment(enhancedContrast, data.color.rgb);
        // These aren't.
        const float4 glyph = glyphAtlas[data.texcoord];
        const float3 contrasted = DWrite_EnhanceContrast3(glyph.rgb, blendEnhancedContrast);
        const float3 alphaCorrected = DWrite_ApplyAlphaCorrection3(contrasted, data.color.rgb, gammaRatios);
        weights = float4(alphaCorrected * data.color.a, 1);
        color = weights * data.color;
        break;
    }
    case SHADING_TYPE_TEXT_PASSTHROUGH:
    {
        color = glyphAtlas[data.texcoord];
        weights = color.aaaa;
        break;
    }
    case SHADING_TYPE_DOTTED_LINE:
    {
        const bool on = frac(data.position.x / (2.0f * underlineWidth)) < 0.5f;
        color = on * premultiplyColor(data.color);
        weights = color.aaaa;
        break;
    }
    case SHADING_TYPE_DOTTED_LINE_WIDE:
    {
        const bool on = frac(data.position.x / (4.0f * underlineWidth)) < 0.5f;
        color = on * premultiplyColor(data.color);
        weights = color.aaaa;
        break;
    }
    case SHADING_TYPE_DASHED_LINE:
    {
        const bool on = frac(data.position.x / backgroundCellSize.x) < 0.5f;
        color = on * premultiplyColor(data.color);
        weights = color.aaaa;
        break;
    }
    case SHADING_TYPE_DASHED_LINE_WIDE:
    {
        const bool on = frac(data.position.x / (2.0f * backgroundCellSize.x)) < 0.5f;
        color = on * premultiplyColor(data.color);
        weights = color.aaaa;
        break;
    }
    case SHADING_TYPE_CURLY_LINE:
    {
        const float strokeWidthHalf = underlineWidth / 2.0f;
        const int cellIdxY = data.position.y / backgroundCellSize.y;
        const float cellPosY = cellIdxY * backgroundCellSize.y;
        const float centerY = cellPosY + underlineCellOffset + strokeWidthHalf;
        const float Pi = radians(180);
        const float freq = 2.0f * Pi / backgroundCellSize.x;
        const float amp = curlyLineHeight - 1.0f; // -1.0f avoids clipping at the peaks

        // The wave starts with a negative-peak(trough). To make it start with a positive-peak(crest), we phase shift it by `Pi`.
        const float s = sin(data.position.x * freq + Pi);
        const float d = abs(centerY + s * amp - data.position.y);
        const float a = 1 - saturate(d - strokeWidthHalf);
        color = a * premultiplyColor(data.color);
        weights = color.aaaa;
        break;
    }
    case SHADING_TYPE_CURLY_LINE_WIDE:
    {
        float strokeWidthHalf = underlineWidth / 2.0f;
        const int cellIdxY = data.position.y / backgroundCellSize.y;
        const float cellPosY = cellIdxY * backgroundCellSize.y;
        float centerY = cellPosY + underlineCellOffset + strokeWidthHalf;
        const float Pi = radians(180);
        float freq = 2.0f * Pi / backgroundCellSize.x;
        float amp = curlyLineHeight - 1.0f; // -1.0f avoids clipping at the peaks

        // In 'Wide' case, we need to draw the same wave on an area twice as big.
        strokeWidthHalf *= 2;
        centerY -= curlyLineHeight + strokeWidthHalf;
        freq /= 2;
        amp *= 2;

        // The wave starts with a negative-peak(trough). To make it start with a positive-peak(crest), we phase shift it by `Pi`.
        const float s = sin(data.position.x * freq + Pi);
        const float d = abs(centerY + s * amp - data.position.y);
        const float a = 1 - saturate(d - strokeWidthHalf);
        color = a * premultiplyColor(data.color);
        weights = color.aaaa;
        break;
    }
    default:
    {
        color = premultiplyColor(data.color);
        weights = color.aaaa;
        break;
    }
    }

    Output output;
    output.color = color;
    output.weights = weights;
    return output;
}
