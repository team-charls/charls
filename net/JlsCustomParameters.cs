//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use.
//

using System.Runtime.InteropServices;

namespace CharLS
{
    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    internal struct JlsCustomParameters
    {
        internal int MaxValue;
        internal int Threshold1;
        internal int Threshold2;
        internal int Threshold3;
        internal int ResetThreshold;
    }
}
