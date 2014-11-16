// 
// (C) Jan de Vaan 2007-2010, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 

#include "config.h"
#include "util.h"
#include "header.h"
#include "jpegstreamreader.h"
#include "jpegstreamwriter.h"
#include "jpegmarkersegment.h"
#include "jpegimagedatasegment.h"
#include "decoderstrategy.h"
#include "encoderstrategy.h"
#include <memory>


// JFIF\0
uint8_t jfifID [] = { 'J', 'F', 'I', 'F', '\0' };


LONG CLAMP(LONG i, LONG j, LONG MAXVAL)
{
	if (i > MAXVAL || i < j)
		return j;

	return i;
}


JlsCustomParameters ComputeDefault(LONG MAXVAL, LONG NEAR)
{
	JlsCustomParameters preset = JlsCustomParameters();

	LONG FACTOR = (MIN(MAXVAL, 4095) + 128)/256;

	preset.T1 = CLAMP(FACTOR * (BASIC_T1 - 2) + 2 + 3*NEAR, NEAR + 1, MAXVAL);
	preset.T2 = CLAMP(FACTOR * (BASIC_T2 - 3) + 3 + 5*NEAR, preset.T1, MAXVAL);
	preset.T3 = CLAMP(FACTOR * (BASIC_T3 - 4) + 4 + 7*NEAR, preset.T2, MAXVAL);
	preset.MAXVAL = MAXVAL;
	preset.RESET = BASIC_RESET;
	return preset;
}


JLS_ERROR CheckParameterCoherent(const JlsParameters& parameters)
{
	if (parameters.bitspersample < 2 || parameters.bitspersample > 16)
		return ParameterValueNotSupported;

	if (parameters.ilv < 0 || parameters.ilv > 2)
		return InvalidCompressedData;

	switch (parameters.components)
	{
		case 4: return parameters.ilv == ILV_SAMPLE ? ParameterValueNotSupported : OK;
		case 3: return OK;
		case 1: return parameters.ilv != ILV_NONE ? ParameterValueNotSupported : OK;
		case 0: return InvalidJlsParameters;

		default: return parameters.ilv != ILV_NONE ? ParameterValueNotSupported : OK;
	}
}


void JpegImageDataSegment::Serialize(JpegStreamWriter& streamWriter)
{
	JlsParameters info = _info;
	info.components = _ccompScan;
	auto codec = JlsCodecFactory<EncoderStrategy>().GetCodec(info, _info.custom);
	std::unique_ptr<ProcessLine> processLine(codec->CreateProcess(_rawStreamInfo));
	ByteStreamInfo compressedData = streamWriter.OutputStream();
	size_t cbyteWritten = codec->EncodeScan(std::move(processLine), &compressedData, streamWriter._bCompare ? streamWriter.GetPos() : nullptr);
	streamWriter.Seek(cbyteWritten);
}


JpegStreamReader::JpegStreamReader(ByteStreamInfo byteStreamInfo) :
	_byteStream(byteStreamInfo),
	_bCompare(false),
	_info(),
	_rect()
{
}


void JpegStreamReader::Read(ByteStreamInfo rawPixels)
{
	ReadHeader();

	JLS_ERROR error = CheckParameterCoherent(_info);
	if (error != OK)
		throw JlsException(error);

	if (_rect.Width <= 0)
	{
		_rect.Width = _info.width;
		_rect.Height = _info.height;
	}

	int64_t bytesPerPlane = (int64_t)(_rect.Width) * _rect.Height * ((_info.bitspersample + 7)/8);

	if (rawPixels.rawData && int64_t(rawPixels.count) < bytesPerPlane * _info.components)
		throw JlsException(UncompressedBufferTooSmall);

	int componentIndex = 0;
	
	while (componentIndex < _info.components)
	{
		ReadStartOfScan(componentIndex == 0);

		std::unique_ptr<DecoderStrategy> qcodec = JlsCodecFactory<DecoderStrategy>().GetCodec(_info, _info.custom);
		std::unique_ptr<ProcessLine> processLine(qcodec->CreateProcess(rawPixels));
		qcodec->DecodeScan(std::move(processLine), _rect, &_byteStream, _bCompare); 
		SkipBytes(&rawPixels, (size_t)bytesPerPlane);

		if (_info.ilv != ILV_NONE)
			return;

		componentIndex += 1;
	}
}


void JpegStreamReader::ReadNBytes(std::vector<char>& dst, int byteCount)
{
	for (int i = 0; i < byteCount; ++i)
	{
		dst.push_back((char)ReadByte());
	}
}


void JpegStreamReader::ReadHeader()
{
	if (ReadByte() != 0xFF)
		throw JlsException(MissingJpegMarkerStart);

	if (ReadByte() != JPEG_SOI)
		throw JlsException(InvalidCompressedData);

	for (;;)
	{
		if (ReadByte() != 0xFF)
			throw JlsException(MissingJpegMarkerStart);

        uint8_t marker = ReadByte();
		if (marker == JPEG_SOS)
			return;

		LONG cbyteMarker = ReadWord();

		int bytesRead = ReadMarker(marker) + 2;

		int paddingToRead = cbyteMarker - bytesRead;

		if (paddingToRead < 0)
			throw JlsException(InvalidCompressedData);

		for (int i = 0; i < paddingToRead; ++i)
		{
			ReadByte();
		}
	}
}


