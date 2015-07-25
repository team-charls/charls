//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use.
//

using System.Runtime.InteropServices;

namespace CharLS
{
    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    internal struct JlsParameters
    {
        internal int Width;
        internal int Height;
        internal int BitsPerSample;
        internal int BytesPerLine;
        internal int Components;
        internal int AllowedLossyError;
        internal JpegLSInterleaveMode InterleaveMode;
        internal int ColorTransform;
        internal bool OutputBgr;
        internal JlsCustomParameters Custom;
        internal JfifParameters Jfif;
    }
}
