cd ..

set /p version=<bin\build_version.txt

rd /s /q dist
mkdir dist
mkdir dist\bin

copy bin\halley-editor.exe dist\bin\halley-editor.exe /Y
copy bin\SDL2.dll dist\bin\SDL2.dll /Y
copy bin\ShaderConductor.dll dist\bin\ShaderConductor.dll /Y
copy bin\dxcompiler.dll dist\bin\dxcompiler.dll /Y
copy bin\dxil.dll dist\bin\dxil.dll /Y
copy bin\libcrypto-3-x64.dll dist\bin\libcrypto-3-x64.dll /Y
copy bin\libssl-3-x64.dll dist\bin\libssl-3-x64.dll /Y
copy bin\build_version.txt dist\bin\build_version.txt /Y

robocopy assets dist\assets /E

cd dist
7z a -tzip halley-editor-%version%.zip *
pause