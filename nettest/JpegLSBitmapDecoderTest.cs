//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use.
//

using System;
using System.IO;
using System.Windows.Media;
using NUnit.Framework;

namespace CharLS.Test
{
    [TestFixture]
    public class JpegLSBitmapDecoderTest
    {
        [Test]
        public void CreateWithNull()
        {
            Assert.Throws<ArgumentNullException>(() => new JpegLSBitmapDecoder(null));
        }

        public void Rgb24Bitmap()
        {
            using (var stream = OpenDataFile("T8C0E0.JLS"))
            {
                var frame = new JpegLSBitmapDecoder(stream).Frames[0];
                Assert.AreEqual(256, frame.PixelHeight);
                Assert.AreEqual(256, frame.PixelWidth);
                Assert.AreEqual(PixelFormats.Rgb24, frame.Format);
            }
        }

        private Stream OpenDataFile(string path)
        {
            var fullPath = "DataFiles\\" + path;
            return File.OpenRead(fullPath);
        }
    }
}
