//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use. 
//

#include "jpegmarkersegment.h"
#include "header.h"
#include "util.h"
#include <vector>
#include <cstdint>


JpegMarkerSegment* JpegMarkerSegment::CreateStartOfFrameMarker(int width, int height, LONG bitsPerSample, LONG componentCount)
{
	ASSERT(width >= 0 && width <= UINT16_MAX);
	ASSERT(height >= 0 && height <= UINT16_MAX);
	ASSERT(bitsPerSample > 0 && bitsPerSample <= UINT8_MAX);
	ASSERT(componentCount > 0 && componentCount <= (UINT8_MAX - 1));

	// Create a Frame Header as defined in T.87, C.2.2 and T.81, B.2.2
	std::vector<uint8_t> content;
	content.push_back(static_cast<uint8_t>(bitsPerSample)); // P = Sample precision
	push_back(content, static_cast<uint16_t>(height));    // Y = Number of lines
	push_back(content, static_cast<uint16_t>(width));    // X = Number of samples per line

	// Components
	content.push_back(static_cast<uint8_t>(componentCount)); // Nf = Number of image components in frame
	for (int component = 0; component < componentCount; ++component)
	{
		// Component Specification parameters
		content.push_back(static_cast<uint8_t>(component + 1)); // Ci = Component identifier
		content.push_back(0x11);                                // Hi + Vi = Horizontal sampling factor + Vertical sampling factor
		content.push_back(0);                                   // Tqi = Quantization table destination selector (reserved for JPEG-LS, should be set to 0)
	}

	return new JpegMarkerSegment(JPEG_SOF_55, std::move(content));
}


JpegMarkerSegment* JpegMarkerSegment::CreateJpegFileInterchangeFormatMarker(const JfifParameters& jfifParameters)
{
	BYTE jfifID [] = { 'J', 'F', 'I', 'F', '\0' };

	std::vector<BYTE> rgbyte;
	for (int i = 0; i < (int)sizeof(jfifID); i++)
	{
		rgbyte.push_back(jfifID[i]);
	}

	push_back(rgbyte, (USHORT) jfifParameters.Ver);

	rgbyte.push_back(jfifParameters.units);
	push_back(rgbyte, (USHORT) jfifParameters.XDensity);
	push_back(rgbyte, (USHORT) jfifParameters.YDensity);

	// thumbnail
	rgbyte.push_back((BYTE) jfifParameters.Xthumb);
	rgbyte.push_back((BYTE) jfifParameters.Ythumb);
	if (jfifParameters.Xthumb > 0)
	{
		if (jfifParameters.pdataThumbnail)
			throw JlsException(InvalidJlsParameters);

		rgbyte.insert(rgbyte.end(), (BYTE*) jfifParameters.pdataThumbnail, (BYTE*) jfifParameters.pdataThumbnail + 3 * jfifParameters.Xthumb * jfifParameters.Ythumb);
	}

	return new JpegMarkerSegment(JPEG_APP0, std::move(rgbyte));
}


JpegMarkerSegment* JpegMarkerSegment::CreateJpegLSExtendedParametersMarker(const JlsCustomParameters& customParameters)
{
	std::vector<BYTE> rgbyte;

	rgbyte.push_back(1);
	push_back(rgbyte, (USHORT) customParameters.MAXVAL);
	push_back(rgbyte, (USHORT) customParameters.T1);
	push_back(rgbyte, (USHORT) customParameters.T2);
	push_back(rgbyte, (USHORT) customParameters.T3);
	push_back(rgbyte, (USHORT) customParameters.RESET);

	return new JpegMarkerSegment(JPEG_LSE, std::move(rgbyte));
}


JpegMarkerSegment* JpegMarkerSegment::CreateColorTransformMarker(int i)
{
	std::vector<BYTE> rgbyteXform;

	rgbyteXform.push_back('m');
	rgbyteXform.push_back('r');
	rgbyteXform.push_back('f');
	rgbyteXform.push_back('x');
	rgbyteXform.push_back((BYTE) i);

	return new JpegMarkerSegment(JPEG_APP8, std::move(rgbyteXform));
}


JpegMarkerSegment* JpegMarkerSegment::CreateStartOfScanMarker(const JlsParameters* pparams, LONG icomponent)
{
	BYTE itable = 0;

	std::vector<BYTE> rgbyte;

	if (icomponent < 0)
	{
		rgbyte.push_back((BYTE) pparams->components);
		for (LONG i = 0; i < pparams->components; ++i)
		{
			rgbyte.push_back(BYTE(i + 1));
			rgbyte.push_back(itable);
		}
	}
	else
	{
		rgbyte.push_back(1);
		rgbyte.push_back((BYTE) icomponent);
		rgbyte.push_back(itable);
	}

	rgbyte.push_back(BYTE(pparams->allowedlossyerror));
	rgbyte.push_back(BYTE(pparams->ilv));
	rgbyte.push_back(0); // transform

	return new JpegMarkerSegment(JPEG_SOS, std::move(rgbyte));
}
