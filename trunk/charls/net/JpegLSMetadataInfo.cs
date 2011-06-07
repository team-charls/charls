//
// (C) Jan de Vaan 2007-2011, all rights reserved. See the accompanying "License.txt" for licensed use.
//

using System;

namespace CharLS
{
    /// <summary>
    /// Contains meta information about a compressed JPEG-LS stream or info how to compress.
    /// </summary>
    public class JpegLSMetadataInfo : IEquatable<JpegLSMetadataInfo>
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="JpegLSMetadataInfo"/> class.
        /// </summary>
        public JpegLSMetadataInfo()
        {
        }

        internal JpegLSMetadataInfo(ref JlsParameters parameters)
        {
            Width = parameters.Width;
            Height = parameters.Height;
            ComponentCount = parameters.Components;
            BitsPerSample = parameters.BitsPerSample;
            AllowedLossyError = parameters.AllowedLossyError;
        }

        /// <summary>
        /// Gets or sets the width.
        /// </summary>
        /// <value>The width.</value>
        public int Width { get; set; }

        /// <summary>
        /// Gets or sets the height.
        /// </summary>
        /// <value>The height.</value>
        public int Height { get; set; }

        /// <summary>
        /// Gets or sets the bits per sample.
        /// </summary>
        /// <value>The bits per sample.</value>
        public int BitsPerSample { get; set; }

        /// <summary>
        /// Gets or sets the bytes per line.
        /// </summary>
        /// <value>The bytes per line.</value>
        public int BytesPerLine { get; set; }

        /// <summary>
        /// Gets or sets the component count.
        /// </summary>
        /// <value>The component count.</value>
        public int ComponentCount { get; set; }

        /// <summary>
        /// Gets or sets the allowed lossy error.
        /// </summary>
        /// <value>The allowed lossy error.</value>
        public int AllowedLossyError { get; set; }

        /// <summary>
        /// Gets the size of an byte array needed to hold the uncompressed pixels.
        /// </summary>
        /// <value>The size of byte array.</value>
        public int UncompressedSize
        {
            get
            {
                return Width * Height * ComponentCount * ((BitsPerSample + 7) / 8);
            }
        }

        public override string ToString()
        {
            return string.Format("Width = {0}, Height = {1}, BitsPerSample = {2}, ComponentCount = {3}, AllowedLossyError = {4}",
                Width, Height, BitsPerSample, ComponentCount, AllowedLossyError);
        }

        public override bool Equals(object obj)
        {
            return Equals(obj as JpegLSMetadataInfo);
        }

        public bool Equals(JpegLSMetadataInfo other)
        {
            if (other == null)
                return false;

            return Width == other.Width &&
                   Height == other.Height &&
                   ComponentCount == other.ComponentCount &&
                   BitsPerSample == other.BitsPerSample &&
                   AllowedLossyError == other.AllowedLossyError;
        }

        public override int GetHashCode()
        {
            unchecked
            {
                int result = this.Width;
                result = (result * 397) ^ this.Height;
                result = (result * 397) ^ this.BitsPerSample;
                result = (result * 397) ^ this.BytesPerLine;
                result = (result * 397) ^ this.ComponentCount;
                result = (result * 397) ^ this.AllowedLossyError;
                return result;
            }
        }
    }
}
