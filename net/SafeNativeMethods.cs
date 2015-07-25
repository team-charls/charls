//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use.
//

using System;
using System.Runtime.InteropServices;
using System.Text;

namespace CharLS
{
    internal static class SafeNativeMethods
    {
        private const string Nativex86Library = "CharLS.dll";
        private const string Nativex64Library = "CharLS64.dll";

        [DllImport(Nativex86Library, SetLastError = false, CharSet = CharSet.Ansi, BestFitMapping = false, ThrowOnUnmappableChar = true)]
        internal static extern JpegLSError JpegLsReadHeader([In] byte[] compressedSource, int compressedLength, out JlsParameters info, [Out] StringBuilder errorMessage);

        [DllImport(Nativex64Library, SetLastError = false, CharSet = CharSet.Ansi, BestFitMapping = false, ThrowOnUnmappableChar = true, EntryPoint = "JpegLsReadHeader")]
        internal static extern JpegLSError JpegLsReadHeader64([In] byte[] compressedSource, long compressedLength, out JlsParameters info, [Out] StringBuilder errorMessage);

        [DllImport(Nativex86Library, SetLastError = false, CharSet = CharSet.Ansi, BestFitMapping = false, ThrowOnUnmappableChar = true)]
        internal static extern JpegLSError JpegLsDecode(
            [Out] byte[] uncompressedData,
            int uncompressedLength,
            [In] byte[] compressedData,
            int compressedLength,
            IntPtr info,
            [Out] StringBuilder errorMessage);

        [DllImport(Nativex64Library, SetLastError = false, CharSet = CharSet.Ansi, BestFitMapping = false, ThrowOnUnmappableChar = true, EntryPoint = "JpegLsDecode")]
        internal static extern JpegLSError JpegLsDecode64(
            [Out] byte[] uncompressedData,
            long uncompressedLength,
            [In] byte[] compressedData,
            long compressedLength,
            IntPtr info,
            [Out] StringBuilder errorMessage);

        [DllImport(Nativex86Library, SetLastError = false, CharSet = CharSet.Ansi, BestFitMapping = false, ThrowOnUnmappableChar = true)]
        internal static extern JpegLSError JpegLsEncode(
            [Out] byte[] compressedData,
            int compressedLength,
            out int byteCountWritten,
            [In] byte[] uncompressedData,
            int uncompressedLength,
            [In] ref JlsParameters info,
            [Out] StringBuilder errorMessage);

        [DllImport(Nativex64Library, SetLastError = false, CharSet = CharSet.Ansi, BestFitMapping = false, ThrowOnUnmappableChar = true, EntryPoint = "JpegLsEncode")]
        internal static extern JpegLSError JpegLsEncode64(
            [Out] byte[] compressedData,
            long compressedLength,
            out long byteCountWritten,
            [In] byte[] uncompressedData,
            long uncompressedLength,
            [In] ref JlsParameters info,
            [Out] StringBuilder errorMessage);
    }
}
