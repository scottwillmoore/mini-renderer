cmake ^
    -G "Visual Studio 16 2019" ^
    -A x64 ^
    -S . ^
    -B build ^
    -D VCPKG_TARGET_TRIPLET=x64-windows-static ^
    -D CMAKE_TOOLCHAIN_FILE=../lib/scripts/buildsystems/vcpkg.cmake ^
    || goto :error

cmake ^
    --build build ^
    --target ALL_BUILD ^
    --config Release ^
    || goto :error


echo Build success!
exit /b

:error
echo Build failed...
exit /b %errorlevel%