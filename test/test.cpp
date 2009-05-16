// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 

#include "stdafx.h"
#include <iostream>
#include <vector>


#include "../interface.h"
#include "../util.h"
#include "../defaulttraits.h"
#include "../losslesstraits.h"
#include "../colortransform.h"

#include "time.h"

typedef const char* SZC;

namespace // local helpers
{


bool ReadFile(SZC strName, std::vector<BYTE>* pvec, int ioffs = 0, int bytes = 0)
{
	FILE* pfile = fopen(strName, "rb");
	if( !pfile ) 
    {
		fprintf( stderr, "Could not open %s\n", strName );
		return false;
    }
	
	fseek(pfile, 0, SEEK_END);	
	int cbyteFile = ftell(pfile);
	if (ioffs < 0)
	{
		ASSERT(bytes != 0);
		ioffs = cbyteFile - bytes;
	}
	if (bytes == 0)
	{
		bytes = cbyteFile  -ioffs;
	}

	fseek(pfile, ioffs, SEEK_SET);	
	pvec->resize(bytes);
	fread(&(*pvec)[0],1, pvec->size(), pfile);
	fclose(pfile);
	return true;
}


void WriteFile(SZC strName, std::vector<BYTE>& vec)
{
	FILE* pfile = fopen(strName, "wb");
	if( !pfile ) 
    {
		fprintf( stderr, "Could not open %s\n", strName );
		return;
    }
	
	fwrite(&vec[0],1, vec.size(), pfile);

	fclose(pfile);
	
}




void Triplet2Planar(std::vector<BYTE>& rgbyte, Size size)
{
	std::vector<BYTE> rgbytePlanar(rgbyte.size());
	
	int cbytePlane = size.cx * size.cy;
	for (int ipixel = 0; ipixel < cbytePlane; ipixel++)
	{
		rgbytePlanar[ipixel]				= rgbyte[ipixel * 3 + 0];
		rgbytePlanar[ipixel + 1*cbytePlane]	= rgbyte[ipixel * 3 + 1];
		rgbytePlanar[ipixel + 2*cbytePlane] = rgbyte[ipixel * 3 + 2];
	}
	std::swap(rgbyte, rgbytePlanar);
}

void Triplet2Line(std::vector<BYTE>& rgbyte, Size size)
{
	std::vector<BYTE> rgbyteInterleaved(rgbyte.size());
	
	int cbyteLine = size.cx;

	for (int line = 0; line < size.cy; ++line)
	{
		const BYTE* pbyteLineIn = &rgbyte[line * size.cx * 3];
		BYTE* pbyteLineOut = &rgbyteInterleaved[line * size.cx * 3];

		for (int ipixel = 0; ipixel < cbyteLine; ipixel++)
		{
			pbyteLineOut[ipixel]				= pbyteLineIn[ipixel * 3 + 0];
			pbyteLineOut[ipixel + 1*cbyteLine]	= pbyteLineIn[ipixel * 3 + 1];
			pbyteLineOut[ipixel + 2*cbyteLine]  = pbyteLineIn[ipixel * 3 + 2];
		}
	}
	std::swap(rgbyte, rgbyteInterleaved);
}


void SwapBytes(std::vector<BYTE>* rgbyte)
{ 
	 for (size_t i = 0; i < rgbyte->size()-1; i += 2)
	 {
		 std::swap((*rgbyte)[i], (*rgbyte)[i + 1]);
	 }
}

}

