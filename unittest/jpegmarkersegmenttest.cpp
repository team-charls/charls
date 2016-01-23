//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use. 
//

#include "stdafx.h"
#include "CppUnitTest.h"

#include "..\src\jpegmarkersegment.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CharLSUnitTest
{
	TEST_CLASS(JpegMarkerSegmentTest)
	{
	public:
		TEST_METHOD(CreateStartOfFrameMarker)
		{
			// TODO
			//static JpegMarkerSegment* CreateStartOfFrameMarker(Size size, LONG bitsPerSample, LONG ccomp);
		}

		TEST_METHOD(CreateJpegFileInterchangeFormatMarker)
		{
			// TODO
			//static JpegMarkerSegment* CreateJpegFileInterchangeFormatMarker(const JfifParameters& jfif);
		}

		TEST_METHOD(CreateJpegLSExtendedParametersMarker)
		{
			// TODO
			//static JpegMarkerSegment* CreateJpegLSExtendedParametersMarker(const JlsCustomParameters& pcustom);
		}

		TEST_METHOD(CreateColorTransformMarker)
		{
			// TODO
			//static JpegMarkerSegment* CreateColorTransformMarker(int i);
		}

		TEST_METHOD(CreateStartOfScanMarker)
		{
			// TODO
			//static JpegMarkerSegment* CreateStartOfScanMarker(const JlsParameters* pparams, LONG icomponent);
		}
	};
}