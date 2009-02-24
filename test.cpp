// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 

#include "stdafx.h"
#include "stdio.h"
#include <vector>
#include <iostream>

#include "header.h"
#include "streams.h"
#include "defaulttraits.h"
#include "losslesstraits.h"
#include <windows.h>


#pragma warning (disable: 4996)


double getTime() 
{ 
	LARGE_INTEGER time;
	::QueryPerformanceCounter(&time);
	LARGE_INTEGER freq;
	::QueryPerformanceFrequency(&freq);

	return double(time.LowPart) * 1000.0/double(freq.LowPart);
}

int ReadJLSHeader(void* pdata, ScanInfo* pmetadata, int cbyte)
{
	JLSInputStream reader((BYTE*)pdata, cbyte);
	reader.ReadHeader();	
	*pmetadata = reader.GetMetadata();
	return reader.GetBytesRead();
}



int Decode(void* pvoid, int cbyte, const BYTE* pbytecompressed, int cbytecompr, const void* pbyteExpected = NULL)
{
	JLSInputStream reader(pbytecompressed, cbytecompr);

	if (pbyteExpected != NULL)
	{
		::memcpy(pvoid, pbyteExpected, cbyte);
		reader.EnableCompare(true);
	}

	if (!reader.Read(pvoid))
		return -1;

	return reader.GetBytesRead();

}


int Encode(const void* pbyte, Size size, int cbit, int ccomp, interleavemode ilv, BYTE* pbytecompressed, int cbytecompr, int nearval, const void* pbyteExpected = NULL)
{
	JLSOutputStream stream;
	stream.Init(size, cbit, ccomp);
	
	if (ilv == ILV_NONE)
	{
		int cbyteComp = size.cx*size.cy*((cbit +7)/8);
		for (int icomp = 0; icomp < ccomp; ++icomp)
		{
			const BYTE* pbyteComp = static_cast<const BYTE*>(pbyte) + icomp*cbyteComp;
			stream.AddScan(pbyteComp, size, cbit, 1, ILV_NONE, nearval);
		}
	}
	else 
	{
		stream.AddScan(pbyte, size, cbit, ccomp, ilv, nearval);
	}

	stream.Write(pbytecompressed, cbytecompr, pbyteExpected);
	
	return stream.GetBytesWritten();	
}




// int double SZC int* void*
typedef const char* SZC;

template <class TYPE>
struct validprintfchar;



