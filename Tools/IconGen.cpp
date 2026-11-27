/*
    Draws the voxline mark at every Windows icon size and packs it into an ICO.
    The mark lives in Source/ui/Logo.h, so the standalone's shell icon is the
    same drawing as the logo in the editor header.

    Usage: VoxlineIcon [output.ico] [preview.png]
*/

#include <juce_gui_basics/juce_gui_basics.h>

#include <algorithm>
#include <cstdio>
#include <vector>

#include "ui/Logo.h"

namespace
{

constexpr int iconSizes[] = { 256, 128, 64, 48, 32, 24, 20, 16 };

/** Box-filtered downscale. JUCE's platform resampler needs a GUI device and
    returned blank images in this console tool, so the averaging is explicit. */
juce::Image downsample (const juce::Image& source, int size)
{
    const int factor = source.getWidth() / size;
    const float samples = (float) (factor * factor);

    juce::Image result (juce::Image::ARGB, size, size, true, juce::SoftwareImageType());

    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            float a = 0.0f, r = 0.0f, g = 0.0f, b = 0.0f;

            for (int sy = 0; sy < factor; ++sy)
            {
                for (int sx = 0; sx < factor; ++sx)
                {
                    const auto c = source.getPixelAt (x * factor + sx, y * factor + sy);
                    const float alpha = c.getFloatAlpha();

                    a += alpha;
                    r += c.getFloatRed() * alpha;
                    g += c.getFloatGreen() * alpha;
                    b += c.getFloatBlue() * alpha;
                }
            }

            a /= samples;
            r /= samples;
            g /= samples;
            b /= samples;

            const auto colour = a > 0.0f
                                    ? juce::Colour::fromFloatRGBA (r / a, g / a, b / a, a)
                                    : juce::Colours::transparentBlack;

            result.setPixelAt (x, y, colour);
        }
    }

    return result;
}

juce::Image renderIcon (int size)
{
    // Draw large and average down: the extra resolution keeps the strokes and
    // the wave crisp once the shell scales the icon to 16 px.
    constexpr int supersample = 4;

    // The software type is explicit: the native backend has no device to draw
    // into in this console tool.
    juce::Image canvas (juce::Image::ARGB, size * supersample, size * supersample, true,
                        juce::SoftwareImageType());
    juce::Graphics g (canvas);

    auto tile = juce::Rectangle<float> ((float) size * supersample, (float) size * supersample)
                    .reduced (std::max (1.0f, (float) size * supersample * 0.03f));

    vx::ui::drawLogoMark (g, tile);

    return downsample (canvas, size);
}

void writeLe16 (juce::OutputStream& out, juce::uint32 value)
{
    out.writeByte ((char) (value & 0xff));
    out.writeByte ((char) ((value >> 8) & 0xff));
}

void writeLe32 (juce::OutputStream& out, juce::uint32 value)
{
    writeLe16 (out, value & 0xffff);
    writeLe16 (out, (value >> 16) & 0xffff);
}

/** A 32-bit bottom-up DIB, the classic ICO payload for the smaller sizes. */
juce::MemoryBlock makeDib (const juce::Image& image)
{
    const int w = image.getWidth();
    const int h = image.getHeight();
    const int maskStride = ((w + 31) / 32) * 4;
    const int xorBytes = w * h * 4;
    const int andBytes = maskStride * h;

    juce::MemoryBlock dib ((size_t) (40 + xorBytes + andBytes), true);
    auto* bytes = static_cast<juce::uint8*> (dib.getData());

    auto put16 = [bytes] (int offset, juce::uint32 v)
    {
        bytes[offset] = (juce::uint8) (v & 0xff);
        bytes[offset + 1] = (juce::uint8) ((v >> 8) & 0xff);
    };
    auto put32 = [&put16] (int offset, juce::uint32 v)
    {
        put16 (offset, v & 0xffff);
        put16 (offset + 2, (v >> 16) & 0xffff);
    };

    put32 (0, 40);                      // header size
    put32 (4, (juce::uint32) w);        // width
    put32 (8, (juce::uint32) (h * 2));  // height: XOR image plus AND mask
    put16 (12, 1);                      // planes
    put16 (14, 32);                     // bits per pixel
    put32 (16, 0);                      // BI_RGB
    put32 (20, (juce::uint32) (xorBytes + andBytes));

    auto* xorData = bytes + 40;
    auto* andData = xorData + xorBytes;

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            const auto colour = image.getPixelAt (x, h - 1 - y); // ICO rows run bottom-up
            auto* pixel = xorData + ((size_t) y * (size_t) w + (size_t) x) * 4;

            pixel[0] = colour.getBlue();
            pixel[1] = colour.getGreen();
            pixel[2] = colour.getRed();
            pixel[3] = colour.getAlpha();

            if (colour.getAlpha() < 8)
                andData[(size_t) y * (size_t) maskStride + (size_t) (x / 8)]
                    |= (juce::uint8) (0x80 >> (x % 8));
        }
    }

    return dib;
}

