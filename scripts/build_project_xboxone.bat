@echo off

cd /D %1

if not exist %2 (
    mkdir %2
)

cd %2

cmake -A Gaming.Xbox.XboxOne.x64 ^
    -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY ^
    -DHALLEY_PATH=../halley ^
    -DBUILD_HALLEY_TOOLS=0 ^
    -DBUILD_HALLEY_TESTS=0 ^
    -DHALLEY_IGNORE_CONSOLES=0 ^
    -D_GAMING_XBOX=1 ^
    -D_GAMING_XBOX_XBOXONE=1 ^
    .. || exit /b 1

:: - copy meta file(s)
:: - create symbolic link to assets folder
set deploy="%1\%2\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose"

if not exist "%1\assets-xboxone" (
    mkdir "%1\assets-xboxone"
)

if not exist "%deploy%\assets" (
    mklink /J "%deploy%\assets" "%1\assets-xboxone"
)

echo Configuration done.
