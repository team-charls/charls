//
// (C) Jan de Vaan 2007-2011, all rights reserved. See the accompanying "License.txt" for licensed use.
//

using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace CharLS.Test
{
    [TestClass]
    public class JpegLSCodecTest
    {
        [TestMethod]
        public void JpegLsReadHeader()
        {
            var source = new byte[100];

            JpegLSCodec.GetPixelDataInfo(source);
        }
    }
}
