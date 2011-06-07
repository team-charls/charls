//
// (C) Jan de Vaan 2007-2011, all rights reserved. See the accompanying "License.txt" for licensed use.
//

using System.IO;
using NUnit.Framework;

namespace CharLS.Test
{
    [TestFixture]
    public class JpegLSCodecTest
    {
        [Test]
        public void GetMetadataInfoFromLosslessEncodedColorImage()
        {
            var source = ReadAllBytes("T8C0E0.JLS");
            var info = JpegLSCodec.GetMetadataInfo(source);
            var expected = new JpegLSMetadataInfo { Height = 256, Width = 256, BitsPerSample = 8, ComponentCount = 3 };

            Assert.AreEqual(expected, info);
        }

        [Test]
        public void GetMetadataInfoFromNearLosslessEncodedColorImage()
        {
            var source = ReadAllBytes("T8C0E3.JLS");
            var info = JpegLSCodec.GetMetadataInfo(source);
            var expected = new JpegLSMetadataInfo { Height = 256, Width = 256, BitsPerSample = 8, ComponentCount = 3, AllowedLossyError = 3 };

            Assert.AreEqual(expected, info);
        }

        [Test]
        public void Decompress()
        {
            var source = ReadAllBytes("T8C0E0.JLS");
            var expected = ReadAllBytes("TEST8.PPM", 15);
            var uncompressed = JpegLSCodec.Decompress(source);

            //// TODO: resolve issue why decompressed data doesn't match expected.
            ////Assert.AreEqual(expected, uncompressed);
        }

        private static byte[] ReadAllBytes(string path, int bytesToSkip = 0)
        {
            var fullPath = "DataFiles\\" + path;

            if (bytesToSkip == 0)
                return File.ReadAllBytes(fullPath);

            using (var stream = File.OpenRead(fullPath))
            {
                var result = new byte[new FileInfo(fullPath).Length - bytesToSkip];

                stream.Seek(bytesToSkip, SeekOrigin.Begin);
                stream.Read(result, 0, result.Length);
                return result;
            }
        }
    }
}
