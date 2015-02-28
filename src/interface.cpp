//
// (C) Jan de Vaan 2007-2010, all rights reserved. See the accompanying "License.txt" for licensed use.
//


// Implement correct linkage for win32 dlls
#if defined(WIN32) && defined(CHARLS_DLL)
#define CHARLS_IMEXPORT(returntype) __declspec(dllexport) returntype __stdcall
#else
#define CHARLS_IMEXPORT(returntype) returntype
#endif

#include "config.h"
#include "util.h"
#include "charls.h"
#include "header.h"
#include "jpegstreamreader.h"
#include "jpegstreamwriter.h"
#include "jpegmarkersegment.h"
#include <sstream>


static JLS_ERROR CheckInput(const ByteStreamInfo& uncompressedStream, const JlsParameters& parameters)
{
    if (!uncompressedStream.rawStream && !uncompressedStream.rawData)
        return InvalidJlsParameters;

    if (parameters.width < 1 || parameters.width > 65535)
        return ParameterValueNotSupported;

    if (parameters.height < 1 || parameters.height > 65535)
        return ParameterValueNotSupported;

    if (uncompressedStream.rawData)
    {
        if (uncompressedStream.count < size_t(parameters.height * parameters.width * parameters.components * (parameters.bitspersample > 8 ? 2 : 1)))
            return UncompressedBufferTooSmall;
    }
    else if (!uncompressedStream.rawStream)
        return InvalidJlsParameters;

    return CheckParameterCoherent(parameters);
}


static JLS_ERROR SystemErrorToCharLSError(const std::system_error& e)
{
    return e.code().category() == CharLSCategoryInstance() ? static_cast<JLS_ERROR>(e.code().value()) : UnspecifiedFailure;
}


CHARLS_IMEXPORT(JLS_ERROR) JpegLsEncodeStream(ByteStreamInfo compressedStreamInfo, size_t& pcbyteWritten, ByteStreamInfo rawStreamInfo, const struct JlsParameters& parameters)
{
    try
    {
        JLS_ERROR parameterError = CheckInput(rawStreamInfo, parameters);
        if (parameterError != OK)
            return parameterError;

        JlsParameters info = parameters;
        if (info.bytesperline == 0)
        {
            info.bytesperline = info.width * ((info.bitspersample + 7)/8);
            if (info.ilv != ILV_NONE)
            {
                info.bytesperline *= info.components;
            }
        }

        JpegStreamWriter writer;
        if (info.jfif.Ver)
        {
            writer.AddSegment(JpegMarkerSegment::CreateJpegFileInterchangeFormatMarker(info.jfif));
        }

        writer.AddSegment(JpegMarkerSegment::CreateStartOfFrameMarker(info.width, info.height, info.bitspersample, info.components));


        if (info.colorTransform != 0)
        {
            writer.AddColorTransform(info.colorTransform);
        }

        if (info.ilv == ILV_NONE)
        {
            int32_t cbyteComp = info.width * info.height * ((info.bitspersample + 7) / 8);
            for (int32_t component = 0; component < info.components; ++component)
            {
                writer.AddScan(rawStreamInfo, info);
                SkipBytes(&rawStreamInfo, cbyteComp);
            }
        }
        else
        {
            writer.AddScan(rawStreamInfo, info);
        }

        writer.Write(compressedStreamInfo);
        pcbyteWritten = writer.GetBytesWritten();
        return OK;
    }
    catch (const std::system_error& e)
    {
        return SystemErrorToCharLSError(e);
    }
    catch (...)
    {
        return UnexpectedFailure;
    }
}


CHARLS_IMEXPORT(JLS_ERROR) JpegLsDecodeStream(ByteStreamInfo rawStream, ByteStreamInfo compressedStream, JlsParameters* info)
{
    try
    {
        JpegStreamReader reader(compressedStream);

        if (info)
        {
            reader.SetInfo(*info);
        }

        reader.Read(rawStream);
        return OK;
    }
    catch (const std::system_error& e)
    {
        return SystemErrorToCharLSError(e);
    }
    catch (...)
    {
        return UnexpectedFailure;
    }
}


