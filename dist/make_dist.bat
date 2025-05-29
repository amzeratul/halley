set /p version=<..\bin\build_version.txt

rd /s /q halley
del /f /q *.zip
mkdir halley
mkdir halley\bin

copy ..\bin\halley-editor.exe halley\bin\halley-editor.exe /Y
copy ..\bin\SDL2.dll halley\bin\SDL2.dll /Y
copy ..\bin\ShaderConductor.dll halley\bin\ShaderConductor.dll /Y
copy ..\bin\dxcompiler.dll halley\bin\dxcompiler.dll /Y
copy ..\bin\dxil.dll halley\bin\dxil.dll /Y
copy ..\bin\libcrypto-3-x64.dll halley\bin\libcrypto-3-x64.dll /Y
copy ..\bin\libssl-3-x64.dll halley\bin\libssl-3-x64.dll /Y
copy ..\bin\build_version.txt halley\bin\build_version.txt /Y

robocopy ..\assets halley\assets /E

7z a -tzip halley-editor-%version%.zip halley
pause
