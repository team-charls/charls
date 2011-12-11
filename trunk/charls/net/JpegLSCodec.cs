//
// (C) Jan de Vaan 2007-2011, all rights reserved. See the accompanying "License.txt" for licensed use.
//

using System;
using System.Diagnostics.Contracts;
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
        /// Compresses the specified image passed in the source buffer.
        /// </summary>
        /// <param name="info">The info.</param>
        /// <param name="pixels">The source.</param>
        /// <param name="jfifHeader">if set to <c>true</c> a jfif header will be added to the encoded byte stream.</param>
        /// <returns>
        /// An arraySegment with a reference to the byte array with the compressed data in the JPEG-LS format.
        /// </returns>
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
        /// Compresses the specified image passed in the source buffer.
        /// </summary>
        /// <param name="info">The info.</param>
        /// <param name="pixels">The source.</param>
        /// <param name="pixelCount">The pixel count.</param>
        /// <param name="jfifHeader">if set to <c>true</c> a jfif header will be added to the encoded byte stream.</param>
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

            if (!TryCompress(info, pixels, pixels.Length, buffer, buffer.Length, jfifHeader, out compressedCount))
            {
                // Increase output buffer to hold compressed data.
                buffer = new byte[(int)(pixels.Length * 1.5) + JpegLSHeaderLength];

                Contract.Assume(info.Width > 0 && info.Width <= 65535);
                Contract.Assume(info.Height > 0 && info.Height <= 65535);
                if (!TryCompress(info, pixels, pixels.Length, buffer, buffer.Length, jfifHeader, out compressedCount))
                    throw new InternalBufferOverflowException(
                        "Compression failed: compressed output larger then 1.5 * input.");
            }

            return new ArraySegment<byte>(buffer, 0, compressedCount);
        }

        /// <summary>
        /// Tries the compress the array with pixels into the provided buffer.
        /// </summary>
        /// <param name="info">The info.</param>
        /// <param name="pixels">The pixels.</param>
        /// <param name="pixelCount">The pixel count.</param>
        /// <param name="buffer">The buffer.</param>
        /// <param name="bufferLength">Length of the buffer.</param>
        /// <param name="jfifHeader">if set to <c>true</c> a jfif header will be added to the encoded byte stream.</param>
        /// <param name="compressedCount">The compressed count.</param>
        /// <returns>true when the compressed fits into the buffer, otherwise false.</returns>
        public static bool TryCompress(JpegLSMetadataInfo info, byte[] pixels, int pixelCount, byte[] buffer, int bufferLength, bool jfifHeader, out int compressedCount)
        {
            Contract.Requires<ArgumentNullException>(info != null);
            Contract.Requires<ArgumentException>(info.Width > 0 && info.Width <= 65535);
            Contract.Requires<ArgumentException>(info.Height > 0 && info.Height <= 65535);
            Contract.Requires<ArgumentNullException>(pixels != null);
            Contract.Requires<ArgumentNullException>(pixelCount > 0 && pixelCount <= pixels.Length);
            Contract.Requires<ArgumentNullException>(buffer != null);
            Contract.Requires<ArgumentNullException>(bufferLength > 0 && bufferLength <= buffer.Length);

            var parameters = new JlsParameters();
            info.CopyTo(ref parameters);
            if (jfifHeader)
            {
                parameters.Jfif.Version = (1 << 8) + 2; // JFIF version 1.02
                parameters.Jfif.Units = 0;
                parameters.Jfif.DensityX = 1;
                parameters.Jfif.DensityY = 1;
            }

            var result = SafeNativeMethods.JpegLsEncode(
                buffer, bufferLength, out compressedCount, pixels, pixelCount, ref parameters);
            if (result == JpegLSError.CompressedBufferTooSmall)
                return false;

            HandleResult(result);
            return true;
        }

        /// <summary>
        /// Gets the metadata info as stored in a JPEG-LS compressed byte array.
        /// </summary>
        /// <param name="source">The JPEG-LS compressed source.</param>
        /// <returns>An JpegLSMetadataInfo instance.</returns>
        public static JpegLSMetadataInfo GetMetadataInfo(byte[] source)
        {
            Contract.Requires<ArgumentNullException>(source != null);
            Contract.Ensures(Contract.Result<JpegLSMetadataInfo>() != null);

            return GetMetadataInfo(source, source.Length);
        }

        /// <summary>
        /// Gets the metadata info as stored in a JPEG-LS compressed byte array.
        /// </summary>
        /// <param name="source">The JPEG-LS compressed source.</param>
        /// <param name="count">The count of bytes that are valid in the array.</param>
        /// <returns>An JpegLSMetadataInfo instance.</returns>
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
        /// <returns>A byte array with the decompressed data.</returns>
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
        /// <returns>A byte array with the decompressed data.</returns>
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
        /// <param name="destination">The destination byte array that will hold the decompressed data when the function returns.</param>
        public static void Decompress(byte[] source, int count, byte[] destination)
        {
            Contract.Requires<ArgumentNullException>(source != null);
            Contract.Requires<ArgumentException>(count >= 0 && count <= source.Length);
            Contract.Requires<ArgumentNullException>(destination != null);

            JpegLSError error = SafeNativeMethods.JpegLsDecode(destination, destination.Length, source, count, IntPtr.Zero);
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
            if (result == JpegLSError.None)
                return;

            if (result == JpegLSError.InvalidCompressedData)
                throw new InvalidDataException("Bad compressed data. Unable to decompress.");

            if (result == JpegLSError.InvalidJlsParameters)
                throw new InvalidDataException("One of the JLS parameters is invalid. Unable to process.");

            if (result == JpegLSError.UncompressedBufferTooSmall)
                throw new ArgumentException("The pixel buffer is too small, related to the metadata description");
        }
    }
}
