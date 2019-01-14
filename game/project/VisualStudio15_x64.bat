@echo off
set PATH="C:\Program Files\CMake\bin\";%PATH%
mkdir VisualStudio15
cd VisualStudio15
cmake -G "Visual Studio 15" ../
pause