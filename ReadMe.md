This project illustrate issue with liking Boost in Debug build. In Release it's all Ok.
Used environment: 
Windows10 + MSYS2 + MinGW64 
GCC 14.2.0 
Cmake 3.30.4 
Boost 1.86.0
Boost installed with:  ./b2 --build-type=complete toolset=gcc threading=multi link=static runtime-link=static runtime-debugging=on --prefix=d:/boost install

Difference between Debug and Release is only CMAKE flags: CMAKE_BUILD_TYPE=Debug or Release