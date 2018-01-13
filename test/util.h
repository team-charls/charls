//
// (C) Jan de Vaan 2007-2010, all rights reserved. See the accompanying "License.txt" for licensed use.
//

#ifndef TEST_UTIL
#define TEST_UTIL

#include "../src/charls.h"
#include <vector>
#include <exception>

struct Size
{
    Size(int32_t width, int32_t height) :
        cx(width),
        cy(height)
    {}
    int32_t cx;
    int32_t cy;
};


void FixEndian(std::vector<uint8_t>* rgbyte, bool littleEndianData);
bool ReadFile(const char* filename, std::vector<uint8_t>* pvec, int offset = 0, int bytes = 0);
void TestFile(const char* filename, int ioffs, Size size2, int cbit, int ccomp, bool littleEndianFile = false, int loopCount = 1);
void TestRoundTrip(const char* strName, const std::vector<uint8_t>& rgbyteRaw, Size size, int cbit, int ccomp, int loopCount = 1);
void TestRoundTrip(const char* strName, const std::vector<uint8_t>& rgbyteRaw, JlsParameters& params, int loopCount = 1);
void WriteFile(const char* filename, std::vector<uint8_t>& buffer);
void test_portable_anymap_file(const char* filename, int loopCount = 1);

class UnitTestException : public std::exception {
public:
    explicit UnitTestException() = default;
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
