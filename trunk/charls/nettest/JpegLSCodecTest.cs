//
// (C) Jan de Vaan 2007-2011, all rights reserved. See the accompanying "License.txt" for licensed use.
//

using System;
using NUnit.Framework;

namespace CharLS.Test
{
    using System.IO;

    [TestFixture]
    public class JpegLSCodecTest
    {
        [Test]
        public void GetMetadataInfo()
        {
            var source = ReadAllBytes("T8C0E0.JLS");
            var info = JpegLSCodec.GetMetadataInfo(source);

            Assert.AreEqual(256, info.Width);
            Assert.AreEqual(256, info.Height);
            Assert.AreEqual(3, info.ComponentCount);
        }

        [Test]
        public void Decompress()
        {
            var source = ReadAllBytes("T8C0E0.JLS");
            var uncompressed = JpegLSCodec.Decompress(source);
        }

        private static byte[] ReadAllBytes(string path)
        {
            return File.ReadAllBytes("DataFiles\\" + path);
        }
    }

}
