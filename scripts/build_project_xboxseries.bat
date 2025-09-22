@echo off

cd /D %1

if not exist %2 (
    mkdir %2
)

cd %2

cmake -A Gaming.Xbox.Scarlett.x64 ^
    -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY ^
    -DHALLEY_PATH=../halley ^
    -DBUILD_HALLEY_TOOLS=0 ^
    -DBUILD_HALLEY_TESTS=0 ^
    -DHALLEY_IGNORE_CONSOLES=0 ^
    -D_GAMING_XBOX=1 ^
    -D_GAMING_XBOX_SCARLETT=1 ^
    .. || exit /b 1

:: - copy meta file(s)
:: - create symbolic link to assets folder

if not exist "%1\assets-xboxseries" (
    mkdir "%1\assets-xboxseries"
)

set deploy="%1\%2\Gaming.Xbox.Scarlett.x64\Layout\Image\Loose"

if not exist "%deploy%" (
    mkdir "%deploy%"
)

if not exist "%deploy%\assets" (
    mklink /J "%deploy%\assets" "%1\assets-xboxseries"
)

echo Configuration done.
