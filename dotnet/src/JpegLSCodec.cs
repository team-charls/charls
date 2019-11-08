// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;

namespace CharLS
{
    /// <summary>
    /// Provides methods and for compressing and decompressing arrays using the JPEG-LS algorithm.
    /// </summary>
    /// <remarks>
    /// This class represents the JPEG-LS algorithm, an industry standard algorithm for lossless and near-lossless
    /// image data compression and decompression.
    /// </remarks>
    public static class JpegLSCodec
    {
        // Design notes:
        //  - The words compress/decompress are used as these are the terms used by the .NET BCLs (System.IO.Compression namespace)
        //    The CharLS C API uses the terms encode/decode.
        //  - The input/output buffers parameters are using the common .NET order, which is different the CharLS C API.

        /// <summary>
        /// Compresses the specified image passed in the source pixel buffer.
        /// </summary>
        /// <param name="info">The meta info that describes the format and type of the pixels.</param>
        /// <param name="pixels">An array of bytes that represents the content of a bitmap image.</param>
        /// <param name="jfifHeader">if set to <c>true</c> a JFIF header will be added to the encoded byte stream.</param>
        /// <returns>An arraySegment with a reference to the byte array with the compressed data in the JPEG-LS format.</returns>
        /// <exception cref="ArgumentNullException">info -or- pixels is null.</exception>
        /// <exception cref="ArgumentOutOfRangeException">info.Width -or- info.Height contains an invalid value.</exception>
        /// <exception cref="InvalidDataException">The compressed output doesn't fit into the maximum defined output buffer.</exception>
        public static ArraySegment<byte> Compress(JpegLSMetadataInfo info, byte[] pixels, bool jfifHeader = false)
        {
            if (pixels == null)
                throw new ArgumentNullException(nameof(pixels));

            var pixelCount = pixels.Length;
            return Compress(info, pixels, pixelCount, jfifHeader);
        }

        /// <summary>
        /// Compresses the specified image passed in the source pixel buffer.
        /// </summary>
        /// <param name="info">The meta info that describes the format and type of the pixels.</param>
        /// <param name="pixels">An array of bytes that represents the content of a bitmap image.</param>
        /// <param name="pixelCount">The number of pixel in the pixel array.</param>
        /// <param name="jfifHeader">if set to <c>true</c> a JFIF header will be added to the encoded byte stream.</param>
        /// <returns>An arraySegment with a reference to the byte array with the compressed data in the JPEG-LS format.</returns>
        /// <exception cref="ArgumentNullException">info -or- pixels is null.</exception>
        /// <exception cref="ArgumentOutOfRangeException">info.Width -or- info.Height -or- pixelCount contains an invalid value.</exception>
        /// <exception cref="InvalidDataException">The compressed output doesn't fit into the maximum defined output buffer.</exception>
        public static ArraySegment<byte> Compress(JpegLSMetadataInfo info, byte[] pixels, int pixelCount, bool jfifHeader = false)
        {
            if (info == null)
                throw new ArgumentNullException(nameof(info));
            if (pixels == null)
                throw new ArgumentNullException(nameof(pixels));
            if (pixelCount <= 0 || pixelCount > pixels.Length)
                throw new ArgumentOutOfRangeException(nameof(pixelCount), "pixelCount <= 0 || pixelCount > pixels.Length");

            const int JpegLSHeaderLength = 100;

            // Assume compressed size <= uncompressed size (covers 99% of the cases).
            var buffer = new byte[pixels.Length + JpegLSHeaderLength];

            if (!TryCompress(info, pixels, pixels.Length, jfifHeader, buffer, buffer.Length, out var compressedCount))
            {
                // Increase output buffer to hold compressed data.
                buffer = new byte[(int)(pixels.Length * 1.5) + JpegLSHeaderLength];

                if (!TryCompress(info, pixels, pixels.Length, jfifHeader, buffer, buffer.Length, out compressedCount))
                    throw new InvalidDataException("Compression failed: compressed output larger then 1.5 * input.");
            }

            return new ArraySegment<byte>(buffer, 0, compressedCount);
        }