void WriteFile(SZC szcName, void* pvoidData, int cbyte)
{
	FILE* pfile = fopen(szcName, "wb");
	fwrite(pvoidData, 1, cbyte, pfile);
	fclose(pfile);	
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

void SwapBytes(std::vector<BYTE>* rgbyte)
{
	 for (UINT i = 0; i < rgbyte->size(); i += 2)
	 {
		 std::swap((*rgbyte)[i], (*rgbyte)[i + 1]);
	 }
}


void Test(const char* strName, const BYTE* rgbyteRaw, Size size, int cbit, int ccomp)
{	
	std::vector<BYTE> pbyteCompressed;
	pbyteCompressed.resize(size.cx *size.cy * 4);
	
	std::vector<BYTE> rgbyteOut;
	rgbyteOut.resize(size.cx * size.cy * ((cbit + 7) / 8) * ccomp);
	
	double dblstart = getTime();
	
	size_t cbyteCompressed = Encode(rgbyteRaw, size, cbit, ccomp, ccomp == 3 ? ILV_SAMPLE : ILV_NONE, &pbyteCompressed[0], int(pbyteCompressed.size()), 0);

	//CString strDst = strName;
	//strDst = strDst + ".jls";
	//WriteFile(strDst.GetString(), &pbyteCompressed[0], cbyteCompressed);

	double dwtimeEncodeComplete = getTime();
	
	double dblfactor = 1.0 *  rgbyteOut.size() / cbyteCompressed;
	printf("Encode: %f Ratio: %f \n\r", dwtimeEncodeComplete - dblstart, dblfactor);
	
	Decode(&rgbyteOut[0], rgbyteOut.size(), &pbyteCompressed[0], int(cbyteCompressed));
	
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



void TestJls(std::vector<BYTE>& rgbyteRaw, Size size, int cbit, int ccomp, interleavemode ilv, const BYTE* rgbyteFile, int cbyteCompressed, bool bNear)
{
	if (cbit > 8 && cbit <= 16)
	{
		SwapBytes(&rgbyteRaw);
	}

	
	std::vector<BYTE> rgbyteTest;
	rgbyteTest.resize(2 *cbyteCompressed );
	
	UINT cbyte = Encode(&rgbyteRaw[0], size, cbit, ccomp, ilv, &rgbyteTest[0], int(rgbyteTest.size()), bNear ? 3 : 0, rgbyteFile);

	WriteFile("test.jls", &rgbyteTest[0], cbyte);

	JLSInputStream reader(rgbyteFile, cbyteCompressed);
	

	std::vector<BYTE> rgbyteUnc;
	rgbyteUnc.resize(size.cx * size.cy * ((cbit + 7) / 8) * ccomp);

	Decode(&rgbyteUnc[0], rgbyteUnc.size(), rgbyteFile, cbyteCompressed, &rgbyteRaw[0]);
	
	WriteFile("test.raw", &rgbyteUnc[0], rgbyteUnc.size());
}


void TestNear(char* strNameEncoded, Size size, int cbit)
{
	std::vector<BYTE> rgbyteFile;
	ReadFile(strNameEncoded, &rgbyteFile);

	std::vector<BYTE> rgbyteUnc;
	rgbyteUnc.resize(size.cx * size.cy * ((cbit + 7) / 8));

	Decode(&rgbyteUnc[0], rgbyteUnc.size(), &rgbyteFile[0], rgbyteFile.size());
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


void DecompressFile(SZC strNameEncoded, SZC strNameRaw, int ioffs)
{
	std::cout << "Conformance test:" << strNameEncoded << "\n\r";
	std::vector<BYTE> rgbyteFile;
	ReadFile(strNameEncoded, &rgbyteFile);

 	std::vector<BYTE> rgbyteRaw;
	ReadFile(strNameRaw, &rgbyteRaw, ioffs);

	ScanInfo metadata;
	int cbyteHeader = ReadJLSHeader(&rgbyteFile[0], &metadata, rgbyteFile.size());
	if (cbyteHeader == 0)
	{
		ASSERT(false);
		return;
	}

	if (metadata.ilv == ILV_NONE && metadata.ccomp == 3)
	{
		Triplet2Planar(rgbyteRaw, metadata.size);
	}

	TestJls(rgbyteRaw,  metadata.size, metadata.cbit, metadata.ccomp, metadata.ilv, &rgbyteFile[0], rgbyteFile.size(), metadata.nnear != 0);
}


void TestFile(SZC strName, int ioffs, Size size2, int cbit, int ccomp)
{
	std::vector<BYTE> rgbyteNoise;
	
	ReadFile(strName, &rgbyteNoise, ioffs);

	Test(strName, &rgbyteNoise[0], size2, cbit, ccomp);

};

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
	TestJls(vecRaw, size, 8, 1, ILV_NONE, rgbyteComp, sizeof(rgbyteComp), false);
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
	DecompressFile("..\\test\\conformance\\t8nde0.jls", "..\\test\\conformance\\test8bs2.pgm",15);	

	// Test 10
	DecompressFile("..\\test\\conformance\\t8nde3.jls", "..\\test\\conformance\\test8bs2.pgm",15);	

	// Test 11
	DecompressFile("..\\test\\conformance\\t16e0.jls", "..\\test\\conformance\\test16.pgm",16);
	
	// Test 12
	DecompressFile("..\\test\\conformance\\t16e3.jls", "..\\test\\conformance\\test16.pgm",16);

	

	// additional, Lena compressed with other codec (UBC?), vfy with CharLS
	DecompressFile("..\\test\\lena8b.jls", "..\\test\\lena8b.raw",0);



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

	Test("noise", &rgbyteNoise[0], size2, 7, 1);
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


extern "C"
{

void __declspec(dllexport) __cdecl Test()
{
	TestTraits16bit();		
	TestTraits8bit();		
	TestConformanceLosslessMode();	
	TestPerformance();
	TestNoiseImage();
	TestAnnexH3();		
	
}



}