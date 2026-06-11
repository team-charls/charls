// SPDX-FileCopyrightText: © 2010 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "benchmark.hpp"
#include "compare.hpp"
#include "decode.hpp"
#include "encode.hpp"

MSVC_WARNING_SUPPRESS(4866) // Vcpkg fails to add argparse as external include directory.
#include <argparse/argparse.hpp>
MSVC_WARNING_UNSUPPRESS()

#include <filesystem>
#include <iostream>
#include <string>

using argparse::ArgumentParser;
using std::cout;
using std::string;
using namespace charls::cli;
namespace fs = std::filesystem;

namespace {

const char* const input_argument{"input"};
const char* const output_argument{"output"};
const char* const source1_argument{"source1"};
const char* const source2_argument{"source2"};
const char* const loop_count_argument{"--loop-count"};
const char* const interleave_mode_argument{"--interleave-mode"};
const char* const near_lossless_argument{"--near-lossless"};
const char* const color_transform_argument{"--color-transform"};

[[nodiscard]]
uint32_t get_loop_count(const ArgumentParser& command)
{
    static constexpr uint32_t default_loop_count{10};
    const auto loop_count{command.present<uint32_t>(loop_count_argument)};
    return loop_count.has_value() && *loop_count != 0 ? *loop_count : default_loop_count;
}

[[nodiscard]]
int32_t get_interleave_mode_argument(const ArgumentParser& command)
{
    const auto interleave_mode{command.present<int32_t>(interleave_mode_argument)};
    return interleave_mode.value_or(default_interleave_mode);
}

[[nodiscard]]
int32_t get_near_lossless_argument(const ArgumentParser& command)
{
    static constexpr int32_t default_near_lossless{};
    const auto near{command.present<int32_t>(near_lossless_argument)};
    return near.value_or(default_near_lossless);
}

[[nodiscard]]
int32_t get_color_transform_argument(const ArgumentParser& command)
{
    static constexpr int32_t default_color_transform{};
    const auto color_transform{command.present<int32_t>(color_transform_argument)};
    return color_transform.value_or(default_color_transform);
}

} // namespace


int main(const int argc, const char* const argv[]) // NOLINT(bugprone-exception-escape)
{
    ArgumentParser program("charls-cli");
    program.add_description("CharLS command line interface");

    ArgumentParser encode_command("encode");
    encode_command.add_description("Encode a binary Netpbm file to a JPEG-LS file");
    encode_command.add_argument(input_argument).help("The binary Netpbm file to encode to JPEG-LS (required)");
    encode_command.add_argument(output_argument)
        .nargs(0, 1)
        .help("The output JPEG-LS file path. If not specified, the output file is created "
              "with the same name as the input file and a .jls extension");
    encode_command.add_argument(interleave_mode_argument)
        .scan<'i', int32_t>()
        .help("Interleave mode parameter (optional: default = 0 for 1 component, 1 for multiple components)");
    encode_command.add_argument(near_lossless_argument).scan<'i', int32_t>().help("NEAR parameter (optional: default = 0)");
    encode_command.add_argument(color_transform_argument)
        .scan<'i', int32_t>()
        .help("Color transformation parameter (optional: default = 0)");
    program.add_subparser(encode_command);

    ArgumentParser decode_command("decode");
    decode_command.add_description("Decode a JPEG-LS file to a binary Netpbm file");
    decode_command.add_argument(input_argument).help("The JPEG-LS file to decode to a binary Netpbm file (required)");
    decode_command.add_argument(output_argument)
        .nargs(0, 1)
        .help("The output Netpbm file path. If not specified, the output filename is based on the input filename");
    program.add_subparser(decode_command);

    ArgumentParser compare_command("compare");
    compare_command.add_description("Compare 2 Netpbm files");
    compare_command.add_argument(source1_argument).help("File source 1 (required)");
    compare_command.add_argument(source2_argument).help("File source 2 (required)");
    program.add_subparser(compare_command);

    ArgumentParser benchmark_encode_command("benchmark-encode");
    benchmark_encode_command.add_description("Benchmark encoding a JPEG-LS image");
    benchmark_encode_command.add_argument(input_argument).help("The binary Netpbm file to encode (required)");
    benchmark_encode_command.add_argument(loop_count_argument)
        .scan<'u', uint32_t>()
        .help("Loop count (optional: default = 10)");
    program.add_subparser(benchmark_encode_command);

    ArgumentParser benchmark_decode_command("benchmark-decode");
    benchmark_decode_command.add_description("Benchmark decoding a JPEG-LS image");
    benchmark_decode_command.add_argument(input_argument).help("The JPEG-LS file to decode (required)");
    benchmark_decode_command.add_argument(loop_count_argument)
        .scan<'u', uint32_t>()
        .help("Loop count (optional: default = 10)");
    program.add_subparser(benchmark_decode_command);

    try
    {
        program.parse_args(argc, argv);

        if (program.is_subcommand_used(encode_command))
        {
            const auto input_filename{encode_command.get<string>(input_argument)};
            auto output_filename{encode_command.present<string>(output_argument)};
            if (!output_filename.has_value())
            {
                output_filename = fs::path(input_filename).replace_extension(".jls").string();
            }

            encode_netpbm(input_filename, *output_filename, get_interleave_mode_argument(encode_command),
                          get_near_lossless_argument(encode_command), get_color_transform_argument(encode_command));
        }
        else if (program.is_subcommand_used(decode_command))
        {
            const auto input_filename{decode_command.get<string>(input_argument)};
            auto output_filename{decode_command.present<string>(output_argument)};
            if (!output_filename.has_value())
            {
                output_filename = fs::path(input_filename).replace_extension(".pnm").string();
            }

            decode_to_pnm(input_filename, *output_filename);
        }
        else if (program.is_subcommand_used(compare_command))
        {
            const bool equal{compare_netpbm(compare_command.get<string>(source1_argument),
                                            compare_command.get<string>(source2_argument))};
            if (!equal)
                return EXIT_FAILURE;
        }
        else if (program.is_subcommand_used(benchmark_encode_command))
        {
            benchmark_encode(benchmark_encode_command.get<string>(input_argument), get_loop_count(benchmark_encode_command));
        }
        else if (program.is_subcommand_used(benchmark_decode_command))
        {
            benchmark_decode(benchmark_decode_command.get<string>(input_argument), get_loop_count(benchmark_decode_command));
        }
        else
        {
            cout << program;
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }
    catch (const std::runtime_error& error)
    {
        cout << "command failed: " << error.what() << "\n";
        return EXIT_FAILURE;
    }
}
