// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "precomp.h"

#include "ImageSlice.hpp"
#include "Row.hpp"
#include "textBuffer.hpp"

ImageSlice::ImageSlice(const til::size cellSize) noexcept :
    _cellSize{ cellSize }
{
}

til::size ImageSlice::CellSize() const noexcept
{
    return _cellSize;
}

til::CoordType ImageSlice::ColumnOffset() const noexcept
{
    return _columnBegin;
}

til::CoordType ImageSlice::PixelWidth() const noexcept
{
    return _pixelWidth;
}

std::span<const RGBQUAD> ImageSlice::Pixels() const noexcept
{
    return _pixelBuffer;
}

const RGBQUAD* ImageSlice::Pixels(const til::CoordType columnBegin) const noexcept
{
    const auto pixelOffset = (columnBegin - _columnBegin) * _cellSize.width;
    return &til::at(_pixelBuffer, pixelOffset);
}

RGBQUAD* ImageSlice::MutablePixels(const til::CoordType columnBegin, const til::CoordType columnEnd)
{
    // IF the buffer is empty or isn't large enough for the requested range, we'll need to resize it.
    if (_pixelBuffer.empty() || columnBegin < _columnBegin || columnEnd > _columnEnd)
    {
        const auto oldColumnBegin = _columnBegin;
        const auto oldPixelWidth = _pixelWidth;
        const auto existingData = !_pixelBuffer.empty();
        _columnBegin = existingData ? std::min(_columnBegin, columnBegin) : columnBegin;
        _columnEnd = existingData ? std::max(_columnEnd, columnEnd) : columnEnd;
        _pixelWidth = (_columnEnd - _columnBegin) * _cellSize.width;
        _pixelWidth = (_pixelWidth + 3) & ~3; // Renderer needs this as a multiple of 4
        const auto bufferSize = _pixelWidth * _cellSize.height;
        if (existingData)
        {
            // If there is existing data in the buffer, we need to copy it
            // across to the appropriate position in the new buffer.
            auto newPixelBuffer = std::vector<RGBQUAD>(bufferSize);
            const auto newOffset = (oldColumnBegin - _columnBegin) * _cellSize.width;
            auto newIterator = newPixelBuffer.begin() + newOffset;
            auto oldIterator = _pixelBuffer.begin();
            for (auto i = 0; i < _cellSize.height; i++)
            {
                std::copy_n(oldIterator, oldPixelWidth, newIterator);
                std::advance(oldIterator, oldPixelWidth);
                std::advance(newIterator, _pixelWidth);
            }
            _pixelBuffer = std::move(newPixelBuffer);
        }
        else
        {
            // Otherwise we just initialize the buffer to the correct size.
            _pixelBuffer.resize(bufferSize);
        }
    }
    const auto pixelOffset = (columnBegin - _columnBegin) * _cellSize.width;
    return &til::at(_pixelBuffer, pixelOffset);
}

void ImageSlice::CopyBlock(const TextBuffer& srcBuffer, const til::rect srcRect, TextBuffer& dstBuffer, const til::rect dstRect)
{
    // If the top of the source is less than the top of the destination, we copy
    // the rows from the bottom upwards, to avoid the possibility of the source
    // being overwritten if it were to overlap the destination range.
    if (srcRect.top < dstRect.top)
    {
        for (auto y = srcRect.height(); y-- > 0;)
        {
            const auto& srcRow = srcBuffer.GetRowByOffset(srcRect.top + y);
            auto& dstRow = dstBuffer.GetMutableRowByOffset(dstRect.top + y);
            CopyCells(srcRow, srcRect.left, dstRow, dstRect.left, dstRect.right);
        }
    }
    else
    {
        for (auto y = 0; y < srcRect.height(); y++)
        {
            const auto& srcRow = srcBuffer.GetRowByOffset(srcRect.top + y);
            auto& dstRow = dstBuffer.GetMutableRowByOffset(dstRect.top + y);
            CopyCells(srcRow, srcRect.left, dstRow, dstRect.left, dstRect.right);
        }
    }
}

void ImageSlice::CopyRow(const ROW& srcRow, ROW& dstRow)
{
    const auto& srcSlice = srcRow.GetImageSlice();
    auto& dstSlice = dstRow.GetMutableImageSlice();
    dstSlice = srcSlice ? std::make_unique<ImageSlice>(*srcSlice) : nullptr;
}

