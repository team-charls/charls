//
// (C) Jan de Vaan 2007-2011, all rights reserved. See the accompanying "License.txt" for licensed use.
//

namespace CharLS
{
    /// <summary>
    /// Defines the result codes that the native CharLS implementation can return (see enumeration JLS_ERROR in publicTypes.h)
    /// </summary>
    public enum JpegLSError
    {
        /// <summary>
        /// The operation completed without errors.
        /// </summary>
        None = 0,

        /// <summary>
        /// One of the JLS parameters is invalid.
        /// </summary>
        InvalidJlsParameters,

        /// <summary>
        /// The parameter value not supported
        /// </summary>
        ParameterValueNotSupported,

        /// <summary>
        /// The uncompressed buffer is too small to hold all the output.
        /// </summary>
        UncompressedBufferTooSmall,

        /// <summary>
        /// The compressed buffer too small, more input data was expected.
        /// </summary>
        CompressedBufferTooSmall,

        /// <summary>
        /// This error is returned when the encoded bit stream contains a general structural problem.
        /// </summary>
        InvalidCompressedData,

        /// <summary>
        /// The too much compressed data.
        /// </summary>
        TooMuchCompressedData,

        /// <summary>
        /// This error is returned when the bit stream is encoded with an option that is not supported by this implementation.
        /// </summary>
        ImageTypeNotSupported,

        /// <summary>
        /// The bit depth for transformation is not supported.
        /// </summary>
        UnsupportedBitDepthForTransform,

        /// <summary>
        /// The color transform is not supported.
        /// </summary>
        UnsupportedColorTransform,

        /// <summary>
        /// This error is returned when an encoded frame is found that is not encoded with the JPEG-LS algorithm.
        /// </summary>
        UnsupportedEncoding,

        /// <summary>
        /// This error is returned when an unknown JPEG marker code is detected in the encoded bit stream.
        /// </summary>
        UnknownJpegMarker,

        /// <summary>
        /// This error is returned when the algorithm expect a 0xFF code (indicates start of a JPEG marker) but none was found.
        /// </summary>
        MissingJpegMarkerStart
    }
}