        /// <summary>
        /// Tries the compress the array with pixels into the provided buffer.
        /// </summary>
        /// <param name="info">The meta info that describes the format and type of the pixels.</param>
        /// <param name="pixels">An array of bytes that represents the content of a bitmap image.</param>
        /// <param name="pixelCount">The number of pixel in the pixel array.</param>
        /// <param name="jfifHeader">if set to <c>true</c> a JFIF header will be added to the encoded byte stream.</param>
        /// <param name="destination">The destination buffer that will hold the JPEG-LS compressed (encoded) bit stream.</param>
        /// <param name="destinationLength">Length of the destination buffer that can be used (can be less then the length of the destination array).</param>
        /// <param name="compressedCount">The number of bytes that have been compressed (encoded) into the destination array.</param>
        /// <returns><c>true</c> when the compressed bit stream fits into the destination array, otherwise <c>false</c>.</returns>
        /// <exception cref="ArgumentNullException">info -or- pixels is null.</exception>
        /// <exception cref="ArgumentOutOfRangeException">info.Width -or- info.Height -or- pixelCount -or- destinationLength contains an invalid value.</exception>
        public static bool TryCompress(JpegLSMetadataInfo info, byte[] pixels, int pixelCount, bool jfifHeader, byte[] destination, int destinationLength, out int compressedCount)
        {
            if (info == null)
                throw new ArgumentNullException(nameof(info));
            if (pixels == null)
                throw new ArgumentNullException(nameof(pixels));
            if (pixelCount <= 0 || pixelCount > pixels.Length)
                throw new ArgumentOutOfRangeException(nameof(pixelCount), "pixelCount <= 0 || pixelCount > pixels.Length");
            if (destination == null)
                throw new ArgumentNullException(nameof(destination));
            if (destinationLength <= 0 || destinationLength > destination.Length)
                throw new ArgumentOutOfRangeException(nameof(destinationLength), "destination <= 0 || destinationCount > destination.Length");

            var parameters = default(JlsParameters);
            info.CopyTo(ref parameters);
            if (jfifHeader)
            {
                parameters.Jfif.Version = (1 << 8) + 2; // JFIF version 1.02
                parameters.Jfif.Units = 0; // No units, aspect ratio only specified
                parameters.Jfif.DensityX = 1; // use standard 1:1 aspect ratio. (density should always be set to non-zero values).
                parameters.Jfif.DensityY = 1;
            }

            JpegLSError result;
            if (Environment.Is64BitProcess)
            {
                result = SafeNativeMethods.JpegLsEncodeX64(destination, destinationLength, out var count, pixels, pixelCount, ref parameters, IntPtr.Zero);
                compressedCount = (int)count;
            }
            else
            {
                result = SafeNativeMethods.JpegLsEncodeX86(destination, destinationLength, out compressedCount, pixels, pixelCount, ref parameters, IntPtr.Zero);
            }

            if (result == JpegLSError.SourceBufferTooSmall)
                return false;

            HandleResult(result);
            return true;
        }

        /// <summary>
        /// Gets the metadata info as stored in a JPEG-LS compressed bit stream.
        /// </summary>
        /// <param name="source">The JPEG-LS compressed bit stream.</param>
        /// <returns>An JpegLSMetadataInfo instance.</returns>
        /// <exception cref="ArgumentNullException">source is null.</exception>
        /// <exception cref="InvalidDataException">Thrown when the source array contains invalid compressed data.</exception>
        public static JpegLSMetadataInfo GetMetadataInfo(byte[] source)
        {
            if (source == null)
                throw new ArgumentNullException(nameof(source));

            return GetMetadataInfo(source, source.Length);
        }

