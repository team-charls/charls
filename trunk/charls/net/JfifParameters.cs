//
// (C) Jan de Vaan 2007-2011, all rights reserved. See the accompanying "License.txt" for licensed use.
//

using System;
using System.Runtime.InteropServices;

namespace CharLS
{
    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    internal struct JfifParameters
    {
        private int ver;
        private byte units;
        private int densityX;
        private int densityY;
        private short thumbX;
        private short thumbY;
        private IntPtr dataThumbnail; // user must set buffer which size is Xthumb*Ythumb*3(RGB) before JpegLsDecode()
    }
}
