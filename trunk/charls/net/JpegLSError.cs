//
// (C) Jan de Vaan 2007-2011, all rights reserved. See the accompanying "License.txt" for licensed use.
//

namespace CharLS
{
    internal enum JpegLSError
    {
        None = 0,
        InvalidJlsParameters,
        ParameterValueNotSupported,
        UncompressedBufferTooSmall,
        CompressedBufferTooSmall,
        InvalidCompressedData,
        TooMuchCompressedData,
        ImageTypeNotSupported,
        UnsupportedBitDepthForTransform,
        UnsupportedColorTransform
    }
}
