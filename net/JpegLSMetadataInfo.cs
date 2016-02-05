//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use.
//

using System;
using System.Diagnostics.Contracts;
using System.Globalization;
using System.IO;

namespace CharLS
{
    /// <summary>
    /// Contains meta information about a compressed JPEG-LS stream or info how to compress.
    /// </summary>
    /// <remarks>
    /// This 'info' class is used for 2 purposes:
    /// 1) The JPEG-LS algorithm needs information which type of bitmap is stored in an array bytes.
    /// This information is needed to correctly compress (encode) the pixels to a JPEG-LS byte stream.
    /// 2) Information which kind of bitmap is stored in a JPEG-LS byte stream is stored in the JPEG-LS byte stream.
    /// This information can be extracted without decompressing (decoding) the byte stream. This information is often
    /// required to prepare output buffers to received the decompressed byte stream.
    /// </remarks>
    public sealed class JpegLSMetadataInfo : IEquatable<JpegLSMetadataInfo>
    {
        private int width;
        private int height;
        private int bitsPerComponent;
        private int componentCount;

        /// <summary>
        /// Initializes a new instance of the <see cref="JpegLSMetadataInfo"/> class.
        /// </summary>
        /// <remarks>
        /// Initializes a minimal <see cref="JpegLSMetadataInfo"/> instance with:
        /// Width = 1
        /// Height = 1
        /// BitsPerComponent = 2
        /// ComponentCount = 1
        /// </remarks>
        public JpegLSMetadataInfo()
            : this(1, 1, 2, 1)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="JpegLSMetadataInfo"/> class.
        /// </summary>
        /// <param name="width">The width of the bitmap.</param>
        /// <param name="height">The height of the bitmap.</param>
        /// <param name="bitsPerComponent">The number of bits per component. Typical 8 for color and 2 to 16 for monochrome bitmaps.</param>
        /// <param name="componentCount">The component count. Typical 1 for monochrome images and 3 for color images.</param>
        public JpegLSMetadataInfo(int width, int height, int bitsPerComponent, int componentCount)
        {
            if (width <= 0)
                throw new ArgumentException("width <= 0", nameof(width));
            if (height <= 0)
                throw new ArgumentException("height <= 0", nameof(height));
            if (bitsPerComponent < 2)
                throw new ArgumentException("bitsPerComponent < 2", nameof(bitsPerComponent));
            if (componentCount <= 0)
                throw new ArgumentException("componentCount <= 0", nameof(componentCount));
            Contract.EndContractBlock();

            this.width = width;
            this.height = height;
            this.bitsPerComponent = bitsPerComponent;
            this.componentCount = componentCount;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="JpegLSMetadataInfo"/> class.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <exception cref="InvalidDataException">Thrown when one the values are out of bounds.</exception>
        internal JpegLSMetadataInfo(ref JlsParameters parameters)
        {
            if (parameters.Width < 1)
                throw new InvalidDataException("parameters.Width < 1");
            if (parameters.Height < 1)
                throw new InvalidDataException("parameters.Height < 1");
            if (parameters.BitsPerSample < 2)
                throw new InvalidDataException("parameters.BitsPerSample < 2");
            if (parameters.Components < 1)
                throw new InvalidDataException("parameters.Components < 1");

            width = parameters.Width;
            height = parameters.Height;
            componentCount = parameters.Components;
            bitsPerComponent = parameters.BitsPerSample;
            AllowedLossyError = parameters.AllowedLossyError;
            InterleaveMode = parameters.InterleaveMode;
        }

        /// <summary>
        /// Gets or sets the width of the image in pixels.
        /// </summary>
        /// <value>The width.</value>
        public int Width
        {
            get
            {
                Contract.Ensures(Contract.Result<int>() > 0);
                return width;
            }

            set
            {
                if (value <= 0)
                    throw new ArgumentException("value <= 0", nameof(value));
                Contract.EndContractBlock();
                width = value;
            }
        }

        /// <summary>
        /// Gets or sets the height of the image in pixels.
        /// </summary>
        /// <value>The height.</value>
        public int Height
        {
            get
            {
                Contract.Ensures(Contract.Result<int>() > 0);
                return height;
            }

            set
            {
                if (value <= 0)
                    throw new ArgumentException("value <= 0", nameof(value));
                Contract.EndContractBlock();
                height = value;
            }
        }

        /// <summary>
        /// Gets or sets the bits per component.
        /// Typical 8 for a color component and between 2 and 16 for a monochrome component.
        /// </summary>
        /// <value>The bits per sample.</value>
        public int BitsPerComponent
        {
            get
            {
                Contract.Ensures(Contract.Result<int>() > 1);
                return bitsPerComponent;
            }

            set
            {
                if (value < 2)
                    throw new ArgumentException("value < 2", nameof(value));
                Contract.EndContractBlock();
                bitsPerComponent = value;
            }
        }

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
        public int ComponentCount
        {
            get
            {
                Contract.Ensures(Contract.Result<int>() > 0);
                return componentCount;
            }

            set
            {
                if (value < 1)
                    throw new ArgumentException("value < 1", nameof(value));
                Contract.EndContractBlock();
                componentCount = value;
            }
        }

        /// <summary>
        /// Gets or sets the allowed error value for non-lossless compression.
        /// </summary>
        /// <value>The allowed error value.</value>
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
            get
            {
                Contract.Ensures(Contract.Result<int>() > 0);
                var result = Width * Height * ComponentCount * ((BitsPerComponent + 7) / 8);
                Contract.Assume(result > 0);
                return result;
            }
        }

        /// <summary>
        /// Returns a <see cref="string"/> that represents this instance.
        /// </summary>
        /// <returns>
        /// A <see cref="string"/> that represents this instance.
        /// </returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Width = {0}, Height = {1}, BitsPerSample = {2}, ComponentCount = {3}, AllowedLossyError = {4}",
                Width, Height, BitsPerComponent, ComponentCount, AllowedLossyError);
        }

        /// <summary>
        /// Determines whether the specified <see cref="object"/> is equal to this instance.
        /// </summary>
        /// <param name="obj">The <see cref="object"/> to compare with this instance.</param>
        /// <returns>
        /// <c>true</c> if the specified <see cref="object"/> is equal to this instance; otherwise, <c>false</c>.
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
        /// <c>true</c> if the specified <see cref="object"/> is equal to this instance; otherwise, <c>false</c>.
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

        [ContractInvariantMethod]
        private void ObjectInvariant()
        {
            Contract.Invariant(width > 0);
            Contract.Invariant(height > 0);
            Contract.Invariant(bitsPerComponent > 1);
            Contract.Invariant(componentCount > 0);
        }
    }
}
