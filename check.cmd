#  -I "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\include"

"C:\Program Files\Cppcheck\cppcheck.exe" --enable=all  --inconclusive --platform=win32A -D_WIN32 -D_M_X64 -D_M_AMD64 -D__cplusplus -D_MSC_VER=1900 --verbose --report-progress src