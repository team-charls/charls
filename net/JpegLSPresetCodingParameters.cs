// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

using System.Runtime.InteropServices;

namespace CharLS
{
    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    internal struct JpegLSPresetCodingParameters
    {
        internal int MaximumSampleValue;
        internal int Threshold1;
        internal int Threshold2;
        internal int Threshold3;
        internal int ResetValue;
    }
}