void ImageSlice::CopyCells(const ROW& srcRow, const til::CoordType srcColumn, ROW& dstRow, const til::CoordType dstColumnBegin, const til::CoordType dstColumnEnd)
{
    // If there's no image content in the source row, we're essentially copying
    // a blank image into the destination, which is the same thing as an erase.
    // Also if the line renditions are different, there's no meaningful way to
    // copy the image content, so we also just treat that as an erase.
    const auto& srcSlice = srcRow.GetImageSlice();
    if (!srcSlice || srcRow.GetLineRendition() != dstRow.GetLineRendition()) [[likely]]
    {
        ImageSlice::EraseCells(dstRow, dstColumnBegin, dstColumnEnd);
    }
    else
    {
        auto& dstSlice = dstRow.GetMutableImageSlice();
        if (!dstSlice)
        {
            dstSlice = std::make_unique<ImageSlice>(srcSlice->CellSize());
        }
        const auto scale = srcRow.GetLineRendition() != LineRendition::SingleWidth ? 1 : 0;
        if (dstSlice->_copyCells(*srcSlice, srcColumn << scale, dstColumnBegin << scale, dstColumnEnd << scale))
        {
            // If _copyCells returns true, that means the destination was
            // completely erased, so we can delete this slice.
            dstSlice = nullptr;
        }
    }
}

bool ImageSlice::_copyCells(const ImageSlice& srcSlice, const til::CoordType srcColumn, const til::CoordType dstColumnBegin, const til::CoordType dstColumnEnd)
{
    const auto srcColumnEnd = srcColumn + dstColumnEnd - dstColumnBegin;

    // First we determine the portions of the copy range that are currently in use.
    const auto srcUsedBegin = std::max(srcColumn, srcSlice._columnBegin);
    const auto srcUsedEnd = std::max(std::min(srcColumnEnd, srcSlice._columnEnd), srcUsedBegin);
    const auto dstUsedBegin = std::max(dstColumnBegin, _columnBegin);
    const auto dstUsedEnd = std::max(std::min(dstColumnEnd, _columnEnd), dstUsedBegin);

    // The used source projected into the destination is the range we must overwrite.
    const auto projectedOffset = dstColumnBegin - srcColumn;
    const auto dstWriteBegin = srcUsedBegin + projectedOffset;
    const auto dstWriteEnd = srcUsedEnd + projectedOffset;

    if (dstWriteBegin < dstWriteEnd)
    {
        auto dstIterator = MutablePixels(dstWriteBegin, dstWriteEnd);
        auto srcIterator = srcSlice.Pixels(srcUsedBegin);
        const auto writeCellCount = dstWriteEnd - dstWriteBegin;
        const auto writeByteCount = sizeof(RGBQUAD) * writeCellCount * _cellSize.width;
        for (auto y = 0; y < _cellSize.height; y++)
        {
            std::memmove(dstIterator, srcIterator, writeByteCount);
            std::advance(srcIterator, srcSlice._pixelWidth);
            std::advance(dstIterator, _pixelWidth);
        }
    }

    // The used destination before and after the written area must be erased.
    if (dstUsedBegin < dstWriteBegin)
    {
        _eraseCells(dstUsedBegin, dstWriteBegin);
    }
    if (dstUsedEnd > dstWriteEnd)
    {
        _eraseCells(dstWriteEnd, dstUsedEnd);
    }

    // If the beginning column is now not less than the end, that means the
    // content has been entirely erased, so we return true to let the caller
    // know that the slice should be deleted.
    return _columnBegin >= _columnEnd;
}

void ImageSlice::EraseBlock(TextBuffer& buffer, const til::rect rect)
{
    for (auto y = rect.top; y < rect.bottom; y++)
    {
        auto& row = buffer.GetMutableRowByOffset(y);
        EraseCells(row, rect.left, rect.right);
    }
}

void ImageSlice::EraseCells(ROW& row, const til::CoordType columnBegin, const til::CoordType columnEnd)
{
    auto& imageSlice = row.GetMutableImageSlice();
    if (imageSlice) [[unlikely]]
    {
        const auto scale = row.GetLineRendition() != LineRendition::SingleWidth ? 1 : 0;
        if (imageSlice->_eraseCells(columnBegin << scale, columnEnd << scale))
        {
            // If _eraseCells returns true, that means the image was
            // completely erased, so we can delete this slice.
            imageSlice = nullptr;
        }
    }
}

bool ImageSlice::_eraseCells(const til::CoordType columnBegin, const til::CoordType columnEnd)
{
    if (columnBegin <= _columnBegin && columnEnd >= _columnEnd)
    {
        // If we're erasing the entire range that's in use, we return true to
        // indicate that there is now nothing left. We don't bother altering
        // the buffer because the caller is now expected to delete this slice.
        return true;
    }
    else
    {
        const auto eraseBegin = std::max(columnBegin, _columnBegin);
        const auto eraseEnd = std::min(columnEnd, _columnEnd);
        if (eraseBegin < eraseEnd)
        {
            const auto eraseOffset = (eraseBegin - _columnBegin) * _cellSize.width;
            const auto eraseLength = (eraseEnd - eraseBegin) * _cellSize.width;
            auto eraseIterator = _pixelBuffer.begin() + eraseOffset;
            for (auto y = 0; y < _cellSize.height; y++)
            {
                std::fill_n(eraseIterator, eraseLength, RGBQUAD{});
                std::advance(eraseIterator, _pixelWidth);
            }
        }
        return false;
    }
}
