// 
// (C) Jan de Vaan 2007-2010, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 

#ifndef TEST_UTIL
#define TEST_UTIL

#include "../src/util.h"
#include <vector>
#include <exception>

typedef const char* SZC;

void FixEndian(std::vector<BYTE>* rgbyte, bool littleEndianData);
bool ReadFile(SZC strName, std::vector<BYTE>* pvec, int ioffs = 0, int bytes = 0);
void TestFile(SZC strName, int ioffs, Size size2, int cbit, int ccomp, bool littleEndianFile = false, int loopCount = 1);
void TestRoundTrip(const char* strName, std::vector<BYTE>& rgbyteRaw, Size size, int cbit, int ccomp, int loopCount = 1);

void WriteFile(SZC strName, std::vector<BYTE>& vec);
void DecompressFile(SZC strNameEncoded, SZC strNameRaw, int ioffs, bool bcheckEncode = true);

class UnitTestException : public std::exception {
public:
    explicit UnitTestException() : exception()
    {
    }
};

class Assert
{
public:
    static void IsTrue(bool condition)
    {
        if (!condition)
            throw UnitTestException();
    }
};

#endif