juce::MemoryBlock makePng (const juce::Image& image)
{
    juce::MemoryBlock png;
    juce::MemoryOutputStream stream (png, false);
    juce::PNGImageFormat().writeImageToStream (image, stream);
    return png;
}

struct IconEntry
{
    int size;
    juce::MemoryBlock data;
};

bool writeIco (const juce::File& file, const std::vector<IconEntry>& entries)
{
    juce::MemoryBlock ico;
    juce::MemoryOutputStream stream (ico, false);

    writeLe16 (stream, 0); // reserved
    writeLe16 (stream, 1); // type: icon
    writeLe16 (stream, (juce::uint32) entries.size());

    juce::uint32 offset = 6 + 16 * (juce::uint32) entries.size();

    for (const auto& entry : entries)
    {
        stream.writeByte ((char) (entry.size >= 256 ? 0 : entry.size));
        stream.writeByte ((char) (entry.size >= 256 ? 0 : entry.size));
        stream.writeByte (0); // palette colours
        stream.writeByte (0); // reserved
        writeLe16 (stream, 1); // planes
        writeLe16 (stream, 32); // bits per pixel
        writeLe32 (stream, (juce::uint32) entry.data.getSize());
        writeLe32 (stream, offset);

        offset += (juce::uint32) entry.data.getSize();
    }

    for (const auto& entry : entries)
        stream.write (entry.data.getData(), entry.data.getSize());

    stream.flush();

    if (! file.getParentDirectory().createDirectory())
        return false;

    return file.replaceWithData (ico.getData(), ico.getSize());
}

bool writePng (const juce::File& file, const juce::Image& image)
{
    juce::MemoryBlock png;
    juce::MemoryOutputStream stream (png, false);
    juce::PNGImageFormat().writeImageToStream (image, stream);

    if (! file.getParentDirectory().createDirectory())
        return false;

    return file.replaceWithData (png.getData(), png.getSize());
}

} // namespace

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    const juce::File cwd = juce::File::getCurrentWorkingDirectory();
    const juce::File icoFile = cwd.getChildFile (argc > 1 ? juce::String (argv[1])
                                                           : juce::String ("Source/platform/voxline.ico"));

    std::vector<IconEntry> entries;

    for (int size : iconSizes)
    {
        auto image = renderIcon (size);

        // 256 px icons are PNG-compressed, as Windows expects; the rest are DIBs.
        entries.push_back ({ size, size >= 256 ? makePng (image) : makeDib (image) });
    }

    if (! writeIco (icoFile, entries))
    {
        std::fprintf (stderr, "Could not write %s\n", icoFile.getFullPathName().toRawUTF8());
        return 1;
    }

    std::printf ("Wrote %s (%d sizes, 16-256 px)\n",
                 icoFile.getFullPathName().toRawUTF8(), (int) entries.size());

    if (argc > 2)
    {
        const juce::File previewFile = cwd.getChildFile (juce::String (argv[2]));

        if (! writePng (previewFile, renderIcon (256)))
        {
            std::fprintf (stderr, "Could not write %s\n", previewFile.getFullPathName().toRawUTF8());
            return 1;
        }

        std::printf ("Wrote %s\n", previewFile.getFullPathName().toRawUTF8());
    }

    return 0;
}
