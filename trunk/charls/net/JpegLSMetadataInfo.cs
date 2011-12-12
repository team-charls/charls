//
// (C) Jan de Vaan 2007-2011, all rights reserved. See the accompanying "License.txt" for licensed use.
//

using System;
using System.Diagnostics.Contracts;
using System.Globalization;

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

        /// <summary>
        /// Initializes a new instance of the <see cref="JpegLSMetadataInfo"/> class.
        /// </summary>
        /// <param name="pixelWidth">The width of the bitmap.</param>
        /// <param name="pixelHeight">The height of the bitmap.</param>
        /// <param name="bitsPerSample">The bits per sample.</param>
        /// <param name="componentCount">The component count.</param>
        public JpegLSMetadataInfo(int pixelWidth, int pixelHeight, int bitsPerSample, int componentCount)
        {
            Width = pixelWidth;
            Height = pixelHeight;
            BitsPerComponent = bitsPerSample;
            ComponentCount = componentCount;
        }

        internal JpegLSMetadataInfo(ref JlsParameters parameters)
        {
            Width = parameters.Width;
            Height = parameters.Height;
            ComponentCount = parameters.Components;
            BitsPerComponent = parameters.BitsPerSample;
            AllowedLossyError = parameters.AllowedLossyError;
            InterleaveMode = parameters.InterleaveMode;
        }

        /// <summary>
        /// Gets or sets the width of the image in pixels.
        /// </summary>
        /// <value>The width.</value>
        public int Width { get; set; }

        /// <summary>
        /// Gets or sets the height of the image in pixels.
        /// </summary>
        /// <value>The height.</value>
        public int Height { get; set; }

        /// <summary>
        /// Gets or sets the bits per component.
        /// Typical 8 for a color component and between 2 and 16 for a monochrome component.
        /// </summary>
        /// <value>The bits per sample.</value>
        public int BitsPerComponent { get; set; }

        /// <summary>
        /// Gets or sets the bytes per line.
        /// </summary>
        /// <value>The bytes per line.</value>
        public int BytesPerLine { get; set; }

        /// <summary>
        /// Gets or sets the component count per pixel.
        /// Typical 1 for monochrome images and 3 for color images.
        /// </summary>
        /// <value>The component count.</value>
        public int ComponentCount { get; set; }

        /// <summary>
        /// Gets or sets the allowed lossy error.
        /// </summary>
        /// <value>The allowed lossy error.</value>
        public int AllowedLossyError { get; set; }

        /// <summary>
        /// Gets or sets the interleave mode.
        /// </summary>
        /// <value>The interleave mode.</value>
        public JpegLSInterleaveMode InterleaveMode { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether CharLS will perform a RGB to BGR conversion.
        /// </summary>
        /// <value>
        /// <c>true</c> if CharLS should perform a conversion; otherwise, <c>false</c>.
        /// </value>
        public bool OutputBgr { get; set; }

        /// <summary>
        /// Gets the size of an byte array needed to hold the uncompressed pixels.
        /// </summary>
        /// <value>The size of byte array.</value>
        public int UncompressedSize
        {
            get { return Width * Height * ComponentCount * ((BitsPerComponent + 7) / 8); }
        }

        /// <summary>
        /// Returns a <see cref="System.String"/> that represents this instance.
        /// </summary>
        /// <returns>
        /// A <see cref="System.String"/> that represents this instance.
        /// </returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Width = {0}, Height = {1}, BitsPerSample = {2}, ComponentCount = {3}, AllowedLossyError = {4}",
                Width, Height, BitsPerComponent, ComponentCount, AllowedLossyError);
        }

        /// <summary>
        /// Determines whether the specified <see cref="System.Object"/> is equal to this instance.
        /// </summary>
        /// <param name="obj">The <see cref="System.Object"/> to compare with this instance.</param>
        /// <returns>
        /// <c>true</c> if the specified <see cref="System.Object"/> is equal to this instance; otherwise, <c>false</c>.
        /// </returns>
        public override bool Equals(object obj)
        {
            return Equals(obj as JpegLSMetadataInfo);
        }

        /// <summary>
        /// Determines whether the specified <see cref="JpegLSMetadataInfo"/> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="JpegLSMetadataInfo"/> to compare with this instance.</param>
        /// <returns>
        /// <c>true</c> if the specified <see cref="System.Object"/> is equal to this instance; otherwise, <c>false</c>.
        /// </returns>
        [Pure]
        public bool Equals(JpegLSMetadataInfo other)
        {
            if (other == null)
                return false;

            return Width == other.Width &&
                   Height == other.Height &&
                   ComponentCount == other.ComponentCount &&
                   BitsPerComponent == other.BitsPerComponent &&
                   AllowedLossyError == other.AllowedLossyError &&
                   InterleaveMode == other.InterleaveMode &&
                   OutputBgr == other.OutputBgr;
        }

        /// <summary>
        /// Serves as a hash function for a particular type.
        /// </summary>
        /// <returns>
        /// A hash code for the current <see cref="JpegLSMetadataInfo"/>.
        /// </returns>
        public override int GetHashCode()
        {
            unchecked
            {
                int result = Width;
                result = (result * 397) ^ Height;
                result = (result * 397) ^ BitsPerComponent;
                result = (result * 397) ^ BytesPerLine;
                result = (result * 397) ^ ComponentCount;
                result = (result * 397) ^ AllowedLossyError;
                result = (result * 397) ^ InterleaveMode.GetHashCode();
                result = (result * 397) ^ OutputBgr.GetHashCode();
                return result;
            }
        }

        internal void CopyTo(ref JlsParameters parameters)
        {
            parameters.Width = Width;
            parameters.Height = Height;
            parameters.Components = ComponentCount;
            parameters.BitsPerSample = BitsPerComponent;
            parameters.InterleaveMode = InterleaveMode;
            parameters.AllowedLossyError = AllowedLossyError;
            parameters.OutputBgr = OutputBgr;
        }
    }
}
