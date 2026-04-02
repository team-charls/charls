// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "support/portable_arbitrary_map.hpp"
#include "performance.hpp"
#include "util.hpp"

MSVC_WARNING_SUPPRESS(4866) // Vcpkg fails to add argparse as external include directory.
#include <argparse/argparse.hpp>
MSVC_WARNING_UNSUPPRESS()

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

using std::byte;
using std::cout;
using std::getline;
using std::ifstream;
using std::ios;
using std::istream;
using std::iter_swap;
using std::ofstream;
using std::ostream;
using std::runtime_error;
using std::streamoff;
using std::string;
using std::stringstream;
using std::vector;
using namespace charls;
namespace fs = std::filesystem;

namespace {

constexpr ios::openmode mode_input{ios::in | ios::binary};

ifstream open_input_stream(const char* filename)
try
{
    ifstream stream;
    stream.exceptions(ios::eofbit | ios::failbit | ios::badbit);
    stream.open(filename, mode_input);

    return stream;
}
catch (const std::ifstream::failure&)
{
    cout << "Failed to open/read file: " << fs::absolute(filename) << "\n";
    throw;
}


constexpr uint32_t log2_floor(const uint32_t n) noexcept
{
    ASSERT(n != 0 && "log2 is not defined for 0");

    uint32_t result = 0;
    uint32_t val = n;
    while (val >>= 1)
        ++result;
    return result;
}


constexpr int result_to_exit_code(const bool result) noexcept
{
    return result ? EXIT_SUCCESS : EXIT_FAILURE;
}


uint32_t max_value_to_bits_per_sample(const uint32_t max_value) noexcept
{
    ASSERT(max_value > 0);
    return log2_floor(max_value) + 1;
}


void read(istream& input, void* buffer, const size_t size)
{
    input.read(static_cast<char*>(buffer), static_cast<std::streamsize>(size));
}


template<typename Container>
void read(istream& input, Container& destination_container)
{
    read(input, destination_container.data(), destination_container.size());
}


size_t get_stream_length(istream& stream, const size_t end_offset = 0)
{
    stream.seekg(0, ios::end);
    const auto length{static_cast<size_t>(stream.tellg())};
    stream.seekg(static_cast<ios::off_type>(end_offset), ios::beg);

    return length;
}


template<typename SizeType>
void convert_planar_to_pixel(const size_t width, const size_t height, const void* source, void* destination) noexcept
{
    const size_t stride_in_pixels{width * 3};
    const auto* plane0{static_cast<const SizeType*>(source)};
    const auto* plane1{static_cast<const SizeType*>(source) + (width * height)};
    const auto* plane2{static_cast<const SizeType*>(source) + (width * height * 2)};

    auto* pixels{static_cast<SizeType*>(destination)};

    for (size_t row{}; row != height; ++row)
    {
        for (size_t column{}, offset = 0; column != width; ++column, offset += 3)
        {
            pixels[offset + 0] = plane0[column];
            pixels[offset + 1] = plane1[column];
            pixels[offset + 2] = plane2[column];
        }

        plane0 += width;
        plane1 += width;
        plane2 += width;
        pixels += stride_in_pixels;
    }
}


bool decode_to_pnm(const char* filename_input, const char* filename_output)
try
{
    ifstream input{open_input_stream(filename_input)};

    const size_t length{get_stream_length(input)};
    vector<byte> encoded_source(length);
    read(input, encoded_source);

    vector<byte> decoded_destination;
    frame_info frame_info;
    interleave_mode interleave_mode;
    std::tie(frame_info, interleave_mode) = jpegls_decoder::decode(encoded_source, decoded_destination);

    if (frame_info.component_count != 1 && frame_info.component_count != 3)
    {
        cout << "Only JPEG-LS images with component count 1 or 3 are supported to decode to pnm\n";
        return false;
    }

    // PPM format only supports by-pixel, convert if needed.
    if (interleave_mode == charls::interleave_mode::none && frame_info.component_count == 3)
    {
        vector<byte> pixels(decoded_destination.size());
        if (frame_info.bits_per_sample > 8)
        {
            convert_planar_to_pixel<uint16_t>(frame_info.width, frame_info.height, decoded_destination.data(),
                                              pixels.data());
        }
        else
        {
            convert_planar_to_pixel<uint8_t>(frame_info.width, frame_info.height, decoded_destination.data(), pixels.data());
        }

        swap(decoded_destination, pixels);
    }

    // PNM format requires most significant byte first (big endian).
    const int max_value{(1 << frame_info.bits_per_sample) - 1};

    if (const int bytes_per_sample{max_value > 255 ? 2 : 1}; bytes_per_sample == 2)
    {
        for (auto i{decoded_destination.begin()}; i != decoded_destination.end(); i += 2)
        {
            iter_swap(i, i + 1);
        }
    }

    const int magic_number{frame_info.component_count == 3 ? 6 : 5};

    ofstream output{open_output_stream(filename_output)};
    output << 'P' << magic_number << "\n" << frame_info.width << ' ' << frame_info.height << "\n" << max_value << "\n";
    write(output, decoded_destination, decoded_destination.size());
    output.close();

    return true;
}
catch (const runtime_error& error)
{
    cout << "Failed to decode " << filename_input << " to " << filename_output << ", reason: " << error.what() << '\n';
    return false;
}


vector<int> read_pnm_header(istream& pnm_file)
{
    vector<int> read_values;

    // All portable anymap format (PNM) start with the character P.
    if (const auto first{static_cast<char>(pnm_file.get())}; first != 'P')
        return read_values;

    while (read_values.size() < 4)
    {
        string bytes;
        getline(pnm_file, bytes);
        stringstream line(bytes);

        while (read_values.size() < 4)
        {
            int value{-1};
            line >> value;
            if (value <= 0)
                break;

            read_values.push_back(value);
        }
    }
    return read_values;
}


// Purpose: this function can encode an image stored in the Portable Anymap Format (PNM)
//          into the JPEG-LS format. The 2 binary formats P5 and P6 are supported:
//          Portable GrayMap: P5 = binary, extension = .pgm, 0-2^16 (gray scale)
//          Portable PixMap: P6 = binary, extension.ppm, range 0-2^16 (RGB)
void encode_pnm(const char* filename_input, const char* filename_output)
{
    ifstream pnm_file(open_input_stream(filename_input));

    const auto read_values{read_pnm_header(pnm_file)};
    if (read_values.size() != 4)
        throw std::runtime_error("Bad PNM header");

    const frame_info frame_info{static_cast<uint32_t>(read_values[1]), static_cast<uint32_t>(read_values[2]),
                                static_cast<int32_t>(max_value_to_bits_per_sample(static_cast<uint32_t>(read_values[3]))),
                                read_values[0] == 6 ? 3 : 1};

    const auto bytes_per_sample{static_cast<int32_t>(::bit_to_byte_count(frame_info.bits_per_sample))};
    vector<uint8_t> input_buffer(static_cast<size_t>(frame_info.width) * frame_info.height * bytes_per_sample *
                                 frame_info.component_count);
    read(pnm_file, input_buffer);
    if (!pnm_file.good())
        throw std::runtime_error("Failed to read from file");

    // PNM format is stored with most significant byte first (big endian).
    if (bytes_per_sample == 2)
    {
        for (auto i{input_buffer.begin()}; i != input_buffer.end(); i += 2)
        {
            iter_swap(i, i + 1);
        }
    }

    jpegls_encoder encoder;
    encoder.frame_info(frame_info)
        .interleave_mode(frame_info.component_count == 3 ? interleave_mode::line : interleave_mode::none);

    vector<uint8_t> destination(encoder.estimated_destination_size());
    encoder.destination(destination);
    const size_t bytes_encoded{encoder.encode(input_buffer)};

    ofstream jls_file_stream(open_output_stream(filename_output));
    write(jls_file_stream, destination, bytes_encoded);
    jls_file_stream.close();
}


void encode_pam(const char* filename_input, const char* filename_output)
{
    const support::portable_arbitrary_map pam_file(filename_input);

    const frame_info frame_info{pam_file.width(), pam_file.height(), pam_file.bits_per_sample(), pam_file.component_count()};

    jpegls_encoder encoder;
    encoder.frame_info(frame_info)
        .interleave_mode(frame_info.component_count > 1 ? interleave_mode::line : interleave_mode::none);

    vector<uint8_t> destination(encoder.estimated_destination_size());
    encoder.destination(destination);
    const size_t bytes_encoded{encoder.encode(pam_file.image_data())};

    ofstream jls_file_stream(open_output_stream(filename_output));
    write(jls_file_stream, destination, bytes_encoded);
    jls_file_stream.close();
}


bool encode_netpbm(const char* filename_input, const char* filename_output)
try
{
    if (std::filesystem::path(filename_input).extension() == ".pam")
    {
        encode_pam(filename_input, filename_output);
    }
    else
    {
        encode_pnm(filename_input, filename_output);
    }

    return true;
}
catch (const runtime_error& error)
{
    cout << "Failed to encode " << filename_input << " to " << filename_output << ", reason: " << error.what() << '\n';
    return false;
}


bool compare_pnm(istream& pnm_file1, istream& pnm_file2)
{
    const auto header1{read_pnm_header(pnm_file1)};
    if (header1.size() != 4)
    {
        cout << "Cannot read header from input file 1\n";
        return false;
    }

    const auto header2{read_pnm_header(pnm_file2)};
    if (header2.size() != 4)
    {
        cout << "Cannot read header from input file 2\n";
        return false;
    }

    if (header1[0] != header2[0])
    {
        cout << "Header type " << header1[0] << " is not equal with type " << header2[0] << "\n";
        return false;
    }

    const auto width{static_cast<size_t>(header1[1])};
    if (width != static_cast<size_t>(header2[1]))
    {
        cout << "Width " << width << " is not equal with width " << header2[1] << "\n";
        return false;
    }

    const auto height{static_cast<size_t>(header1[2])};
    if (height != static_cast<size_t>(header2[2]))
    {
        cout << "Height " << height << " is not equal with height " << header2[2] << "\n";
        return false;
    }

    if (header1[3] != header2[3])
    {
        cout << "max-value " << header1[3] << " is not equal with max-value " << header2[3] << "\n";
        return false;
    }
    const size_t bytes_per_sample{header1[3] > 255 ? 2U : 1U};

    const size_t byte_count{width * height * bytes_per_sample};
    vector<byte> bytes1(byte_count);
    vector<byte> bytes2(byte_count);

    read(pnm_file1, bytes1);
    read(pnm_file2, bytes2);

    for (size_t x{}; x != height; ++x)
    {
        for (size_t y{}; y < width; y += bytes_per_sample)
        {
            if (bytes_per_sample == 1)
            {
                if (bytes1[(x * width) + y] != bytes2[(x * width) + y])
                {
                    cout << "Values of the 2 files are different, height:" << x << ", width:" << y << "\n";
                    return false;
                }
            }
            else
            {
                if (bytes1[(x * width) + y] != bytes2[(x * width) + y] ||
                    bytes1[(x * width) + (y + 1)] != bytes2[(x * width) + (y + 1)])
                {
                    cout << "Values of the 2 files are different, height:" << x << ", width:" << y << "\n";
                    return false;
                }
            }
        }
    }

    cout << "Values of the 2 files are equal\n";
    return true;
}

} // namespace