        /// <summary>
        /// Gets the metadata info as stored in a JPEG-LS compressed bit stream.
        /// </summary>
        /// <param name="source">The JPEG-LS compressed bit stream.</param>
        /// <param name="count">The count of bytes that are valid in the array.</param>
        /// <returns>An JpegLSMetadataInfo instance.</returns>
        /// <exception cref="ArgumentNullException">source is null.</exception>
        /// <exception cref="ArgumentOutOfRangeException">count contains an invalid value.</exception>
        /// <exception cref="InvalidDataException">Thrown when the source array contains invalid compressed data.</exception>
        public static JpegLSMetadataInfo GetMetadataInfo(byte[] source, int count)
        {
            if (source == null)
                throw new ArgumentNullException(nameof(source));
            if (count < 0 || count > source.Length)
                throw new ArgumentOutOfRangeException(nameof(count), "count < 0 || count > source.Length");

            JpegLsReadHeaderThrowWhenError(source, count, out var info);
            return new JpegLSMetadataInfo(ref info);
        }

        /// <summary>
        /// Decompresses the JPEG-LS encoded data passed in the source byte array.
        /// </summary>
        /// <param name="source">The byte array that contains the JPEG-LS encoded data to decompress.</param>
        /// <returns>A byte array with the pixel data.</returns>
        /// <exception cref="ArgumentNullException">source is null.</exception>
        /// <exception cref="InvalidDataException">Thrown when the source array contains invalid compressed data.</exception>
        public static byte[] Decompress(byte[] source)
        {
            if (source == null)
                throw new ArgumentNullException(nameof(source));

            return Decompress(source, source.Length);
        }

        /// <summary>
        /// Decompresses the JPEG-LS encoded data passed in the source byte array.
        /// </summary>
        /// <param name="source">The byte array that contains the JPEG-LS encoded data to decompress.</param>
        /// <param name="count">The number of bytes of the array to decompress.</param>
        /// <returns>A byte array with the pixel data.</returns>
        /// <exception cref="ArgumentNullException">source is null.</exception>
        /// <exception cref="ArgumentOutOfRangeException">count contains an invalid value.</exception>
        /// <exception cref="InvalidDataException">Thrown when the source array contains invalid compressed data.</exception>
        public static byte[] Decompress(byte[] source, int count)
        {
            if (source == null)
                throw new ArgumentNullException(nameof(source));
            if (count < 0 || count > source.Length)
                throw new ArgumentOutOfRangeException(nameof(count), "count < 0 || count > source.Length");

            JpegLsReadHeaderThrowWhenError(source, count, out var info);

            var destination = new byte[GetUncompressedSize(ref info)];
            Decompress(source, count, destination);
            return destination;
        }

        /// <summary>
        /// Decompresses the JPEG-LS encoded data passed in the source byte array into the destination array.
        /// </summary>
        /// <param name="source">The byte array that contains the JPEG-LS encoded data to decompress.</param>
        /// <param name="count">The number of bytes of the array to decompress.</param>
        /// <param name="pixels">The byte array that will hold the pixels when the function returns.</param>
        /// <exception cref="ArgumentNullException">source -or- pixels is null.</exception>
        /// <exception cref="ArgumentOutOfRangeException">count contains an invalid value.</exception>
        /// <exception cref="ArgumentException">Thrown when the destination array is too small to hold the decompressed pixel data.</exception>
        /// <exception cref="InvalidDataException">Thrown when the source array contains an invalid encoded JPEG-LS bit stream.</exception>
        public static void Decompress(byte[] source, int count, byte[] pixels)
        {
            if (source == null)
                throw new ArgumentNullException(nameof(source));
            if (count < 0 || count > source.Length)
                throw new ArgumentOutOfRangeException(nameof(count), "count < 0 || count > source.Length");
            if (pixels == null)
                throw new ArgumentNullException(nameof(pixels));

            var error = Environment.Is64BitProcess ?
                SafeNativeMethods.JpegLsDecodeX64(pixels, pixels.Length, source, count, IntPtr.Zero, IntPtr.Zero) :
                SafeNativeMethods.JpegLsDecodeX86(pixels, pixels.Length, source, count, IntPtr.Zero, IntPtr.Zero);
            HandleResult(error);
        }

