//
// (C) Jan de Vaan 2007-2011, all rights reserved. See the accompanying "License.txt" for licensed use.
//

using System;
using System.Runtime.InteropServices;

namespace CharLS
{
    /// <summary>
    /// Encapsulates the parameters that will be written to the JFIF header.
    /// Since JFIF 1.02 thumbnails should preferable be created in extended segments.
    /// </summary>
    /// <remarks>
    /// Some fields are not used but neverless defined to ensure memory layout and size is identical with the native structure.
    /// </remarks>
    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    internal struct JfifParameters
    {
        internal int Version;
        internal byte Units;
        internal int DensityX;
        internal int DensityY;
        private readonly short thumbX; // note: passing a thumbnail to add to the bytestream is currently not supported in the .NET layer.
        private readonly short thumbY;
        private readonly IntPtr dataThumbnail; // user must set buffer which size is Xthumb*Ythumb*3(RGB) before JpegLsDecode()
    }
}
