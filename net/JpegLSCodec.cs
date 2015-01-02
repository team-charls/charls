//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use. 
//

using System;
using System.Diagnostics.Contracts;
using System.Globalization;
using System.IO;

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
        /* Design notes:
           - The words compress/decompress are used as these are the terms used by the .NET BCLs (System.IO.Compression namespace)
             The CharLS C API uses the terms encode/decode.
           - The input/output buffers parameters are using the common .NET order, which is different the CharLS C API.
        */

        /// <summary>
        /// Compresses the specified image passed in the source pixel buffer.
        /// </summary>
        /// <param name="info">The meta info that describes the format and type of the pixels.</param>
        /// <param name="pixels">An array of bytes that represents the content of a bitmap image.</param>
        /// <param name="jfifHeader">if set to <c>true</c> a JFIF header will be added to the encoded byte stream.</param>
        /// <returns>An arraySegment with a reference to the byte array with the compressed data in the JPEG-LS format.</returns>
        /// <exception cref="InternalBufferOverflowException">The compressed output doesn't fit into the maximum defined output buffer.</exception>
        public static ArraySegment<byte> Compress(JpegLSMetadataInfo info, byte[] pixels, bool jfifHeader = false)
        {
            Contract.Requires<ArgumentNullException>(info != null);
            Contract.Requires<ArgumentException>(info.Width > 0 && info.Width <= 65535);
            Contract.Requires<ArgumentException>(info.Height > 0 && info.Height <= 65535);
            Contract.Requires<ArgumentNullException>(pixels != null);

            var pixelCount = pixels.Length;
            Contract.Assume(pixelCount > 0 && pixelCount <= pixels.Length);
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
        /// <exception cref="InternalBufferOverflowException">The compressed output doesn't fit into the maximum defined output buffer.</exception>
        public static ArraySegment<byte> Compress(JpegLSMetadataInfo info, byte[] pixels, int pixelCount, bool jfifHeader)
        {
            Contract.Requires<ArgumentNullException>(info != null);
            Contract.Requires<ArgumentException>(info.Width > 0 && info.Width <= 65535);
            Contract.Requires<ArgumentException>(info.Height > 0 && info.Height <= 65535);
            Contract.Requires<ArgumentNullException>(pixels != null);
            Contract.Requires<ArgumentNullException>(pixelCount > 0 && pixelCount <= pixels.Length);

            const int JpegLSHeaderLength = 100;

            // Assume compressed size <= uncompressed size (covers 99% of the cases).
            var buffer = new byte[pixels.Length + JpegLSHeaderLength];
            int compressedCount;

            if (!TryCompress(info, pixels, pixels.Length, jfifHeader, buffer, buffer.Length, out compressedCount))
            {
                // Increase output buffer to hold compressed data.
                buffer = new byte[(int)(pixels.Length * 1.5) + JpegLSHeaderLength];

                Contract.Assume(info.Width > 0 && info.Width <= 65535);
                Contract.Assume(info.Height > 0 && info.Height <= 65535);
                if (!TryCompress(info, pixels, pixels.Length, jfifHeader, buffer, buffer.Length, out compressedCount))
                    throw new InternalBufferOverflowException(
                        "Compression failed: compressed output larger then 1.5 * input.");
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
        public static bool TryCompress(JpegLSMetadataInfo info, byte[] pixels, int pixelCount, bool jfifHeader, byte[] destination, int destinationLength, out int compressedCount)
        {
            Contract.Requires<ArgumentNullException>(info != null);
            Contract.Requires<ArgumentException>(info.Width > 0 && info.Width <= 65535);
            Contract.Requires<ArgumentException>(info.Height > 0 && info.Height <= 65535);
            Contract.Requires<ArgumentNullException>(pixels != null);
            Contract.Requires<ArgumentNullException>(pixelCount > 0 && pixelCount <= pixels.Length);
            Contract.Requires<ArgumentNullException>(destination != null);
            Contract.Requires<ArgumentNullException>(destinationLength > 0 && destinationLength <= destination.Length);

            var parameters = new JlsParameters();
            info.CopyTo(ref parameters);
            if (jfifHeader)
            {
                parameters.Jfif.Version = (1 << 8) + 2; // JFIF version 1.02
                parameters.Jfif.Units = 0; // No units, aspect ratio only specified
                parameters.Jfif.DensityX = 1; // use standard 1:1 aspect ratio. (density should always be set to non-zero values).
                parameters.Jfif.DensityY = 1;
            }

            var result = Environment.Is64BitProcess ?
                SafeNativeMethods.JpegLsEncode64(destination, destinationLength, out compressedCount, pixels, pixelCount, ref parameters) :
                SafeNativeMethods.JpegLsEncode(destination, destinationLength, out compressedCount, pixels, pixelCount, ref parameters);
            if (result == JpegLSError.CompressedBufferTooSmall)
                return false;

            HandleResult(result);
            return true;
        }

        /// <summary>
        /// Gets the metadata info as stored in a JPEG-LS compressed bit stream.
        /// </summary>
        /// <param name="source">The JPEG-LS compressed bit stream.</param>
        /// <returns>An JpegLSMetadataInfo instance.</returns>
        /// <exception cref="InvalidDataException">Thrown when the source array contains invalid compressed data.</exception>
        public static JpegLSMetadataInfo GetMetadataInfo(byte[] source)
        {
            Contract.Requires<ArgumentNullException>(source != null);
            Contract.Ensures(Contract.Result<JpegLSMetadataInfo>() != null);

            return GetMetadataInfo(source, source.Length);
        }

        /// <summary>
        /// Gets the metadata info as stored in a JPEG-LS compressed bit stream.
        /// </summary>
        /// <param name="source">The JPEG-LS compressed bit stream.</param>
        /// <param name="count">The count of bytes that are valid in the array.</param>
        /// <returns>An JpegLSMetadataInfo instance.</returns>
        /// <exception cref="InvalidDataException">Thrown when the source array contains invalid compressed data.</exception>
        public static JpegLSMetadataInfo GetMetadataInfo(byte[] source, int count)
        {
            Contract.Requires<ArgumentNullException>(source != null);
            Contract.Requires<ArgumentException>(count >= 0 && count <= source.Length);
            Contract.Ensures(Contract.Result<JpegLSMetadataInfo>() != null);

            JlsParameters info;
            JpegLsReadHeaderThrowWhenError(source, count, out info);
            return new JpegLSMetadataInfo(ref info);
        }

        /// <summary>
        /// Decompresses the JPEG-LS encoded data passed in the source byte array.
        /// </summary>
        /// <param name="source">The byte array that contains the JPEG-LS encoded data to decompress.</param>
        /// <returns>A byte array with the pixel data.</returns>
        /// <exception cref="InvalidDataException">Thrown when the source array contains invalid compressed data.</exception>
        public static byte[] Decompress(byte[] source)
        {
            Contract.Requires<ArgumentNullException>(source != null);
            Contract.Ensures(Contract.Result<byte[]>() != null);

            return Decompress(source, source.Length);
        }

        /// <summary>
        /// Decompresses the JPEG-LS encoded data passed in the source byte array.
        /// </summary>
        /// <param name="source">The byte array that contains the JPEG-LS encoded data to decompress.</param>
        /// <param name="count">The number of bytes of the array to decompress.</param>
        /// <returns>A byte array with the pixel data.</returns>
        /// <exception cref="InvalidDataException">Thrown when the source array contains invalid compressed data.</exception>
        public static byte[] Decompress(byte[] source, int count)
        {
            Contract.Requires<ArgumentNullException>(source != null);
            Contract.Requires<ArgumentException>(count >= 0 && count <= source.Length);
            Contract.Ensures(Contract.Result<byte[]>() != null);

            JlsParameters info;
            JpegLsReadHeaderThrowWhenError(source, count, out info);

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
        /// <exception cref="ArgumentException">Thrown when the destination array is too small to hold the decompressed pixel data.</exception>
        /// <exception cref="InvalidDataException">Thrown when the source array contains an invalid encodeded JPEG-LS bit stream.</exception>
        public static void Decompress(byte[] source, int count, byte[] pixels)
        {
            Contract.Requires<ArgumentNullException>(source != null);
            Contract.Requires<ArgumentException>(count >= 0 && count <= source.Length);
            Contract.Requires<ArgumentNullException>(pixels != null);

            JpegLSError error = Environment.Is64BitProcess ?
                SafeNativeMethods.JpegLsDecode64(pixels, pixels.Length, source, count, IntPtr.Zero) :
                SafeNativeMethods.JpegLsDecode(pixels, pixels.Length, source, count, IntPtr.Zero);
            HandleResult(error);
        }

        private static void JpegLsReadHeaderThrowWhenError(byte[] source, int length, out JlsParameters info)
        {
            Contract.Requires(source != null);

            JpegLSError result = Environment.Is64BitProcess ?
                SafeNativeMethods.JpegLsReadHeader64(source, length, out info) :
                SafeNativeMethods.JpegLsReadHeader(source, length, out info);
            HandleResult(result);
        }

        private static int GetUncompressedSize(ref JlsParameters info)
        {
            Contract.Ensures(Contract.Result<int>() > 0);

            var size = info.Width * info.Height * info.Components * ((info.BitsPerSample + 7) / 8);
            Contract.Assume(size > 0);
            return size;
        }

        private static void HandleResult(JpegLSError result)
        {
            Exception exception;

            switch (result)
            {
                case JpegLSError.None:
                    return;

                case JpegLSError.InvalidCompressedData:
                    exception = new InvalidDataException("Bad compressed data. Unable to decompress.");
                    break;

                case JpegLSError.InvalidJlsParameters:
                    exception = new InvalidDataException("One of the JLS parameters is invalid. Unable to process.");
                    break;

                case JpegLSError.UncompressedBufferTooSmall:
                    exception = new ArgumentException("The pixel buffer is too small, related to the metadata description");
                    break;

                case JpegLSError.ParameterValueNotSupported:
                    exception = new ArgumentException("The pixel buffer is too small, related to the metadata description");
                    break;

                case JpegLSError.CompressedBufferTooSmall:
                    exception = new InvalidDataException("The buffer containing the compressed data is too small to decode the complete image");
                    break;

                case JpegLSError.TooMuchCompressedData:
                    exception = new InvalidDataException("The buffer containing the compressed data has still data while the complete image has already been decoded");
                    break;

                case JpegLSError.ImageTypeNotSupported:
                    exception = new InvalidDataException("The encoded bit stream contains options that are not supported by this implementation");
                    break;

                case JpegLSError.UnsupportedBitDepthForTransform:
                    exception = new InvalidDataException("Unsupported bit depth for transformation");
                    break;

                case JpegLSError.UnsupportedColorTransform:
                    exception = new InvalidDataException("Unsupported color transformation");
                    break;

                case JpegLSError.UnsupportedEncoding:
                    exception = new InvalidDataException("Unsupported encoded frame detected. Only JPEG-LS encoded frames are supported.");
                    break;

                case JpegLSError.UnknownJpegMarker:
                    exception = new InvalidDataException("An unknown JPEG marker code is detected in the encoded bit stream");
                    break;

                case JpegLSError.MissingJpegMarkerStart:
                    exception = new InvalidDataException("The decoding process expects a start of JPEG marker code (0xFF) but none was found");
                    break;

                case JpegLSError.UnspecifiedFailure:
                    exception = new InvalidDataException("Failure detected, but no specific info about the error is available.");
                    break;

                case JpegLSError.UnexpectedFailure:
                    exception = new InvalidOperationException("Unexpected failure. The state of the implementation may be invalid.");
                    break;

                default:
                    exception = new NotImplementedException(string.Format(CultureInfo.InvariantCulture,
                        "The native codec has returned an unexpected result value: {0}", result));
                    break;
            }

            var data = exception.Data;
            Contract.Assume(data != null);
            data.Add("JpegLSError", result);
            throw exception;
        }
    }
}
