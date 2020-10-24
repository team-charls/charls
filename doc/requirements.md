# Style and Design

## Introduction

The purpose of this document is to capture the requirements that are in and out scope.

## In Scope

### R1 Decode from a memory buffer to a memory buffer

The typical use case is a client application that load JPEG-LS encoded data from a file into a memory buffer
and call decode to get a decoded image that can be displayed on the screen.

### R2 Encode from a memory buffer to a memory buffer

The typical use case is a client application that has an image in memory that needs to be saved.
The library will encode the image to a JPEG-LS byte stream in a memory buffer, which the application
then can save.

## Out Scope

### Decode from a byte stream to a memory buffer

The typical use case is a client application that want to load a JPEG-LS encoded byte stream
from file. The library should then also handling the loading of this byte stream.
To make this process generic, the client needs to provide a callback function that the library
can use when it needs more bytes.
This requirement is out scope as the use case is already covered by R1. The only advantage is that library can
the byte stream in chunks. For the typical image size it is however more efficient to let the client code do
the loading of the byte stream in a buffer.

### Decode from a byte stream to a byte stream

The typical use case for this requirement is decoding of very large images, which would normally cause out-of-memory
conditions. As there is not a direct need for this requirement, it is considered out of scope.

### Encode from a memory buffer to a byte stream

The typical use case for this requirement is save images directly to a file. This is already covered by R2,
using a callback function, would not increase the performance.

### Encode from a byte stream to a byte stream

The typical use case for this requirement is to encode images on a storage medium to JPEG-LS with strict memory
requirements.
