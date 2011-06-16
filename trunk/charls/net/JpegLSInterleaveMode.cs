//
// (C) Jan de Vaan 2007-2011, all rights reserved. See the accompanying "License.txt" for licensed use.
//

namespace CharLS
{
    /// <summary>
    /// Defines the interleave mode for multi-component (color) pixel data.
    /// </summary>
    public enum JpegLSInterleaveMode
    {
        /// <summary>
        /// The pixel data stored as component for component: RRRGGGBBB.
        /// Also default option for pixel data with only 1 component.
        /// </summary>
        Planar,

        /// <summary>
        /// The interleave mode is by line. A full line of each 
        /// component is encoded before moving to the next line.
        /// </summary>
        Line,

        /// <summary>
        /// The data is stored by pixel. For color image this is the format like RGBRGBRGB.
        /// </summary>
        Pixel
    }
}
