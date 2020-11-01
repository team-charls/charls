JPEG-LS TEST IMAGES
===================

The images included in this package constitute the data set used for
implementing the JPEG-LS conformance test as described in Section 8
and Annex E of the JPEG-LS standard document (ITU-T Rec. T.87|ISO/IEC 14495-1).

Test images
-----------

(See explanation of PPM and PGM formats below)

test8.ppm    - 8-bit color test image (photo+graphics+text+random data).
test8r.pgm   - "red" component of test8
test8g.pgm   - "green" component of test8
test8b.pgm   - "blue" component of test8
test8gr4.pgm - "green" component of test8, sub-sampled 4X in the vertical
               direction
test8bs2.pgm - "blue" component of test8, sub-sampled 2X in both directions
test16.pgm   - 16-bit test gray-scale image (actually 12-bit)

Compressed images
-----------------

- t8cXeY.jls  X=0,1,2  Y=0,3  
  Image TEST8 compressed in color mode x with near-lossless error Y
  (Y = 0 means lossless).

- t16eY.jls            Y=0,3  
  Image test16 compressed with near-lossless error Y.

- t8sse0.jls
- t8sse3.jls  
  Image test8 compressed in line-interleaved mode, with near-lossless
  error 0 and 3 (resp.), and with color components sub-sampled
  as follows:  
  - R: not sub-sampled (256 cols x 256 rows)
  - G: sub-sampled 4x in the vertical direction (256 cols x 64 rows)
  - B: sub-sampled 2x in both directions (128 cols x 128 rows)

- t8nde0.jls
- t8nde3.jls  
  Image TEST8 compressed in line-interleaved mode, with near-lossless
  error 0 and 3 (resp.), and with NON-DEFAULT parameters T1=T2=T3=9,
  RESET=31.

The compressed images are provided to help testers/developers debug their
JPEG-LS implementations. The test images contain a mixture of data that will
exercise many paths in the compression/decompression algorithms. There is
no guarantee, however, that every path of the algorithm is tested.  
THE TEST IMAGES SHOULD NOT BE USED TO BENCHMARK COMPRESSION CAPABILITY.
THEY ARE DESIGNED TO BE "HARD" ON THE JPEG-LS COMPRESSOR. JPEG-LS
MAY DO WORSE THAN YOUR FAVORITE COMPRESSOR ON THE TEST IMAGES.

All compressed images have standard JPEG-LS headers (which follow the
standard JPEG marker syntax).  All compressed images were produced using
default parameters, except for t8nde0.jls and t8nde3.jls as noted above.

PPM and PGM INPUT IMAGE FORMATS
-------------------------------

Uncompressed test images images are in either PGM (gray-scale) or
PPM (3-color) format.  This is of course NOT part of the JPEG-LS
standard.

These formats have an ASCII header consisting of 3 lines of the
following form

- PGM (single component):
    P5  
    cols rows  
    maxval

- PPM (3 components)
    P6  
    cols rows  
    maxval

For PGM, the header is followed by cols*rows samples in binary
format, where cols and rows are the number of columns and rows,
respectively.  A test image "cmpnd2g.pgm" is included in the
archive. Samples have 8 bits if maxval < 256, or 16 bits if
256 <= maxval < 65536.

For PPM, the header is followed by cols*rows TRIPLETS of symbols in
binary format. Each symbol in a triplet represents a color component
value (viewers usually interpret PPM triplets as RGB).
