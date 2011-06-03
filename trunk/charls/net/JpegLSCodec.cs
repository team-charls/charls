//
// (C) Jan de Vaan 2007-2011, all rights reserved. See the accompanying "License.txt" for licensed use.
//

using System;
using System.Runtime.InteropServices;

namespace CharLS
{
    using System.IO;

    internal enum JpegLSError
    {
        None = 0,
        InvalidJlsParameters,
        ParameterValueNotSupported,
        UncompressedBufferTooSmall,
        CompressedBufferTooSmall,
        InvalidCompressedData,
        TooMuchCompressedData,
        ImageTypeNotSupported,
        UnsupportedBitDepthForTransform,
        UnsupportedColorTransform
    }

    enum JpegLSInterleaveMode
    {
        None,
        Line,
        Sample
    }

    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    internal struct JlsCustomParameters
    {
        int MAXVAL;
        private int T1;
        private int T2;
        private int T3;
        private int RESET;

        public int Threshold1
        {
            get { return T1; }
            set { T1 = value; }
        }

        public int Threshold2
        {
            get { return T2; }
            set { T2 = value; }
        }

        public int Threshold3
        {
            get { return T3; }
            set { T3 = value; }
        }
    };

    struct JfifParameters
    {
        int   Ver;
        byte  units;
        int   XDensity;
        int   YDensity;
        short Xthumb;
        short Ythumb;
        IntPtr pdataThumbnail; // user must set buffer which size is Xthumb*Ythumb*3(RGB) before JpegLsDecode()
    };

    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    internal struct JlsParameters
    {
        public int width;
        public int height;
        public int bitspersample;
        public int bytesperline;  // for [source (at encoding)][decoded (at decoding)] pixel image in user buffer
        public int components;
        public int allowedlossyerror;
        public JpegLSInterleaveMode ilv;
        public int colorTransform;
        public char outputBgr;
        public JlsCustomParameters custom;
        public JfifParameters jfif;
    };


    public static class JpegLSCodec
    {
        private const string Nativex86Library = "CharLS.dll";

        public static byte[] Compress(byte[] source, long index, long length)
        {
            return null;
        }

        public static byte[] Decompress(byte[] source)
        {
            return Decompress(source, source.Length);
        }

        public static byte[] Decompress(byte[] source, int length)
        {
            JlsParameters info;
            JpegLsReadHeaderThrowWhenError(source, length, out info);

            var decompressed = new byte[GetUncompressedSize(ref info)];
            JpegLSError error = JpegLsDecode(decompressed, decompressed.Length, source, length, IntPtr.Zero);
            HandleResult(error);
            return decompressed;
        }

        public static JpegLSMetadataInfo GetMetadataInfo(byte[] source)
        {
            return GetMetadataInfo(source, source.Length);
        }

        public static JpegLSMetadataInfo GetMetadataInfo(byte[] source, int length)
        {
            JlsParameters info;
            JpegLsReadHeaderThrowWhenError(source, length, out info);
            return new JpegLSMetadataInfo(ref info);
        }

        private static void JpegLsReadHeaderThrowWhenError(byte[] source, int length, out JlsParameters info)
        {
            JpegLSError result = JpegLsReadHeader(source, length, out info);
            HandleResult(result);
        }

        private static int GetUncompressedSize(ref JlsParameters info)
        {
            return info.width * info.height * info.components * ((info.bitspersample + 7) / 8);
        }

        private static void HandleResult(JpegLSError result)
        {
            if (result == JpegLSError.None)
                return;

            if (result == JpegLSError.InvalidCompressedData)
                throw new InvalidDataException();
        }

        [DllImport(Nativex86Library, SetLastError = false)]
        private static extern JpegLSError JpegLsReadHeader([In] byte[] compressedSource, int compressedLength, out JlsParameters info);

        [DllImport(Nativex86Library, SetLastError = false)]
        private static extern JpegLSError JpegLsDecode(
            [Out] byte[] uncompressedData,
            int uncompressedLength,
            [In] byte[] compressedData,
            int compressedLength,
            IntPtr info);

        [DllImport(Nativex86Library, SetLastError = false)]
        private static extern JpegLSError JpegLsEncode(
            byte[] compressedData,
            int compressedLength,
            ref int byteCountWritten,
            byte[] uncompressedData,
            int uncompressedLength,
            ref JlsParameters info);
    }
}
