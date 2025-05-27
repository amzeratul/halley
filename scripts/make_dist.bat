cd ..

set /p version=<bin\build_version.txt

rd /s /q dist
mkdir dist
mkdir dist\halley
mkdir dist\halley\bin

copy bin\halley-editor.exe dist\halley\bin\halley-editor.exe /Y
copy bin\SDL2.dll dist\halley\bin\SDL2.dll /Y
copy bin\ShaderConductor.dll dist\halley\bin\ShaderConductor.dll /Y
copy bin\dxcompiler.dll dist\halley\bin\dxcompiler.dll /Y
copy bin\dxil.dll dist\halley\bin\dxil.dll /Y
copy bin\libcrypto-3-x64.dll dist\halley\bin\libcrypto-3-x64.dll /Y
copy bin\libssl-3-x64.dll dist\halley\bin\libssl-3-x64.dll /Y
copy bin\build_version.txt dist\halley\bin\build_version.txt /Y

robocopy assets dist\halley\assets /E

cd dist
7z a -tzip halley-editor-%version%.zip *
pause
