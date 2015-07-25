//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use.
//

namespace CharLS
{
    /// <summary>
    /// Defines the interleave mode for multi-component (color) pixel data.
    /// </summary>
    public enum JpegLSInterleaveMode
    {
        /// <summary>
        /// The encoded pixel data is not interleaved but stored as component for component: RRRGGGBBB.
        /// Also default option for pixel data with only 1 component.
        /// </summary>
        None = 0,

        /// <summary>
        /// The interleave mode is by line. A full line of each
        /// component is encoded before moving to the next line.
        /// </summary>
        Line = 1,

        /// <summary>
        /// The data is stored by sample (pixel). For color image this is the format like RGBRGBRGB.
        /// </summary>
        Sample = 2
    }
}
