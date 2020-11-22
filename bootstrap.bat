git submodule update --init --recursive || goto :error

pushd lib || goto :error
call bootstrap-vcpkg.bat || goto :error
vcpkg.exe install ^
    glfw3:x64-windows-static ^
    glm:x64-windows-static ^
    || goto :error
popd

echo Bootstrap success!
exit /b

:error
echo Bootstrap failed...
exit /b %errorlevel%
