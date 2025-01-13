/*
 * Copyright (c) 2018-2024, Andreas Kling <andreas@ladybird.org>
 * Copyright (c) 2022, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Vector.h>
#include <LibGfx/ImageFormats/ExifOrientedBitmap.h>
#include <LibGfx/ImageFormats/PNGLoader.h>
#include <LibGfx/ImageFormats/TIFFLoader.h>
#include <LibGfx/ImageFormats/TIFFMetadata.h>
#include <LibGfx/ImmutableBitmap.h>
#include <LibGfx/Painter.h>

#include <LibGfx/ImageFormats/GeneratedWrapper.h>

namespace Gfx {

struct PNGLoadingContext {
    ReadonlyBytes data;

    IntSize size;

    // FIXME: Support APNG
    u32 frame_count { 1 };
    u32 loop_count { 0 };

    Vector<ImageFrameDescriptor> frame_descriptors;

    Optional<ByteBuffer> icc_profile;
    OwnPtr<ExifMetadata> exif_metadata;
};

ErrorOr<NonnullOwnPtr<ImageDecoderPlugin>> PNGImageDecoderPlugin::create(ReadonlyBytes bytes)
{
    auto decoder = adopt_own(*new PNGImageDecoderPlugin(bytes));
    TRY(decoder->initialize());
    return decoder;
}

PNGImageDecoderPlugin::PNGImageDecoderPlugin(ReadonlyBytes data)
    : m_context(adopt_own(*new PNGLoadingContext))
{
    m_context->data = data;
}

size_t PNGImageDecoderPlugin::first_animated_frame_index()
{
    return 0;
}

IntSize PNGImageDecoderPlugin::size()
{
    return m_context->size;
}

bool PNGImageDecoderPlugin::is_animated()
{
    return m_context->frame_count > 1;
}

size_t PNGImageDecoderPlugin::loop_count()
{
    return m_context->loop_count;
}

size_t PNGImageDecoderPlugin::frame_count()
{
    return m_context->frame_count;
}

ErrorOr<ImageFrameDescriptor> PNGImageDecoderPlugin::frame(size_t index, Optional<IntSize>)
{
    if (index >= m_context->frame_count)
        return Error::from_errno(EINVAL);

    return m_context->frame_descriptors[index];
}

ErrorOr<Optional<ReadonlyBytes>> PNGImageDecoderPlugin::icc_data()
{
    if (m_context->icc_profile.has_value())
        return Optional<ReadonlyBytes>(*m_context->icc_profile);
    return OptionalNone {};
}

ErrorOr<void> PNGImageDecoderPlugin::initialize()
{
    auto slice = rust::std::slice::from_raw_parts(m_context->data.data(), m_context->data.size());
    auto cursor = rust::std::io::Cursor<decltype(slice)>::new_(slice);

    auto maybe_reader = rust::image::ImageReader<decltype(cursor)>::new_(move(cursor)).with_guessed_format();
    if (maybe_reader.is_err())
        return Error::from_string_literal("Unable to create decoder from image");

    auto reader = maybe_reader.ok().unwrap();
    auto maybe_decoded = reader.decode();
    if (maybe_decoded.is_err())
        return Error::from_string_literal("Error while decoding image.");

    auto decoded = maybe_decoded.ok().unwrap();
    m_context->size = { decoded.width(), decoded.height() };

    auto raw_data = decoded.to_rgba8().into_raw();
    auto bitmap_from_rust = TRY(Gfx::Bitmap::create_wrapper(Gfx::BitmapFormat::RGBA8888, Gfx::AlphaType::Unpremultiplied, size(), size().width() * 4, raw_data.as_mut_ptr()));
    // FIXME: Avoid copy
    auto bitmap = TRY(bitmap_from_rust->clone());

    m_context->frame_descriptors.empend(move(bitmap), 0);
    return {};
}

PNGImageDecoderPlugin::~PNGImageDecoderPlugin() = default;

bool PNGImageDecoderPlugin::sniff(ReadonlyBytes data)
{
    Array<u8, 8> png_signature { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };
    if (data.size() < png_signature.size())
        return false;
    return data.slice(0, png_signature.size()) == ReadonlyBytes(png_signature.data(), png_signature.size());
}

Optional<Metadata const&> PNGImageDecoderPlugin::metadata()
{
    if (m_context->exif_metadata)
        return *m_context->exif_metadata;
    return OptionalNone {};
}

}
