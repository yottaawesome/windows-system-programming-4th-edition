# Windows System Programming 4th edition source code

## Introduction

This is an unofficial source code repo for Johnson M. Hart's book [Windows System Programming 4th edition](https://www.ebooks.com/en-au/book/726522/windows-system-programming/hart-johnson-m/). No representation is made that the source code belongs to me, it's simply reproduced here for convenience (so you don't need to dig up the physical media of the book) and remains the copyright of Johnson M. Hart.

## Building

You'll need Visual Studio 2022 with the _Desktop development with C++_ workload installed. Open the solution file `src\windows-system-programming.sln` you should be able to build it immediately (note that the project conversion is incomplete, so not all projects have been converted and added). The old project files can be found in `Projects2005`, `Projects2008` and `Projects2055_64` -- these are gradually getting converted and will be deleted eventually.

## Changes

Currently, the source code in `src` is committed as is from the [book's website](http://jmhartsoftware.com/) (this includes libs and exe files). While there are Visual Studio 2005 and 2008 (x64) solutions provided, I'm in the process of adding a new VS2022 solution and associated projects to replace these.

| Chapter | Status      | Notes |
| --------|-------------| ----- |
| 1       | Converted   | |
| 2       | Converted   | |
| 3       | Converted   | |
| 4       | Converted   | |
| 5       | Converted   | |
| 6       | Converted   | |
| 7       | Converted   | |
| 8       | Converted   | |
| 9       | Converted   | |
| 10      | Deferred    | Uses a very old version of pthreads. Multiple compile errors due to bad syntax or references to missing symbols. |
| 11      | Converted   | |
| 12      | In progress | |

## Additional resources

* [Build desktop Windows apps using the Win32 API](https://docs.microsoft.com/en-us/windows/win32/)
* [Get Started with Win32 and C++](https://docs.microsoft.com/en-us/windows/win32/learnwin32/learn-to-program-for-windows)
* [Windows API Index](https://docs.microsoft.com/en-us/windows/win32/apiindex/windows-api-list)
* [COM and ATL](https://docs.microsoft.com/en-us/cpp/atl/introduction-to-com-and-atl)
* [C++/WinRT](https://docs.microsoft.com/en-us/windows/uwp/cpp-and-winrt-apis/)
* [theForger's Win32 API Programming Tutorial](http://www.winprog.org/tutorial/). Like this book, the tutorials are quite old, but still useful. Tutorial source code is available on that site as a zip, and I've [also got a repo of it](https://github.com/yottaawesome/forger-win32-tutorial).
* [Windows Classic Samples](https://github.com/microsoft/Windows-classic-samples)
* [Browse Microsoft's C++ samples](https://docs.microsoft.com/en-us/samples/browse/?languages=cpp)
* [Windows via C/C++ 5th edition](https://www.microsoftpressstore.com/store/windows-via-c-c-plus-plus-9780735639218). This is also an excellent, if old, book on Windows system programming. [Source code repo here](https://github.com/yottaawesome/windows-via-c-cpp).
* [ZetCode's Windows API tutorial](https://zetcode.com/gui/winapi/)