void TestRoundTrip(const char* strName, std::vector<BYTE>& rgbyteRaw, Size size, int cbit, int ccomp)
{	
	std::vector<BYTE> rgbyteCompressed;
	rgbyteCompressed.resize(size.cx *size.cy * ccomp * cbit / 4);
	
	std::vector<BYTE> rgbyteOut;
	rgbyteOut.resize(size.cx * size.cy * ((cbit + 7) / 8) * ccomp);
	
	double dblstart = getTime();
	
	JlsParamaters params = {0};
	params.components = ccomp;
	params.bitspersample = cbit;
	params.height = size.cy;
	params.width = size.cx;

	if (ccomp == 3)
	{
		params.ilv = ILV_LINE;
		params.colorTransform = 0;
	}

	size_t cbyteCompressed;
	JpegLsEncode(&rgbyteCompressed[0], rgbyteCompressed.size(), &cbyteCompressed, &rgbyteRaw[0], rgbyteOut.size(), &params);

	double dwtimeEncodeComplete = getTime();
	
	JpegLsDecode(&rgbyteOut[0], rgbyteOut.size(), &rgbyteCompressed[0], int(cbyteCompressed));
	
	double dblfactor = 1.0 *  rgbyteOut.size() / cbyteCompressed;
	double bitspersample = cbyteCompressed  * 8  * 1.0 /  (ccomp *size.cy * size.cx);
	double dwtimeDecodeComplete = getTime();
	std::cout << "RoundTrip test for: " << strName << "\n\r";

// disabled for Linux, causes valgrind errors
#if defined(WIN32)
 
	printf("Encode: %f Decode: %f Ratio: %f Bps: %f \n\r", dwtimeEncodeComplete - dblstart, dwtimeDecodeComplete - dwtimeEncodeComplete, dblfactor, bitspersample);
	BYTE* pbyteOut = &rgbyteOut[0];
	for (size_t i = 0; i < rgbyteOut.size(); ++i)
	{
		if (rgbyteRaw[i] != pbyteOut[i])
		{
			ASSERT(false);
			break;
		}
	}	

#endif 
					    
}

void TestCompliance(const BYTE* pbyteCompressed, int cbyteCompressed, const BYTE* rgbyteRaw, int cbyteRaw)
{	
	JlsParamaters params = {0};
	JpegLsReadHeader(pbyteCompressed, cbyteCompressed, &params);

	std::vector<BYTE> rgbyteCompressed;
	rgbyteCompressed.resize(params.height *params.width* 4);
	
	std::vector<BYTE> rgbyteOut;
	rgbyteOut.resize(params.height *params.width * ((params.bitspersample + 7) / 8) * params.components);
	
	JLS_ERROR result = JpegLsDecode(&rgbyteOut[0], rgbyteOut.size(), pbyteCompressed, cbyteCompressed);
	ASSERT(result == OK);

	if (params.allowedlossyerror == 0)
	{
		BYTE* pbyteOut = &rgbyteOut[0];
		for (int i = 0; i < cbyteRaw; ++i)
		{
			if (rgbyteRaw[i] != pbyteOut[i])
			{
				ASSERT(false);
				break;
			}
		}						    
	}

//	int cbyteCompressedActual = 0;

//	JLS_ERROR error = JpegLsVerifyEncode(&rgbyteRaw[0], cbyteRaw, pbyteCompressed, cbyteCompressed);
//	ASSERT(error == OK);
}




void TestFile(SZC strName, int ioffs, Size size2, int cbit, int ccomp, bool swap = false)
{
	int cbyte = size2.cx * size2.cy * ccomp * ((cbit + 7)/8);

	std::vector<BYTE> rgbyteUncompressed;
	
	if (!ReadFile(strName, &rgbyteUncompressed, ioffs, cbyte))
		return;

	if (swap)
	{
		SwapBytes(&rgbyteUncompressed);		
	}

	TestRoundTrip(strName, rgbyteUncompressed, size2, cbit, ccomp);

};



void TestFile16BitAs12(SZC strName, int ioffs, Size size2, int ccomp)
{
	std::vector<BYTE> rgbyteUncompressed;
	
	ReadFile(strName, &rgbyteUncompressed, ioffs);

	USHORT* pushort = (USHORT*)&rgbyteUncompressed[0];

	for (int i = 0; i < rgbyteUncompressed.size()/2; ++i)
	{
		pushort[i] = pushort[i] >> 4;
	}

	TestRoundTrip(strName, rgbyteUncompressed, size2, 12, ccomp);

};





