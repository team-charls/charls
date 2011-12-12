//
// (C) Jan de Vaan 2007-2011, all rights reserved. See the accompanying "License.txt" for licensed use.
//

using System;
using System.IO;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using CharLS;

namespace Convert
{
    internal class Program
    {
        private const int Success = 0;
        private const int Failure = 1;

        private static int Main(string[] args)
        {
            // This sample demonstrates how to convert 8 bit monochrome images and 24 bit color images to a .jls
            // The input path should be a absolute path to a file format .NET can read (.bmp, .png, etc).
            string inputPath;
            if (!TryParseArguments(args, out inputPath))
            {
                Console.WriteLine("Usage: Converter <path to image file>");
                return Failure;
            }

            try
            {
                // Load the image file and get the first frame.
                var decoder = BitmapDecoder.Create(new Uri(inputPath, UriKind.Absolute),
                                                   BitmapCreateOptions.PreservePixelFormat, BitmapCacheOption.Default);

                var frame = decoder.Frames[0];
                int componentCount;
                if (!TryGetComponentCount(frame.Format, out componentCount))
                {
                    Console.WriteLine("Input has format: {0}, which is not supported", frame.Format);
                    return Failure;
                }

                var uncompressedPixels = new byte[frame.PixelWidth * frame.PixelHeight * componentCount];
                frame.CopyPixels(uncompressedPixels, frame.PixelWidth * componentCount, 0);

                // Prepare the 'info' metadata that describes the pixels in the byte buffer.
                var info = new JpegLSMetadataInfo(frame.PixelWidth, frame.PixelHeight, 8, componentCount);
                if (componentCount == 3)
                {
                    info.InterleaveMode = JpegLSInterleaveMode.Line;

                    // PixelFormat is Bgr24. CharLS expects RGB byte stream.
                    // By enabling this CharLS will transform input before decoding.
                    info.OutputBgr = true;
                }

                // Compress.
                var compressedPixels = JpegLSCodec.Compress(info, uncompressedPixels, true);

                Save(compressedPixels.Array, compressedPixels.Count, GetOutputPath(inputPath));
            }
            catch (FileNotFoundException e)
            {
                Console.WriteLine("Error: " + e.Message);
            }

            return Success;
        }

        private static bool TryParseArguments(string[] args, out string inputPath)
        {
            inputPath = string.Empty;

            if (args.Length != 1)
                return false;

            inputPath = args[0];
            return true;
        }

        private static bool TryGetComponentCount(PixelFormat format, out int componentCount)
        {
            // For this sample: only RGB color and 8 bit monochrome images are supported.
            if (format == PixelFormats.Bgr24)
            {
                componentCount = 3;
            }
            else if (format == PixelFormats.Gray8)
            {
                componentCount = 1;
            }
            else
            {
                componentCount = 0;
            }

            return componentCount != 0;
        }

        private static string GetOutputPath(string inputPath)
        {
            return Path.ChangeExtension(inputPath, ".jls");
        }

        private static void Save(byte[] pixels, int count, string path)
        {
            using (var output = new FileStream(path, FileMode.OpenOrCreate))
            {
                output.Write(pixels, 0, count);
            }
        }
    }
}