int main(const int argc, const char* const argv[]) // NOLINT(bugprone-exception-escape)
{
    using argparse::ArgumentParser;

    ArgumentParser program("charls-cli");
    program.add_description("CharLS command line interface");

    ArgumentParser encode_command("encode");
    encode_command.add_description("Encode a binary Netpbm file to a JPEG-LS file");
    encode_command.add_argument("input").help("The binary binary file to encode to JPEG-LS (required)");
    encode_command.add_argument("output").nargs(0, 1).help(
        "The output JPEG-LS file path. If not specified, the output file is created "
        "with the same name as the input file and a .jls extension");
    program.add_subparser(encode_command);

    ArgumentParser decode_command("decode");
    decode_command.add_description("Decode a JPEG-LS file to a binary Netpbm file");
    decode_command.add_argument("input").help(
        "The JPEG-LS file to decode to a binary PGM file (required)");
    decode_command.add_argument("output").nargs(0, 1).help(
        "The output Netpbm file path. If not specified, the output filename is based on the input filename");
    program.add_subparser(decode_command);

    ArgumentParser compare_command("compare");
    compare_command.add_description("Compare 2 Netpbm files");
    compare_command.add_argument("source1").help("File source 1 (required)");
    compare_command.add_argument("source2").help("File source 2 (required)");
    program.add_subparser(compare_command);

    ArgumentParser benchmark_encode_command("benchmark-encode");
    benchmark_encode_command.add_description("Benchmark encoding a JPEG-LS image");
    benchmark_encode_command.add_argument("input").help(
        "The binary Netpbm file to encode (required)");
    benchmark_encode_command.add_argument("loop-count").nargs(0, 1).help("Loop count (optional: default = 10");
    program.add_subparser(benchmark_encode_command);

    ArgumentParser benchmark_decode_command("benchmark-decode");
    benchmark_decode_command.add_description("Benchmark decoding a JPEG-LS image");
    benchmark_decode_command.add_argument("input").help("The JPEG-LS file to decode (required)");
    benchmark_decode_command.add_argument("loop-count").nargs(0, 1).help("Loop count (optional: default = 10");
    program.add_subparser(benchmark_decode_command);

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& error)
    {
        cout << error.what() << "\n";
        return EXIT_FAILURE;
    }

    if (program.is_subcommand_used(encode_command))
    {
        auto input_filename = encode_command.get<std::string>("input");
        auto output_filename = encode_command.present<std::string>("output");
        if (!output_filename.has_value())
        {
            output_filename = fs::path(input_filename).replace_extension(".jls").string();
        }
        return result_to_exit_code(encode_netpbm(input_filename.c_str(), output_filename->c_str()));
    }

    if (program.is_subcommand_used(decode_command))
    {
        auto input_filename = decode_command.get<std::string>("input");
        auto output_filename = decode_command.present<std::string>("output");
        if (!output_filename.has_value())
        {
            output_filename = fs::path(input_filename).replace_extension(".pnm").string();
        }
        return result_to_exit_code(decode_to_pnm(input_filename.c_str(), output_filename->c_str()));
    }

    if (program.is_subcommand_used(compare_command))
    {
        auto source1_filename = compare_command.get<std::string>("source1");
        auto source2_filename = compare_command.get<std::string>("source2");

        ifstream pnm_file1(source1_filename, mode_input);
        ifstream pnm_file2(source2_filename, mode_input);
        return result_to_exit_code(compare_pnm(pnm_file1, pnm_file2));
    }

    if (program.is_subcommand_used(benchmark_encode_command))
    {
        auto input_filename = benchmark_encode_command.get<std::string>("input");
        auto loop_count = benchmark_encode_command.present<int>("loop-count");

        encode_performance_tests(input_filename.c_str(), loop_count.value_or(10));
        return EXIT_SUCCESS;
    }

    if (program.is_subcommand_used(benchmark_decode_command))
    {
        auto input_filename = benchmark_encode_command.get<std::string>("input");
        auto loop_count = benchmark_encode_command.present<int>("loop-count");

        decode_performance_tests(input_filename.c_str(), loop_count.value_or(10));
        return EXIT_SUCCESS;
    }

    cout << program;
    return EXIT_FAILURE;
}
