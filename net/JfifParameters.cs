//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use.
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
    /// Some fields are not used but defined to ensure memory layout and size is identical with the native structure.
    /// </remarks>
    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    internal struct JfifParameters
    {
        /// <summary>
        /// Indicates the JFIF version. First byte is major version (currently 0x01), Second byte is minor version (currently 0x02).
        /// </summary>
        internal int Version;

        /// <summary>
        /// Units for pixel density fields. 0 - No units, aspect ratio only specified. 1 - Pixels per inch. 2 - Pixels per centimeter.
        /// </summary>
        internal int Units;

        /// <summary>
        /// Integer horizontal pixel density.
        /// </summary>
        internal int DensityX;

        /// <summary>
        /// Integer vertical pixel density.
        /// </summary>
        internal int DensityY;

        private readonly int thumbX; // note: passing a thumbnail to add to the bytestream is currently not supported in the .NET layer.
        private readonly int thumbY;
        private readonly IntPtr dataThumbnail; // user must set buffer which size is Xthumb*Ythumb*3(RGB) before JpegLsDecode()
    }
}