int JpegStreamReader::ReadMarker(uint8_t marker)
{
	switch (marker)
	{
		case JPEG_SOF_55: return ReadStartOfFrame();
		case JPEG_COM: return ReadComment();
		case JPEG_LSE: return ReadPresetParameters();
		case JPEG_APP0: return 0;
		case JPEG_APP7: return ReadColorSpace();
		case JPEG_APP8: return ReadColorXForm();
		case JPEG_SOF_0:
		case JPEG_SOF_1:
		case JPEG_SOF_2:
		case JPEG_SOF_3:
		case JPEG_SOF_5:
		case JPEG_SOF_6:
		case JPEG_SOF_7:
		case JPEG_SOF_9:
		case JPEG_SOF_10:
		case JPEG_SOF_11:
			throw JlsException(UnsupportedEncoding);

		// Other tags not supported (among which DNL DRI)
		default: throw JlsException(UnknownJpegMarker);
	}
}


int JpegStreamReader::ReadPresetParameters()
{
	LONG type = ReadByte();

	switch (type)
	{
	case 1:
		{
			_info.custom.MAXVAL = ReadWord();
			_info.custom.T1 = ReadWord();
			_info.custom.T2 = ReadWord();
			_info.custom.T3 = ReadWord();
			_info.custom.RESET = ReadWord();
			return 11;
		}
	}

	return 1;
}


void JpegStreamReader::ReadStartOfScan(bool firstComponent)
{
	if (!firstComponent)
	{
		if (ReadByte() != 0xFF)
			throw JlsException(MissingJpegMarkerStart);
		if (ReadByte() != JPEG_SOS)
			throw JlsException(InvalidCompressedData); // TODO: throw more specific error code.
	}
	int length = ReadByte(); //length
	length = length * 256 + ReadByte(); // TODO: do something with 'length' or remove it.

	LONG componentCount = ReadByte();
	if (componentCount != 1 && componentCount != _info.components)
		throw JlsException(ParameterValueNotSupported);

	for (LONG i = 0; i < componentCount; ++i)
	{
		ReadByte();
		ReadByte();
	}
	_info.allowedlossyerror = ReadByte();
	_info.ilv = interleavemode(ReadByte());
	if (!(_info.ilv == ILV_NONE || _info.ilv == ILV_LINE || _info.ilv == ILV_SAMPLE))
		throw JlsException(InvalidCompressedData); // TODO: throw more specific error code.
	if (ReadByte() != 0)
		throw JlsException(InvalidCompressedData); // TODO: throw more specific error code.

	if(_info.bytesperline == 0)
	{
		int width = _rect.Width != 0 ? _rect.Width : _info.width;
		int components = _info.ilv == ILV_NONE ? 1 : _info.components;
		_info.bytesperline = components * width * ((_info.bitspersample + 7)/8);
	}
}


int JpegStreamReader::ReadComment()
{
	return 0;
}


void JpegStreamReader::ReadJfif()
{
	for(int i = 0; i < (int)sizeof(jfifID); i++)
	{
		if(jfifID[i] != ReadByte())
			return;
	}
	_info.jfif.Ver   = ReadWord();

	// DPI or DPcm
	_info.jfif.units = ReadByte();
	_info.jfif.XDensity = ReadWord();
	_info.jfif.YDensity = ReadWord();

	// thumbnail
	_info.jfif.Xthumb = ReadByte();
	_info.jfif.Ythumb = ReadByte();
	if(_info.jfif.Xthumb > 0 && _info.jfif.pdataThumbnail) 
	{
		std::vector<char> tempbuff((char*)_info.jfif.pdataThumbnail, (char*)_info.jfif.pdataThumbnail+3*_info.jfif.Xthumb*_info.jfif.Ythumb);
		ReadNBytes(tempbuff, 3*_info.jfif.Xthumb*_info.jfif.Ythumb);
	}
}


int JpegStreamReader::ReadStartOfFrame()
{
	_info.bitspersample = ReadByte();
	int cline = ReadWord();
	int ccol = ReadWord();
	_info.width = ccol;
	_info.height = cline;
	_info.components= ReadByte();
	return 6;
}


uint8_t JpegStreamReader::ReadByte()
{
	if (_byteStream.rawStream)
        return (uint8_t) _byteStream.rawStream->sbumpc();

	if (_byteStream.count == 0)
		throw JlsException(InvalidCompressedData);

    uint8_t value = _byteStream.rawData[0];
	
	SkipBytes(&_byteStream, 1);

	return value;
}


int JpegStreamReader::ReadWord()
{
	int i = ReadByte() * 256;
	return i + ReadByte();
}


int JpegStreamReader::ReadColorSpace()
{
	return 0;
}


int JpegStreamReader::ReadColorXForm()
{
	std::vector<char> sourceTag;
	ReadNBytes(sourceTag, 4);

	if(strncmp(&sourceTag[0],"mrfx", 4) != 0)
		return 4;

	int xform = ReadByte();
	switch(xform) 
	{
		case COLORXFORM_NONE:
		case COLORXFORM_HP1:
		case COLORXFORM_HP2:
		case COLORXFORM_HP3:
			_info.colorTransform = xform;
			return 5;
		case COLORXFORM_RGB_AS_YUV_LOSSY:
		case COLORXFORM_MATRIX:
			throw JlsException(ImageTypeNotSupported);
		default:
			throw JlsException(InvalidCompressedData);
	}
}
