# Qt Huge Containers
### A Qt library for containers too big to be held in memory

## Description
Often, the main constrain to the size of a container is just how much RAM you machine has.
This library aims at eliminating this constrain.
You will be able to decide the maximum number of items to be retained in memory and the rest will be stored on the hard drive.
The interface is the same as that of normal Qt containers so you can just drop in this class to your existing code.

## Performance
**TODO**
You can find performance statistics regarding memory usage and execution speed in the documentation

## Requirements
**TODO**
[Qt 5.7](https://www.qt.io/) or higher and a compiler that supports C++11 (actually the subset of C++11 supported by MSVC13 is enough).

## Installation
This is a header only library. Just copy `hugecontainer.h` into your project directory and `#include` it.

If you want to build and run the tests and the benchmarks you can just invoke qmake and make from the root directory.

## Documenation
You can build the documentation yourself using [Doxygen](www.doxygen.org) or you can browse it online.

## License
License of the main library is MIT, this is less restrictive than Qt's licenses.
This has been done to free users from certain requirements included in LGPL when using this code.

## Future releases
Future releases of this library will include (in appoximate order)
2. Optional thread safety
3. Optional encryption of data stored on disk using [Crypto++](https://www.cryptopp.com/) and/or [OpenSSL's Libcrypto](https://www.openssl.org/)
4. Adding a version of the library using only STL and no Qt

If you think of any other improvement feel free to open a ticket