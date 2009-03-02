// test.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
#include <windows.h>
#include <iostream>
#include <vector>

#include "../interface.h"
#include "../util.h"
#include "../defaulttraits.h"
#include "../losslesstraits.h"
#pragma warning (disable: 4996)

typedef const char* SZC;




double getTime() 
{ 
	LARGE_INTEGER time;
	::QueryPerformanceCounter(&time);
	LARGE_INTEGER freq;
	::QueryPerformanceFrequency(&freq);

	return double(time.LowPart) * 1000.0/double(freq.LowPart);
}

void ReadFile(SZC strName, std::vector<BYTE>* pvec, int ioffs = 0)
{
	FILE* pfile = fopen(strName, "rb");
	fseek(pfile, 0, SEEK_END);	
	int cbyteFile = ftell(pfile);
	fseek(pfile, ioffs, SEEK_SET);	
	pvec->resize(cbyteFile-ioffs);
	fread(&(*pvec)[0],1, pvec->size(), pfile);
	fclose(pfile);
	
}


void TestRoundTrip(const char* strName, const BYTE* rgbyteRaw, Size size, int cbit, int ccomp)
{	
	std::vector<BYTE> rgbyteCompressed;
	rgbyteCompressed.resize(size.cx *size.cy * 4);
	
	std::vector<BYTE> rgbyteOut;
	rgbyteOut.resize(size.cx * size.cy * ((cbit + 7) / 8) * ccomp);
	
	double dblstart = getTime();
	
	JlsParamaters params = {0};
	params.components = ccomp;
	params.bitspersample = cbit;
	params.height = size.cy;
	params.width = size.cx;
	params.ilv = ccomp == 3 ? ILV_SAMPLE : ILV_NONE;

	int cbyteCompressed;
	JpegLsEncode(&rgbyteCompressed[0], rgbyteCompressed.size(), &cbyteCompressed, rgbyteRaw, rgbyteOut.size(), &params);

	//size_t cbyteCompressed = JpegLsEncode(rgbyteRaw, size, cbit, ccomp, ccomp == 3 ? ILV_SAMPLE : ILV_NONE, &rgbyteCompressed[0], int(rgbyteCompressed.size()), 0);

	//CString strDst = strName;
	//strDst = strDst + ".jls";
	//WriteFile(strDst.GetString(), &rgbyteCompressed[0], cbyteCompressed);

	double dwtimeEncodeComplete = getTime();
	
	JpegLsDecode(&rgbyteOut[0], rgbyteOut.size(), &rgbyteCompressed[0], int(cbyteCompressed));
	
	double dblfactor = 1.0 *  rgbyteOut.size() / cbyteCompressed;
	double dwtimeDecodeComplete = getTime();
	std::cout << "RoundTrip test for: " << strName << "\n\r";
	printf("Encode: %f Decode: %f Ratio: %f \n\r", dwtimeEncodeComplete - dblstart, dwtimeDecodeComplete - dwtimeEncodeComplete, dblfactor);

	BYTE* pbyteOut = &rgbyteOut[0];
	for (UINT i = 0; i < rgbyteOut.size(); ++i)
	{
		if (rgbyteRaw[i] != pbyteOut[i])
		{
			ASSERT(false);
			break;
		}
	}						    

}

void TestCompliance(const BYTE* pbyteCompressed, int cbyteCompressed, const BYTE* rgbyteRaw, int cbyteRaw)
{	
	JlsParamaters params = {0};
	JpegLsReadHeader(pbyteCompressed, cbyteCompressed, &params);

	std::vector<BYTE> rgbyteCompressed;
	rgbyteCompressed.resize(params.height *params.width* 4);
	
	std::vector<BYTE> rgbyteOut;
	rgbyteOut.resize(params.height *params.width * ((params.bitspersample + 7) / 8) * params.components);
	
	JpegLsDecode(&rgbyteOut[0], rgbyteOut.size(), pbyteCompressed, cbyteCompressed);

	if (params.allowedlossyerror == 0)
	{
		BYTE* pbyteOut = &rgbyteOut[0];
		for (UINT i = 0; i < cbyteRaw; ++i)
		{
			if (rgbyteRaw[i] != pbyteOut[i])
			{
				ASSERT(false);
				break;
			}
		}						    
	}

	int cbyteCompressedActual = 0;

	JLS_ERROR error = JpegLsVerifyEncode(&rgbyteRaw[0], cbyteRaw, pbyteCompressed, cbyteCompressed);
	ASSERT(error == OK);
//	JpegLsEncode(&rgbyteOut[0], rgbyteOut.size(), &cbyteCompressedActual, &rgbyteRaw[0], cbyteRaw, &params);

	//BYTE* pbyteOut = &rgbyteOut[0];
	//for (UINT i = 0; i < cbyteCompressed; ++i)
	//{
	//	if (pbyteCompressed[i] != pbyteOut[i])
	//	{
	//		ASSERT(false);
	//		break;
	//	}
	//}						    
}


