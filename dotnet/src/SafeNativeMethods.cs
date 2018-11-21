// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

using System;
using System.Runtime.InteropServices;

namespace CharLS
{
    internal static class SafeNativeMethods
    {
        private const string NativeX86Library = "CharLS.dll";
        private const string NativeX64Library = "CharLS64.dll";

        [DllImport(NativeX86Library, SetLastError = false, CharSet = CharSet.Ansi, BestFitMapping = false, ThrowOnUnmappableChar = true)]
        internal static extern JpegLSError JpegLsReadHeader([In] byte[] compressedSource, int compressedLength, out JlsParameters info, IntPtr reserved);

        [DllImport(NativeX64Library, SetLastError = false, CharSet = CharSet.Ansi, BestFitMapping = false, ThrowOnUnmappableChar = true, EntryPoint = "JpegLsReadHeader")]
        internal static extern JpegLSError JpegLsReadHeader64([In] byte[] compressedSource, long compressedLength, out JlsParameters info, IntPtr reserved);

        [DllImport(NativeX86Library, SetLastError = false, CharSet = CharSet.Ansi, BestFitMapping = false, ThrowOnUnmappableChar = true)]
        internal static extern JpegLSError JpegLsDecode(
            [Out] byte[] uncompressedData,
            int uncompressedLength,
            [In] byte[] compressedData,
            int compressedLength,
            IntPtr info,
            IntPtr reserved);

        [DllImport(NativeX64Library, SetLastError = false, CharSet = CharSet.Ansi, BestFitMapping = false, ThrowOnUnmappableChar = true, EntryPoint = "JpegLsDecode")]
        internal static extern JpegLSError JpegLsDecode64(
            [Out] byte[] uncompressedData,
            long uncompressedLength,
            [In] byte[] compressedData,
            long compressedLength,
            IntPtr info,
            IntPtr reserved);

        [DllImport(NativeX86Library, SetLastError = false, CharSet = CharSet.Ansi, BestFitMapping = false, ThrowOnUnmappableChar = true)]
        internal static extern JpegLSError JpegLsEncode(
            [Out] byte[] compressedData,
            int compressedLength,
            out int byteCountWritten,
            [In] byte[] uncompressedData,
            int uncompressedLength,
            [In] ref JlsParameters info,
            IntPtr reserved);

        [DllImport(NativeX64Library, SetLastError = false, CharSet = CharSet.Ansi, BestFitMapping = false, ThrowOnUnmappableChar = true, EntryPoint = "JpegLsEncode")]
        internal static extern JpegLSError JpegLsEncode64(
            [Out] byte[] compressedData,
            long compressedLength,
            out long byteCountWritten,
            [In] byte[] uncompressedData,
            long uncompressedLength,
            [In] ref JlsParameters info,
            IntPtr reserved);

        [DllImport(NativeX86Library, SetLastError = false, CharSet = CharSet.Ansi, BestFitMapping = false, ThrowOnUnmappableChar = true, EntryPoint = "charls_get_error_message")]
        internal static extern IntPtr CharLSGetErrorMessage(int errorValue);

        [DllImport(NativeX64Library, SetLastError = false, CharSet = CharSet.Ansi, BestFitMapping = false, ThrowOnUnmappableChar = true, EntryPoint = "charls_get_error_message")]
        internal static extern IntPtr CharLSGetErrorMessage64(int errorValue);
    }
}
