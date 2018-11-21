// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

namespace CharLS
{
    /// <summary>
    /// Defines the result codes that the native CharLS implementation can return (see enumeration charls::jpegls_errc in public_types.h).
    /// </summary>
    public enum JpegLSError
    {
        /// <summary>
        /// The operation completed without errors.
        /// </summary>
        None = 0,

        /// <summary>
        /// This error is returned when one of the arguments is invalid and no specific reason is available.
        /// </summary>
        InvalidArgument = 1,

        /// <summary>
        /// The parameter value not supported.
        /// </summary>
        ParameterValueNotSupported = 2,

        /// <summary>
        /// The destination buffer is too small to hold all the output.
        /// </summary>
        DestinationBufferTooSmall = 3,

        /// <summary>
        /// The source buffer is too small, more input data was expected.
        /// </summary>
        SourceBufferTooSmall = 4,

        /// <summary>
        /// This error is returned when the encoded bit stream contains a general structural problem.
        /// </summary>
        InvalidEncodedData = 5,

        /// <summary>
        /// Too much compressed data. The decoding process is ready but the input buffer still contains encoded data.
        /// </summary>
        TooMuchEncodedData = 6,

        /// <summary>
        /// The bit depth for transformation is not supported.
        /// </summary>
        BitDepthForTransformNotSupported = 8,

        /// <summary>
        /// The color transform is not supported.
        /// </summary>
        ColorTransformNotSupported = 9,

        /// <summary>
        /// This error is returned when an encoded frame is found that is not encoded with the JPEG-LS algorithm.
        /// </summary>
        EncodingNotSupported = 10,

        /// <summary>
        /// This error is returned when an unknown JPEG marker code is detected in the encoded bit stream.
        /// </summary>
        UnknownJpegMarkerFound = 11,

        /// <summary>
        /// This error is returned when the algorithm expect a 0xFF code (indicates start of a JPEG marker) but none was found.
        /// </summary>
        JpegMarkerStartByteNotFound = 12,

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
        DuplicateStartOfFrameMarker = 19,

        /// <summary>
        /// This error is returned when the stream contains an unexpected EOI marker.
        /// </summary>
        UnexpectedEndOfImageMarker = 20,

        /// <summary>
        /// This error is returned when the stream contains an invalid type parameter in the JPEG-LS segment.
        /// </summary>
        InvalidJpeglsPresetParameterType = 21,

        /// <summary>
        /// This error is returned when the stream contains an unsupported type parameter in the JPEG-LS segment.
        /// </summary>
        JpeglsPresetExtendedParameterTypeNotSupported = 22,

        /// <summary>
        /// The argument for the width parameter is outside the range [1, 65535].
        /// </summary>
        InvalidArgumentWidth = 100,

        /// <summary>
        /// The argument for the height parameter is outside the range [1, 65535].
        /// </summary>
        InvalidArgumentHeight = 101,

        /// <summary>
        /// The argument for the component count parameter is outside the range [1, 255].
        /// </summary>
        InvalidArgumentComponentCount = 102,

        /// <summary>
        /// The argument for the bit per sample parameter is outside the range [2, 16].
        /// </summary>
        InvalidArgumentBitsPerSample = 103,

        /// <summary>
        /// The argument for the interleave mode is not (None, Sample, Line) or invalid in combination with component count.
        /// </summary>
        InvalidArgumentInterleaveMode = 104,

        /// <summary>
        /// The destination buffer or stream is not set.
        /// </summary>
        InvalidArgumentDestination = 105,

        /// <summary>
        /// The source buffer or stream is not set.
        /// </summary>
        InvalidArgumentSource = 106,

        /// <summary>
        /// The arguments for the thumbnail and the dimensions don't match.
        /// </summary>
        InvalidArgumentThumbnail = 107,

        /// <summary>
        /// This error is returned when the width parameter is defined more then once in an incompatible way.
        /// </summary>
        InvalidParameterWidth = 200,

        /// <summary>
        /// This error is returned when the height parameter is defined more then once in an incompatible way.
        /// </summary>
        InvalidParameterHeight = 201,

        /// <summary>
        /// This error is returned when the stream contains a component count parameter outside the range [1,255]
        /// </summary>
        InvalidParameterComponentCount = 202,

        /// <summary>
        /// This error is returned when the stream contains a bits per sample (sample precision) parameter outside the range [2,16]
        /// </summary>
        InvalidParameterBitsPerSample = 203,

        /// <summary>
        /// This error is returned when the stream contains an interleave mode (ILV) parameter outside the range [0, 2]
        /// </summary>
        InvalidParameterInterleaveMode = 204
    }
}
