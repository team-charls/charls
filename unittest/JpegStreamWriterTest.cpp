//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use. 
//

#include "stdafx.h"
#include "CppUnitTest.h"

#include "..\src\jpegstreamwriter.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CharLSUnitTest
{
	TEST_CLASS(JpegStreamWriterTest)
	{
	public:
		TEST_METHOD(LengthWillbeZeroAfterCreate)
		{
			JpegStreamWriter writer;
			Assert::AreEqual(static_cast<size_t>(0), writer.GetLength());
		}
	};
}