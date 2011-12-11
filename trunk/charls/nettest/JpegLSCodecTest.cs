//
// (C) Jan de Vaan 2007-2011, all rights reserved. See the accompanying "License.txt" for licensed use.
//

using System;
using System.Collections.Generic;
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
            var expected = new JpegLSMetadataInfo { Height = 256, Width = 256, BitsPerComponent = 8, ComponentCount = 3 };

            Assert.AreEqual(expected, info);
        }

        [Test]
        public void GetMetadataInfoFromNearLosslessEncodedColorImage()
        {
            var source = ReadAllBytes("T8C0E3.JLS");
            var info = JpegLSCodec.GetMetadataInfo(source);
            var expected = new JpegLSMetadataInfo { Height = 256, Width = 256, BitsPerComponent = 8, ComponentCount = 3, AllowedLossyError = 3 };

            Assert.AreEqual(expected, info);
        }

        [Test]
        public void Decompress()
        {
            var source = ReadAllBytes("T8C0E0.JLS");
            var expected = ReadAllBytes("TEST8.PPM", 15);
            var uncompressed = JpegLSCodec.Decompress(source);

            var info = JpegLSCodec.GetMetadataInfo(source);
            if (info.InterleaveMode == JpegLSInterleaveMode.Planar && info.ComponentCount == 3)
            {
                expected = TripletToPlanar(expected, info.Width, info.Height);
            }

            Assert.AreEqual(expected, uncompressed);
        }

        [Test]
        public void Compress()
        {
            var info = new JpegLSMetadataInfo(256, 256, 8, 3);

            var uncompressedOriginal = ReadAllBytes("TEST8.PPM", 15);
            uncompressedOriginal = TripletToPlanar(uncompressedOriginal, info.Width, info.Height);

            var compressedSegment = JpegLSCodec.Compress(info, uncompressedOriginal);
            var compressed = new byte[compressedSegment.Count];
            Array.Copy(compressedSegment.Array, compressed, compressed.Length);

            var compressedInfo = JpegLSCodec.GetMetadataInfo(compressed);
            Assert.AreEqual(info, compressedInfo);

            var uncompressed = JpegLSCodec.Decompress(compressed);
            Assert.AreEqual(info.UncompressedSize, uncompressed.Length);
            Assert.AreEqual(uncompressedOriginal, uncompressed);
        }

        [Test]
        public void CompressOneByOneColor()
        {
            var info = new JpegLSMetadataInfo(1, 1, 8, 3);
            var uncompressedOriginal = new byte[] { 77, 33, 255 };

            var compressedSegment = JpegLSCodec.Compress(info, uncompressedOriginal);
            var compressed = new byte[compressedSegment.Count];
            Array.Copy(compressedSegment.Array, compressed, compressed.Length);

            var uncompressed = JpegLSCodec.Decompress(compressed);
            Assert.AreEqual(info.UncompressedSize, uncompressed.Length);
            Assert.AreEqual(uncompressedOriginal, uncompressed);
        }

        [Test]
        [Ignore]
        public void CompressOneByOneBlackAndWhite()
        {
            var info = new JpegLSMetadataInfo(1, 1, 1, 1);
            var uncompressedOriginal = new byte[] { 1 };

            var compressedSegment = JpegLSCodec.Compress(info, uncompressedOriginal);
            var compressed = new byte[compressedSegment.Count];
            Array.Copy(compressedSegment.Array, compressed, compressed.Length);

            var uncompressed = JpegLSCodec.Decompress(compressed);
            Assert.AreEqual(info.UncompressedSize, uncompressed.Length);
            Assert.AreEqual(uncompressedOriginal, uncompressed);
        }

        private static byte[] TripletToPlanar(IList<byte> buffer, int width, int height)
        {
            var result = new byte[buffer.Count];

            int bytePlaneCount = width * height;
            for (int i = 0; i < bytePlaneCount; i++)
            {
                result[i] = buffer[i * 3];
                result[i + bytePlaneCount] = buffer[(i * 3) + 1];
                result[i + (2 * bytePlaneCount)] = buffer[(i * 3) + 2];
            }

            return result;
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
