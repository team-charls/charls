// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

namespace CharLS
{
    /// <summary>
    /// Defines the result codes that the native CharLS implementation can return (see enumeration JLS_ERROR in publicTypes.h).
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
        InvalidJlsParameters = 1,

        /// <summary>
        /// The parameter value not supported.
        /// </summary>
        ParameterValueNotSupported = 2,

        /// <summary>
        /// The uncompressed buffer is too small to hold all the output.
        /// </summary>
        UncompressedBufferTooSmall = 3,

        /// <summary>
        /// The compressed buffer too small, more input data was expected.
        /// </summary>
        CompressedBufferTooSmall = 4,

        /// <summary>
        /// This error is returned when the encoded bit stream contains a general structural problem.
        /// </summary>
        InvalidCompressedData = 5,

        /// <summary>
        /// Too much compressed data. The decoding process is ready but the input buffer still contains encoded data.
        /// </summary>
        TooMuchCompressedData = 6,

        /// <summary>
        /// This error is returned when the bit stream is encoded with an option that is not supported by this implementation.
        /// </summary>
        ImageTypeNotSupported = 7,

        /// <summary>
        /// The bit depth for transformation is not supported.
        /// </summary>
        UnsupportedBitDepthForTransform = 8,

        /// <summary>
        /// The color transform is not supported.
        /// </summary>
        UnsupportedColorTransform = 9,

        /// <summary>
        /// This error is returned when an encoded frame is found that is not encoded with the JPEG-LS algorithm.
        /// </summary>
        UnsupportedEncoding = 10,

        /// <summary>
        /// This error is returned when an unknown JPEG marker code is detected in the encoded bit stream.
        /// </summary>
        UnknownJpegMarker = 11,

        /// <summary>
        /// This error is returned when the algorithm expect a 0xFF code (indicates start of a JPEG marker) but none was found.
        /// </summary>
        MissingJpegMarkerStart = 12,

        /// <summary>
        /// This error is returned when the implementation detected a failure, but no specific error is available.
        /// </summary>
        UnspecifiedFailure = 13,

        /// <summary>
        /// This error is returned when the implementation encountered a failure it didn't expect. No guarantees can be given for the state after this error.
        /// </summary>
        UnexpectedFailure = 14,

        /// <summary>
        /// This error is returned when the first JPEG marker is not the SOI (Start Of Image) marker.
        /// </summary>
        StartOfImageMarkerNotFound = 15,

        /// <summary>
        /// This error is returned when the SOF JPEG marker is not found before the SOS (Start of Scan) marker.
        /// </summary>
        StartOfFrameMarkerNotFound = 16,

        /// <summary>
        /// This error is returned when the segment size of a marker segment is invalid.
        /// </summary>
        InvalidMarkerSegmentSize = 17,

        /// <summary>
        /// This error is returned when the stream contains more then one SOI (Start Of Image) marker.
        /// </summary>
        DuplicateStartOfImageMarker = 18,

        /// <summary>
        /// This error is returned when the stream contains more then one SOF (Start Of Frame) marker.
        /// </summary>
        DuplicateStartOfFrameMarker = 19
    }
}