void TestTraits16bit()
{
	DefaultTraitsT<USHORT,USHORT> traits1 = DefaultTraitsT<USHORT,USHORT>(4095,0);
	LosslessTraitsT<USHORT,12> traits2 = LosslessTraitsT<USHORT,12>();

	ASSERT(traits1.LIMIT == traits2.LIMIT);
	ASSERT(traits1.MAXVAL == traits2.MAXVAL);
	ASSERT(traits1.RESET == traits2.RESET);
	ASSERT(traits1.bpp == traits2.bpp);
	ASSERT(traits1.qbpp == traits2.qbpp);

	for (int i = -4096; i < 4096; ++i)
	{
		ASSERT(traits1.ModRange(i) == traits2.ModRange(i));
		ASSERT(traits1.ComputeErrVal(i) == traits2.ComputeErrVal(i));
	}

	for (int i = -8095; i < 8095; ++i)
	{
		ASSERT(traits1.CorrectPrediction(i) == traits2.CorrectPrediction(i));
		ASSERT(traits1.IsNear(i,2) == traits2.IsNear(i,2));	
	}	
}

void TestTraits8bit()
{
	DefaultTraitsT<BYTE,BYTE> traits1 = DefaultTraitsT<BYTE,BYTE>(255,0);
	LosslessTraitsT<BYTE,8> traits2 = LosslessTraitsT<BYTE,8>();

	ASSERT(traits1.LIMIT == traits2.LIMIT);
	ASSERT(traits1.MAXVAL == traits2.MAXVAL);
	ASSERT(traits1.RESET == traits2.RESET);
	ASSERT(traits1.bpp == traits2.bpp);
	ASSERT(traits1.qbpp == traits2.qbpp);	

	for (int i = -255; i < 255; ++i)
	{
		ASSERT(traits1.ModRange(i) == traits2.ModRange(i));
		ASSERT(traits1.ComputeErrVal(i) == traits2.ComputeErrVal(i));
	}

	for (int i = -255; i < 512; ++i)
	{
		ASSERT(traits1.CorrectPrediction(i) == traits2.CorrectPrediction(i));
		ASSERT(traits1.IsNear(i,2) == traits2.IsNear(i,2));	
	}
}




void TestPerformance()
{
	Size size1024 = Size(1024, 1024);
	Size size512 = Size(512, 512);
		
	// 16 bit mono
	TestFile("test/MR2_UNC", 1728, size1024,  16, 1);
	
	// 8 bit mono
	TestFile("test/0015.raw", 0, size1024,  8, 1);
	TestFile("test/lena8b.raw", 0, size512,  8, 1);

	// 8 bit color
	TestFile("test/desktop.ppm", 40, Size(1280,1024),  8, 3);

	// 12 bit RGB
    TestFile("test/SIEMENS-MR-RGB-16Bits.dcm", -1, Size(192,256),  12, 3);
    TestFile16BitAs12("test/DSC_5455.raw", 142949, Size(300,200), 3);

	// 16 bit RGB
    TestFile("test/DSC_5455.raw", 142949, Size(300,200),  16, 3);
}


void TestLargeImagePerformance()
{

    //TestFile("test/rgb8bit/artificial.ppm", 17, Size(3072,2048),  8, 3);
	//TestFile("test/rgb8bit/bridge.ppm", 17, Size(2749,4049),  8, 3);
//	TestFile("test/rgb8bit/big_building.ppm", 17, Size(7216,5412),  8, 3);
//	TestFile("test/rgb16bit/bridge.ppm", 19, Size(2749,4049),  16, 3, true);
}

void TestNoiseImage()
{
	srand(21344); 
	Size size2 = Size(1024, 1024);
	std::vector<BYTE> rgbyteNoise;
	rgbyteNoise.resize(	size2.cx * size2.cy);

	for (int iline = 0; iline<size2.cy; ++iline)
	{
		for (int icol= 0; icol<size2.cx; ++icol)
		{
			BYTE val = BYTE(rand());
			rgbyteNoise[iline*size2.cx + icol] = BYTE(val & 0x7F);// < iline ? val : 0;
		}	
	}

	TestRoundTrip("noise", rgbyteNoise, size2, 7, 1);
}


