@echo off

cd /D %1

if not exist %2 (
    mkdir %2
)

cd %2

cmake -A Gaming.Xbox.XboxOne.x64 ^
    -DHALLEY_PATH=../halley ^
    -DBUILD_HALLEY_TOOLS=0 ^
    -DBUILD_HALLEY_TESTS=0 ^
    -DHALLEY_IGNORE_CONSOLES=0 ^
    -D_GAMING_XBOX=1 ^
    -D_GAMING_XBOX_XBOXONE=1 ^
    .. || exit /b 1

echo Configuration done.