        private static void JpegLsReadHeaderThrowWhenError(byte[] source, int length, out JlsParameters info)
        {
            var result = Environment.Is64BitProcess ?
                SafeNativeMethods.JpegLsReadHeaderX64(source, length, out info, IntPtr.Zero) :
                SafeNativeMethods.JpegLsReadHeaderX86(source, length, out info, IntPtr.Zero);
            HandleResult(result);
        }

        private static int GetUncompressedSize(ref JlsParameters info)
        {
            var size = info.Width * info.Height * info.Components * ((info.BitsPerSample + 7) / 8);
            return size;
        }

        private static void HandleResult(JpegLSError result)
        {
            Exception exception;

            switch (result)
            {
                case JpegLSError.None:
                    return;

                case JpegLSError.TooMuchEncodedData:
                case JpegLSError.ParameterValueNotSupported:
                case JpegLSError.InvalidEncodedData:
                case JpegLSError.SourceBufferTooSmall:
                case JpegLSError.BitDepthForTransformNotSupported:
                case JpegLSError.ColorTransformNotSupported:
                case JpegLSError.EncodingNotSupported:
                case JpegLSError.UnknownJpegMarkerFound:
                case JpegLSError.JpegMarkerStartByteNotFound:
                case JpegLSError.StartOfImageMarkerNotFound:
                case JpegLSError.StartOfFrameMarkerNotFound:
                case JpegLSError.InvalidMarkerSegmentSize:
                case JpegLSError.DuplicateStartOfImageMarker:
                case JpegLSError.DuplicateStartOfFrameMarker:
                case JpegLSError.DuplicateComponentIdInStartOfFrameSegment:
                case JpegLSError.UnexpectedEndOfImageMarker:
                case JpegLSError.InvalidJpeglsPresetParameterType:
                case JpegLSError.JpeglsPresetExtendedParameterTypeNotSupported:
                case JpegLSError.InvalidParameterWidth:
                case JpegLSError.InvalidParameterHeight:
                case JpegLSError.InvalidParameterComponentCount:
                case JpegLSError.InvalidParameterBitsPerSample:
                case JpegLSError.InvalidParameterInterleaveMode:
                    exception = new InvalidDataException(GetErrorMessage(result));
                    break;

                case JpegLSError.InvalidArgument:
                case JpegLSError.DestinationBufferTooSmall:
                case JpegLSError.InvalidArgumentWidth:
                case JpegLSError.InvalidArgumentHeight:
                case JpegLSError.InvalidArgumentComponentCount:
                case JpegLSError.InvalidArgumentBitsPerSample:
                case JpegLSError.InvalidArgumentInterleaveMode:
                case JpegLSError.InvalidArgumentDestination:
                case JpegLSError.InvalidArgumentSource:
                case JpegLSError.InvalidArgumentThumbnail:
                    exception = new ArgumentException(GetErrorMessage(result));
                    break;

                case JpegLSError.UnexpectedFailure:
                    exception = new InvalidOperationException(GetErrorMessage(result));
                    break;

                default:
                    Debug.Assert(false, "C# and native implementation mismatch");

                    // ReSharper disable once HeuristicUnreachableCode
                    exception = new InvalidOperationException(GetErrorMessage(result));
                    break;
            }

            var data = exception.Data;

            // ReSharper disable once PossibleNullReferenceException
            data.Add(nameof(JpegLSError), result);
            throw exception;
        }

        private static string GetErrorMessage(JpegLSError result)
        {
            var message = Environment.Is64BitProcess ?
                SafeNativeMethods.CharLSGetErrorMessageX64((int)result) :
                SafeNativeMethods.CharLSGetErrorMessageX86((int)result);
            return Marshal.PtrToStringAnsi(message);
        }
    }
}
