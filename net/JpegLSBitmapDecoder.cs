//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use.
//

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics.Contracts;
using System.IO;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace CharLS
{
    /// <summary>
    /// Provides a method to create a BitmapSource from a JPEG-LS encoded bitmap stream.
    /// The .NET Imaging namespace provides a set of build-in BitmapDecoders. This set of classes is however not extensible and
    /// by design the base class BitmapDecoder is not derivable by external classes.
    /// This static class can be used in the same pattern, but is only provides the primary functionality to convert a stream into a BitmapSource.
    /// </summary>
    public sealed class JpegLSBitmapDecoder
    {
        private readonly ReadOnlyCollection<BitmapFrame> frames;

        /// <summary>
        /// Initializes a new instance of the <see cref="JpegLSBitmapDecoder"/> class.
        /// </summary>
        /// <param name="bitmapStream">The bitmap stream to decode.</param>
        /// <exception cref="FileFormatException">The bitmap stream is not a JPEG-LS encoded image.</exception>
        /// <exception cref="DllNotFoundException">The native DLL required to decode the JPEG-LS bit stream could not be loaded.</exception>
        public JpegLSBitmapDecoder(Stream bitmapStream)
        {
            if (bitmapStream == null)
                throw new ArgumentNullException(nameof(bitmapStream));
            Contract.EndContractBlock();

            try
            {
                var buffer = new byte[bitmapStream.Length];
                bitmapStream.Read(buffer, 0, buffer.Length);

                var info = JpegLSCodec.GetMetadataInfo(buffer);

                var pixels = new byte[info.UncompressedSize];
                JpegLSCodec.Decompress(buffer, buffer.Length, pixels);

                var pixelFormat = GetPixelFormat(info);
                int bytesPerPixel = pixelFormat.BitsPerPixel / 8;
                int stride = bytesPerPixel * info.Width;

                var source = BitmapSource.Create(
                    info.Width,
                    info.Height,
                    96,
                    96,
                    pixelFormat,
                    null,
                    pixels,
                    stride);
                var frame = BitmapFrame.Create(source);
                frames = new ReadOnlyCollection<BitmapFrame>(new[] { frame });
            }
            catch (Exception e) when (e is IOException || e is InvalidDataException)
            {
                throw new FileFormatException(e.Message, e);
            }
        }

        /// <summary>
        /// Gets the content of an individual frame within a bitmap.
        /// </summary>
        /// <value>The frames.</value>
        public IList<BitmapFrame> Frames => frames;

        private static PixelFormat GetPixelFormat(JpegLSMetadataInfo info)
        {
            switch (info.ComponentCount)
            {
                case 1:
                    if (info.BitsPerComponent == 8)
                    {
                        return PixelFormats.Gray8;
                    }

                    break;

                case 3:
                    if (info.BitsPerComponent == 8)
                    {
                        return PixelFormats.Rgb24;
                    }

                    break;

                default:
                    throw new NotSupportedException();
            }

            throw new NotSupportedException();
        }
    }
}
