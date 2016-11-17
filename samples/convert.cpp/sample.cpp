


https://files.idrsolutions.com/jdeli-javadoc/com/idrsolutions/image/jpeg/JpegEncoder.html

Java
JpegEncoder encoder = new JpegEncoder();
 //encoder.setQuality(75);
 encoder.write(image, outputStream);
 
 C
  
 Initialize(charls_decoder* decoder_state, void * source, bool read_header);
 ReadHeader
 Decode()
 
 
 C++
 auto encoder = new jpeg_ls_encoder();
 encoder.
 
 
 C++
 charls::jpegls_decoder decoder(void * source, bool read_header = true);
 charls::jpegls_decoder decoder(void * sourceFunction, bool read_header = true);
 
 decoder.get_comment_count();
 std::string msg = decoder.get_comment();
 
 
 decoder.
 decoder.read_header();
 decoder.decode();
 
 C#
 var decoder = JpegLSBitmapDecoder(stream)
 
 
 UWP 
 