void DecompressFile(SZC strNameEncoded, SZC strNameRaw, int ioffs)
{
	std::cout << "Conformance test:" << strNameEncoded << "\n\r";
	std::vector<BYTE> rgbyteFile;
	ReadFile(strNameEncoded, &rgbyteFile);

	JlsParamaters metadata;
	if (JpegLsReadHeader(&rgbyteFile[0], rgbyteFile.size(), &metadata) != OK)
	{
		ASSERT(false);
		return;
	}

 	std::vector<BYTE> rgbyteRaw;
	ReadFile(strNameRaw, &rgbyteRaw, ioffs);

	if (metadata.bitspersample > 8)
	{
		SwapBytes(&rgbyteRaw);		
	}

	Size size = Size(metadata.width, metadata.height);

	if (metadata.ilv == ILV_NONE && metadata.components == 3)
	{
		Triplet2Planar(rgbyteRaw, Size(metadata.width, metadata.height));
	}

	TestCompliance(&rgbyteFile[0], rgbyteFile.size(), &rgbyteRaw[0], rgbyteRaw.size());
}


BYTE palettisedDataH10[] = {
  0xFF, 0xD8, //Start of image (SOI) marker 
  0xFF, 0xF7, //Start of JPEG-LS frame (SOF 55) marker – marker segment follows 
  0x00, 0x0B, //Length of marker segment = 11 bytes including the length field 
  0x02, //P = Precision = 2 bits per sample 
  0x00, 0x04, //Y = Number of lines = 4 
  0x00, 0x03, //X = Number of columns = 3 
  0x01, //Nf = Number of components in the frame = 1 
  0x01, //C1  = Component ID = 1 (first and only component) 
  0x11, //Sub-sampling: H1 = 1, V1 = 1 
  0x00, //Tq1 = 0 (this field is always 0) 
 
  0xFF, 0xF8, //LSE – JPEG-LS preset parameters marker 
  0x00, 0x11, //Length of marker segment = 17 bytes including the length field 
  0x02, //ID = 2, mapping table  
  0x05, //TID = 5 Table identifier (arbitrary) 
  0x03, //Wt = 3 Width of table entry 
  0xFF, 0xFF, 0xFF, //Entry for index 0 
  0xFF, 0x00, 0x00, //Entry for index 1 
  0x00, 0xFF, 0x00, //Entry for index 2 
  0x00, 0x00, 0xFF, //Entry for index 3 
 
  0xFF, 0xDA, //Start of scan (SOS) marker 
  0x00, 0x08, //Length of marker segment = 8 bytes including the length field 
  0x01, //Ns = Number of components for this scan = 1 
  0x01, //C1 = Component ID = 1  
  0x05, //Tm 1  = Mapping table identifier = 5 
  0x00, //NEAR = 0 (near-lossless max error) 
  0x00, //ILV = 0 (interleave mode = non-interleaved) 
  0x00, //Al = 0, Ah = 0 (no point transform) 
  0xDB, 0x95, 0xF0, //3 bytes of compressed image data 
  0xFF, 0xD9 //End of image (EOI) marker 
};









   const BYTE rgbyte[] = { 0,   0,  90,  74, 
							68,  50,  43, 205, 
							64, 145, 145, 145, 
							100, 145, 145, 145};
	const BYTE rgbyteComp[] =   {	0xFF, 0xD8, 0xFF, 0xF7, 0x00, 0x0B, 0x08, 0x00, 0x04, 0x00, 0x04, 0x01, 0x01, 0x11, 0x00, 0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 
							0xC0, 0x00, 0x00, 0x6C, 0x80, 0x20, 0x8E,
							0x01, 0xC0, 0x00, 0x00, 0x57, 0x40, 0x00, 0x00, 0x6E, 0xE6, 0x00, 0x00, 0x01, 0xBC, 0x18, 0x00,
							0x00, 0x05, 0xD8, 0x00, 0x00, 0x91, 0x60, 0xFF, 0xD9};



void TestSampleAnnexH3()
{
	Size size = Size(4,4);	
	std::vector<BYTE> vecRaw(16);
	memcpy(&vecRaw[0], rgbyte, 16);
//	TestJls(vecRaw, size, 8, 1, ILV_NONE, rgbyteComp, sizeof(rgbyteComp), false);
}

void TestSmallBuffer()
{
	std::vector<BYTE> rgbyteCompressed;	
	ReadFile("test/lena8b.jls", &rgbyteCompressed, 0);
	
	std::vector<BYTE> rgbyteOut;
	rgbyteOut.resize(512 * 511);	
	JLS_ERROR error = JpegLsDecode(&rgbyteOut[0], rgbyteOut.size(), &rgbyteCompressed[0], int(rgbyteCompressed.size()));
	
	ASSERT(error == UncompressedBufferTooSmall);	
}


