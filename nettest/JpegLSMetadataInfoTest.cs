//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use.
//

using NUnit.Framework;

namespace CharLS.Test
{
    [TestFixture]
    public class JpegLSMetadataInfoTest
    {
        [Test]
        public void ConstructDefault()
        {
            var info = new JpegLSMetadataInfo();

            Assert.AreEqual(1, info.Width);
            Assert.AreEqual(1, info.Height);
            Assert.AreEqual(1, info.ComponentCount);
            Assert.AreEqual(2, info.BitsPerComponent);
            Assert.AreEqual(0, info.AllowedLossyError);
            Assert.AreEqual(0, info.BytesPerLine);
            Assert.IsFalse(info.OutputBgr);
            Assert.AreEqual(JpegLSInterleaveMode.None, info.InterleaveMode);
        }

        [Test]
        public void ConstructAndModify()
        {
            var info = new JpegLSMetadataInfo
            {
                BytesPerLine = 2,
                InterleaveMode = JpegLSInterleaveMode.Sample,
                OutputBgr = true
            };

            Assert.AreEqual(2, info.BytesPerLine);
            Assert.AreEqual(JpegLSInterleaveMode.Sample, info.InterleaveMode);
            Assert.IsTrue(info.OutputBgr);

            info.InterleaveMode = JpegLSInterleaveMode.Line;
            Assert.AreEqual(JpegLSInterleaveMode.Line, info.InterleaveMode);
        }

        [Test]
        public void EquatableSameObjects()
        {
            var a = new JpegLSMetadataInfo();
            var b = new JpegLSMetadataInfo();

            Assert.IsTrue(a.Equals(b));
            Assert.IsTrue(a.Equals((object)b));
            Assert.AreEqual(a, b);
            Assert.AreEqual(b, a);
            Assert.AreEqual(a.GetHashCode(), b.GetHashCode());
        }

        [Test]
        public void EquatableDifferentObjects()
        {
            var a = new JpegLSMetadataInfo();
            var b = new JpegLSMetadataInfo { Height = 2 };

            Assert.IsFalse(a.Equals(b));
            Assert.IsFalse(a.Equals((object)b));
        }

        [Test]
        public void EquatableWithNull()
        {
            var a = new JpegLSMetadataInfo();
            JpegLSMetadataInfo b = null;

            Assert.IsFalse(a.Equals(b));
            Assert.IsFalse(a.Equals((object)null));
        }
    }
}
