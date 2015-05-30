//
// (C) Jan de Vaan 2007-2010, all rights reserved. See the accompanying "License.txt" for licensed use.
//



#include "charls.h"
#include "util.h"
#include "jpegstreamreader.h"
#include "jpegstreamwriter.h"
#include "jpegmarkersegment.h"


using namespace charls;


static ApiResult CheckInput(const ByteStreamInfo& uncompressedStream, const JlsParameters& parameters)
{
    if (!uncompressedStream.rawStream && !uncompressedStream.rawData)
        return ApiResult::InvalidJlsParameters;

    if (parameters.width < 1 || parameters.width > 65535)
        return ApiResult::ParameterValueNotSupported;

    if (parameters.height < 1 || parameters.height > 65535)
        return ApiResult::ParameterValueNotSupported;

    if (uncompressedStream.rawData)
    {
        if (uncompressedStream.count < size_t(parameters.height * parameters.width * parameters.components * (parameters.bitspersample > 8 ? 2 : 1)))
            return ApiResult::UncompressedBufferTooSmall;
    }
    else if (!uncompressedStream.rawStream)
        return ApiResult::InvalidJlsParameters;

    return CheckParameterCoherent(parameters);
}


static ApiResult SystemErrorToCharLSError(const std::system_error& e)
{
    return e.code().category() == CharLSCategoryInstance() ? static_cast<ApiResult>(e.code().value()) : ApiResult::UnspecifiedFailure;
}


CHARLS_IMEXPORT(ApiResult) JpegLsEncodeStream(ByteStreamInfo compressedStreamInfo, size_t& pcbyteWritten, ByteStreamInfo rawStreamInfo, const struct JlsParameters& parameters)
{
    try
    {
        auto parameterError = CheckInput(rawStreamInfo, parameters);
        if (parameterError != ApiResult::OK)
            return parameterError;

        JlsParameters info = parameters;
        if (info.bytesperline == 0)
        {
            info.bytesperline = info.width * ((info.bitspersample + 7)/8);
            if (info.ilv != InterleaveMode::None)
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


        if (info.colorTransform != ColorTransformation::None)
        {
            writer.AddColorTransform(info.colorTransform);
        }

        if (info.ilv == InterleaveMode::None)
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
        return ApiResult::OK;
    }
    catch (const std::system_error& e)
    {
        return SystemErrorToCharLSError(e);
    }
    catch (...)
    {
        return ApiResult::UnexpectedFailure;
    }
}


CHARLS_IMEXPORT(ApiResult) JpegLsDecodeStream(ByteStreamInfo rawStream, ByteStreamInfo compressedStream, const JlsParameters* info)
{
    try
    {
        JpegStreamReader reader(compressedStream);

        if (info)
        {
            reader.SetInfo(*info);
        }

        reader.Read(rawStream);
        return ApiResult::OK;
    }
    catch (const std::system_error& e)
    {
        return SystemErrorToCharLSError(e);
    }
    catch (...)
    {
        return ApiResult::UnexpectedFailure;
    }
}


CHARLS_IMEXPORT(ApiResult) JpegLsReadHeaderStream(ByteStreamInfo rawStreamInfo, JlsParameters* pparams)
{
    try
    {
        JpegStreamReader reader(rawStreamInfo);
        reader.ReadHeader();
        reader.ReadStartOfScan(true);
        JlsParameters info = reader.GetMetadata();
        *pparams = info;
        return ApiResult::OK;
    }
    catch (const std::system_error& e)
    {
        return SystemErrorToCharLSError(e);
    }
    catch (...)
    {
        return ApiResult::UnexpectedFailure;
    }
}

extern "C"
{
    CHARLS_IMEXPORT(ApiResult) JpegLsEncode(void* destination, size_t destinationLength, size_t* bytesWritten, const void* source, size_t sourceLength, const struct JlsParameters* parameters)
    {
        if (!destination || !bytesWritten || !source || !parameters)
            return ApiResult::InvalidJlsParameters;

        ByteStreamInfo rawStreamInfo = FromByteArray(source, sourceLength);
        ByteStreamInfo compressedStreamInfo = FromByteArray(destination, destinationLength);

        return JpegLsEncodeStream(compressedStreamInfo, *bytesWritten, rawStreamInfo, *parameters);
    }


    CHARLS_IMEXPORT(ApiResult) JpegLsDecode(void* destination, size_t destinationLength, const void* source, size_t sourceLength, const struct JlsParameters* info)
    {
        ByteStreamInfo compressedStream = FromByteArray(source, sourceLength);
        ByteStreamInfo rawStreamInfo = FromByteArray(destination, destinationLength);

        return JpegLsDecodeStream(rawStreamInfo, compressedStream, info);
    }

    
    CHARLS_IMEXPORT(ApiResult) JpegLsReadHeader(const void* compressedData, size_t compressedLength, JlsParameters* pparams)
    {
        return JpegLsReadHeaderStream(FromByteArray(compressedData, compressedLength), pparams);
    }


    CHARLS_IMEXPORT(ApiResult) JpegLsVerifyEncode(const void* uncompressedData, size_t uncompressedLength, const void* compressedData, size_t compressedLength)
    {
        JlsParameters info = JlsParameters();

        auto error = JpegLsReadHeader(compressedData, compressedLength, &info);
        if (error != ApiResult::OK)
            return error;

        ByteStreamInfo rawStreamInfo = FromByteArray(uncompressedData, uncompressedLength);

        error = CheckInput(rawStreamInfo, info);

        if (error != ApiResult::OK)
            return error;

        JpegStreamWriter writer;
        if (info.jfif.Ver)
        {
            writer.AddSegment(JpegMarkerSegment::CreateJpegFileInterchangeFormatMarker(info.jfif));
        }

        writer.AddSegment(JpegMarkerSegment::CreateStartOfFrameMarker(info.width, info.height, info.bitspersample, info.components));


        if (info.ilv == InterleaveMode::None)
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

        return ApiResult::OK;
    }


    CHARLS_IMEXPORT(ApiResult) JpegLsDecodeRect(void* uncompressedData, size_t uncompressedLength, const void* compressedData, size_t compressedLength, JlsRect roi, JlsParameters* info)
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
            return ApiResult::OK;
        }
        catch (const std::system_error& e)
        {
            return SystemErrorToCharLSError(e);
        }
        catch (...)
        {
            return ApiResult::UnexpectedFailure;
        }
    }
}
