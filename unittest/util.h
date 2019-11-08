#pragma once

#include "../test/portable_anymap_file.h"

#include <vector>

std::vector<uint8_t> read_file(const char* filename);

void fix_endian(std::vector<uint8_t>& buffer, bool little_endian_data) noexcept;
charls_test::portable_anymap_file read_anymap_reference_file(const char* filename, charls::interleave_mode interleave_mode, const charls::frame_info& frame_info);
std::vector<uint8_t> create_test_spiff_header(uint8_t high_version = 2, uint8_t low_version = 0, bool end_of_directory = true);
std::vector<uint8_t> create_noise_image_16bit(size_t pixel_count, int bit_count, uint32_t seed);
void test_round_trip_legacy(const std::vector<uint8_t>& source, const JlsParameters& params);

namespace CharLSUnitTest {

template<typename Functor>
void assert_expect_exception(charls::jpegls_errc error_value, Functor functor)
{
    try
    {
        functor();
    }
    catch (const charls::jpegls_error& error)
    {
        Microsoft::VisualStudio::CppUnitTestFramework::Assert::IsTrue(error_value == error.code());
        return;
    }
    catch (...)
    {
        throw;
    }

    Assert::Fail();
}

} // namespace CharLSUnitTest