CHARLS_IMEXPORT(JLS_ERROR) JpegLsReadHeaderStream(ByteStreamInfo rawStreamInfo, JlsParameters* pparams)
{
    try
    {
        JpegStreamReader reader(rawStreamInfo);
        reader.ReadHeader();
        reader.ReadStartOfScan(true);
        JlsParameters info = reader.GetMetadata();
        *pparams = info;
        return OK;
    }
    catch (const std::system_error& e)
    {
        return SystemErrorToCharLSError(e);
    }
    catch (...)
    {
        return UnexpectedFailure;
    }
}

extern "C"
{
    CHARLS_IMEXPORT(JLS_ERROR) JpegLsEncode(void* destination, size_t destinationLength, size_t* bytesWritten, const void* source, size_t sourceLength, const struct JlsParameters* parameters)
    {
        if (!destination || !bytesWritten || !source || !parameters)
            return InvalidJlsParameters;

        ByteStreamInfo rawStreamInfo = FromByteArray(source, sourceLength);
        ByteStreamInfo compressedStreamInfo = FromByteArray(destination, destinationLength);

        return JpegLsEncodeStream(compressedStreamInfo, *bytesWritten, rawStreamInfo, *parameters);
    }


    CHARLS_IMEXPORT(JLS_ERROR) JpegLsDecode(void* uncompressedData, size_t uncompressedLength, const void* compressedData, size_t compressedLength, JlsParameters* info)
    {
        ByteStreamInfo compressedStream = FromByteArray(compressedData, compressedLength);
        ByteStreamInfo rawStreamInfo = FromByteArray(uncompressedData, uncompressedLength);

        return JpegLsDecodeStream(rawStreamInfo, compressedStream, info);
    }

    
    CHARLS_IMEXPORT(JLS_ERROR) JpegLsReadHeader(const void* compressedData, size_t compressedLength, JlsParameters* pparams)
    {
        return JpegLsReadHeaderStream(FromByteArray(compressedData, compressedLength), pparams);
    }


    CHARLS_IMEXPORT(JLS_ERROR) JpegLsVerifyEncode(const void* uncompressedData, size_t uncompressedLength, const void* compressedData, size_t compressedLength)
    {
        JlsParameters info = JlsParameters();

        JLS_ERROR error = JpegLsReadHeader(compressedData, compressedLength, &info);
        if (error != OK)
            return error;

        ByteStreamInfo rawStreamInfo = FromByteArray(uncompressedData, uncompressedLength);

        error = CheckInput(rawStreamInfo, info);

        if (error != OK)
            return error;

        JpegStreamWriter writer;
        if (info.jfif.Ver)
        {
            writer.AddSegment(JpegMarkerSegment::CreateJpegFileInterchangeFormatMarker(info.jfif));
        }

        writer.AddSegment(JpegMarkerSegment::CreateStartOfFrameMarker(info.width, info.height, info.bitspersample, info.components));


        if (info.ilv == ILV_NONE)
        {
            int32_t fieldLength = info.width * info.height * ((info.bitspersample + 7) / 8);
            for (int32_t component = 0; component < info.components; ++component)
            {
                writer.AddScan(rawStreamInfo, info);
                SkipBytes(&rawStreamInfo, fieldLength);
            }
        }
        else
        {
            writer.AddScan(rawStreamInfo, info);
        }

        std::vector<uint8_t> rgbyteCompressed(compressedLength + 16);

        memcpy(&rgbyteCompressed[0], compressedData, compressedLength);

        writer.EnableCompare(true);
        writer.Write(FromByteArray(&rgbyteCompressed[0], rgbyteCompressed.size()));

        return OK;
    }


    CHARLS_IMEXPORT(JLS_ERROR) JpegLsDecodeRect(void* uncompressedData, size_t uncompressedLength, const void* compressedData, size_t compressedLength, JlsRect roi, JlsParameters* info)
    {
        try
        {
            ByteStreamInfo compressedStream = FromByteArray(compressedData, compressedLength);
            JpegStreamReader reader(compressedStream);

            ByteStreamInfo rawStreamInfo = FromByteArray(uncompressedData, uncompressedLength);

            if (info)
            {
                reader.SetInfo(*info);
            }

            reader.SetRect(roi);

            reader.Read(rawStreamInfo);
            return OK;
        }
        catch (const std::system_error& e)
        {
            return SystemErrorToCharLSError(e);
        }
        catch (...)
        {
            return UnexpectedFailure;
        }
    }
}
