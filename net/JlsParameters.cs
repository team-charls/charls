//
// (C) CharLS Team, all rights reserved. See the accompanying "License.txt" for licensed use.
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
        private readonly int colorTransform; // note: not used in this adapter interface.
        internal bool OutputBgr;
        private readonly JpegLSPresetCodingParameters custom;  // note: not used in this adapter interface.
        internal JfifParameters Jfif;
    }
}