void TestFile(SZC strName, int ioffs, Size size2, int cbit, int ccomp)
{
	std::vector<BYTE> rgbyteNoise;
	
	ReadFile(strName, &rgbyteNoise, ioffs);

	TestRoundTrip(strName, &rgbyteNoise[0], size2, cbit, ccomp);

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

//	TestFile("..\\test\\mars\\phoenixmars.ppm", 40, Size(5300,4300),  8, 3);
	TestFile("..\\test\\mr2_unc", 1728, size1024,  16, 1);
	TestFile("..\\test\\0015.raw", 0, size1024,  8, 1);
	TestFile("..\\test\\lena8b.raw", 0, size512,  8, 1);

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

	TestRoundTrip("noise", &rgbyteNoise[0], size2, 7, 1);
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


void SwapBytes(std::vector<BYTE>* rgbyte)
{
	 for (UINT i = 0; i < rgbyte->size(); i += 2)
	 {
		 std::swap((*rgbyte)[i], (*rgbyte)[i + 1]);
	 }
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



	const BYTE rgbyte[] = { 0,   0,  90,  74, 
							68,  50,  43, 205, 
							64, 145, 145, 145, 
							100, 145, 145, 145};
	const BYTE rgbyteComp[] =   {	0xFF, 0xD8, 0xFF, 0xF7, 0x00, 0x0B, 0x08, 0x00, 0x04, 0x00, 0x04, 0x01, 0x01, 0x11, 0x00, 0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 
							0xC0, 0x00, 0x00, 0x6C, 0x80, 0x20, 0x8E,
							0x01, 0xC0, 0x00, 0x00, 0x57, 0x40, 0x00, 0x00, 0x6E, 0xE6, 0x00, 0x00, 0x01, 0xBC, 0x18, 0x00,
							0x00, 0x05, 0xD8, 0x00, 0x00, 0x91, 0x60, 0xFF, 0xD9};



void TestAnnexH3()
{
	Size size = Size(4,4);	
	std::vector<BYTE> vecRaw(16);
	memcpy(&vecRaw[0], rgbyte, 16);
//	TestJls(vecRaw, size, 8, 1, ILV_NONE, rgbyteComp, sizeof(rgbyteComp), false);
}

void TestConformanceLosslessMode()
{
//	DecompressFile("..\\test\\mars\\phoenixmars.jls", "..\\test\\mars\\phoenixmars.ppm",40);

	// Test 1
	DecompressFile("..\\test\\conformance\\t8c0e0.jls", "..\\test\\conformance\\test8.ppm",15);

	// Test 2


	// Test 3
	DecompressFile("..\\test\\conformance\\t8c2e0.jls", "..\\test\\conformance\\test8.ppm", 15);

	// Test 4
	DecompressFile("..\\test\\conformance\\t8c0e3.jls", "..\\test\\conformance\\test8.ppm",15);

	// Test 5

	// Test 6
	DecompressFile("..\\test\\conformance\\t8c2e3.jls", "..\\test\\conformance\\test8.ppm",15);


	// Test 7
	// Test 8

	// Test 9
//	DecompressFile("..\\test\\conformance\\t8nde0.jls", "..\\test\\conformance\\test8bs2.pgm",15);	

	// Test 10
//	DecompressFile("..\\test\\conformance\\t8nde3.jls", "..\\test\\conformance\\test8bs2.pgm",15);	

	// Test 11
	DecompressFile("..\\test\\conformance\\t16e0.jls", "..\\test\\conformance\\test16.pgm",16);
	
	// Test 12
	DecompressFile("..\\test\\conformance\\t16e3.jls", "..\\test\\conformance\\test16.pgm",16);

	

	// additional, Lena compressed with other codec (UBC?), vfy with CharLS
	DecompressFile("..\\test\\lena8b.jls", "..\\test\\lena8b.raw",0);



}




int _tmain(int argc, _TCHAR* argv[])
{

	TestConformanceLosslessMode();
	TestTraits16bit();		
	TestTraits8bit();		
	TestPerformance();
	TestNoiseImage();
	TestAnnexH3();
	char c;
	std::cin >> c;
	return 0;
}

