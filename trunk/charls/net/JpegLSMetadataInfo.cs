//
// (C) Jan de Vaan 2007-2011, all rights reserved. See the accompanying "License.txt" for licensed use.
//

namespace CharLS
{
    /// <summary>
    /// Contains meta information about a compressed JPEG-LS stream or info how to compress.
    /// </summary>
    public class JpegLSMetadataInfo
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="JpegLSMetadataInfo"/> class.
        /// </summary>
        public JpegLSMetadataInfo()
        {
        }

        internal JpegLSMetadataInfo(ref JlsParameters jlsParameters)
        {
            Width = jlsParameters.width;
            Height = jlsParameters.height;
            ComponentCount = jlsParameters.components;
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
    }
}
