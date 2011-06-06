//
// (C) Jan de Vaan 2007-2011, all rights reserved. See the accompanying "License.txt" for licensed use.
//

using System.Runtime.InteropServices;

namespace CharLS
{
    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    internal struct JlsCustomParameters
    {
        private int maxValue;
        private int t1;
        private int t2;
        private int t3;
        private int resetThreshold;

        public int Threshold1
        {
            get { return t1; }
            set { t1 = value; }
        }

        public int Threshold2
        {
            get { return t2; }
            set { t2 = value; }
        }

        public int Threshold3
        {
            get { return t3; }
            set { t3 = value; }
        }
    }
}