void TestDamagedBitStream1()
{
	std::vector<BYTE> rgbyteCompressed;	
	ReadFile("test/incorrect_images/InfiniteLoopFFMPEG_working.jls", &rgbyteCompressed, 0);
	
	std::vector<BYTE> rgbyteOut;
	rgbyteOut.resize(256 * 256 * 2);	
	JLS_ERROR error = JpegLsDecode(&rgbyteOut[0], rgbyteOut.size(), &rgbyteCompressed[0], int(rgbyteCompressed.size()));
	WriteFile("test/incorrect_images/InfinixteLoopFFMPEG.RAW", rgbyteOut);
	//ASSERT(error == InvalidCompressedData);
	
}


void TestDamagedBitStream2()
{
	std::vector<BYTE> rgbyteCompressed;	
	ReadFile("test/lena8b.jls", &rgbyteCompressed, 0);
	
	rgbyteCompressed.resize(900);
	rgbyteCompressed.resize(40000,3);

	std::vector<BYTE> rgbyteOut;
	rgbyteOut.resize(512 * 512);	
	JLS_ERROR error = JpegLsDecode(&rgbyteOut[0], rgbyteOut.size(), &rgbyteCompressed[0], int(rgbyteCompressed.size()));
	ASSERT(error == InvalidCompressedData);
	
}



void TestConformance()
{	
	DecompressFile("test/jlsimage/banny_normal.jls", "test/jlsimage/banny.ppm",38);
	DecompressFile("test/jlsimage/banny_Hp1.jls", "test/jlsimage/banny.ppm",38);
	DecompressFile("test/jlsimage/banny_Hp2.jls", "test/jlsimage/banny.ppm",38);
	DecompressFile("test/jlsimage/banny_Hp3.jls", "test/jlsimage/banny.ppm",38);
	// Test 1
	DecompressFile("test/conformance/T8C0E0.JLS", "test/conformance/TEST8.PPM",15);

	// Test 2
	DecompressFile("test/conformance/T8C1E0.JLS", "test/conformance/TEST8.PPM",15);

	// Test 3
	DecompressFile("test/conformance/T8C2E0.JLS", "test/conformance/TEST8.PPM", 15);

	
	// Test 4
	DecompressFile("test/conformance/T8C0E3.JLS", "test/conformance/TEST8.PPM",15);

	// Test 5
	DecompressFile("test/conformance/T8C1E3.JLS", "test/conformance/TEST8.PPM",15);

	// Test 6
	DecompressFile("test/conformance/T8C2E3.JLS", "test/conformance/TEST8.PPM",15);


	// Test 7
	// Test 8

	// Test 9
	DecompressFile("test/conformance/T8NDE0.JLS", "test/conformance/TEST8BS2.PGM",15);	

	// Test 10
	DecompressFile("test/conformance/T8NDE3.JLS", "test/conformance/TEST8BS2.PGM",15);	
	
	// Test 11
	DecompressFile("test/conformance/T16E0.JLS", "test/conformance/TEST16.PGM",16);
	
	// Test 12
	DecompressFile("test/conformance/T16E3.JLS", "test/conformance/TEST16.PGM",16);

	

	// additional, Lena compressed with other codec (UBC?), vfy with CharLS
	DecompressFile("test/lena8b.jls", "test/lena8b.raw",0);
}




void unittest()
{
	
	printf("Test Damaged bitstream\r\n");
	TestDamagedBitStream1();
	TestDamagedBitStream2();
	
	printf("Test Annex H3\r\n");
	TestSampleAnnexH3();

	printf("Test Traits\r\n");
	TestTraits16bit();		
	TestTraits8bit();		

	printf("Test Conformance\r\n");
	TestConformance();
	
	printf("Test Perf\r\n");
	TestPerformance();
	TestLargeImagePerformance();

	printf("Test Small buffer\r\n");
	TestSmallBuffer();

	TestNoiseImage();
}

int main(int argc, char* argv[])
{
	if (argc >= 2)
	{
		std::string str = argv[1];
		if (str.compare("-unittest") == 0)
		{
			unittest();
			char c;
			std::cin >> c;
			return 0;
		}
	}
}
