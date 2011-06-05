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
        /// The pixel data is not multi-component.
        /// </summary>
        None,

        /// <summary>
        /// The interleave mode is by line. A full line of each 
        /// component is encoded before moving to the next line.
        /// </summary>
        Line,

        /// <summary>
        /// The interleave mode is by sample. A sample from each component is processed in turn.
        /// </summary>
        Sample
    }
}
