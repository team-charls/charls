//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use.
//

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
