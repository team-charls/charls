//
// (C) Jan de Vaan 2007-2011, all rights reserved. See the accompanying "License.txt" for licensed use.
//

using System;
using System.Diagnostics.Contracts;
using System.IO;
using System.Runtime.InteropServices;

namespace CharLS
{
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
    }

    struct JfifParameters
    {
        int   Ver;
        byte  units;
        int   XDensity;
        int   YDensity;
        short Xthumb;
        short Ythumb;
        IntPtr pdataThumbnail; // user must set buffer which size is Xthumb*Ythumb*3(RGB) before JpegLsDecode()
    }

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
    }


    public static class JpegLSCodec
    {
        public static byte[] Compress(byte[] source, long index, long length)
        {
            return null;
        }

        public static JpegLSMetadataInfo GetMetadataInfo(byte[] source)
        {
            Contract.Requires<ArgumentNullException>(source != null);

            return GetMetadataInfo(source, source.Length);
        }

        public static JpegLSMetadataInfo GetMetadataInfo(byte[] source, int count)
        {
            Contract.Requires<ArgumentNullException>(source != null);
            Contract.Requires<ArgumentException>(count >= 0 && count <= source.Length);

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

            JlsParameters info;
            JpegLsReadHeaderThrowWhenError(source, count, out info);

            var decompressed = new byte[GetUncompressedSize(ref info)];
            Decompress(source, count, decompressed);
            return decompressed;
        }

        /// <summary>
        /// Decompresses the JPEG-LS encoded data passed in the source byte array into the destination array.
        /// </summary>
        /// <param name="source">The byte array that contains the JPEG-LS encoded data to decompress.</param>
        /// <param name="count">The number of bytes of the array to decompress.</param>
        /// <param name="destination">The destination byte array that will the decompressed data when the function returns.</param>
        public static void Decompress(byte[] source, int count, byte[] destination)
        {
            Contract.Requires<ArgumentNullException>(source != null);
            Contract.Requires<ArgumentException>(count >= 0 && count <= source.Length);
            Contract.Requires<ArgumentNullException>(destination != null);

            JpegLSError error = SafeNativeMethods.JpegLsDecode(source, source.Length, source, count, IntPtr.Zero);
            HandleResult(error);
        }

        private static void JpegLsReadHeaderThrowWhenError(byte[] source, int length, out JlsParameters info)
        {
            Contract.Requires(source != null);

            JpegLSError result = SafeNativeMethods.JpegLsReadHeader(source, length, out info);
            HandleResult(result);
        }

        private static int GetUncompressedSize(ref JlsParameters info)
        {
            Contract.Ensures(Contract.Result<int>() > 0);

            var size = info.width * info.height * info.components * ((info.bitspersample + 7) / 8);
            Contract.Assume(size > 0);
            return size;
        }

        private static void HandleResult(JpegLSError result)
        {
            if (result == JpegLSError.None)
                return;

            if (result == JpegLSError.InvalidCompressedData)
                throw new InvalidDataException();
        }
    }
}
