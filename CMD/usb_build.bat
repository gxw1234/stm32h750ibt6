@echo off
echo [Build] Starting build process...

rem Project directories setup
set PROJECT_DIR=%~dp0..
set BUILD_DIR=%PROJECT_DIR%\build

echo [Build] Project directory: %PROJECT_DIR%
echo [Build] Build directory: %BUILD_DIR%

if not exist "%BUILD_DIR%" (
    echo [Build] Creating build directory...
    mkdir "%BUILD_DIR%"
)

pushd "%BUILD_DIR%"

echo [Build] Running CMake...
cmake -G "Ninja" "%PROJECT_DIR%"

echo [Build] Running Ninja...
ninja

rem Check build status
if %ERRORLEVEL% NEQ 0 (
    echo [Build] ERROR: Compilation failed! Please check the errors above.
    set BUILD_STATUS=FAILED
) else (
    echo [Build] SUCCESS: Compilation completed successfully!
    set BUILD_STATUS=SUCCESS
)

rem Check if ELF file exists
if exist usb_f_h.elf (
    echo [Build] Found ELF file, generating binary and hex files...
    
    rem Generate BIN file
    arm-none-eabi-objcopy -O binary usb_f_h.elf usb_app.bin
    echo [Build] Created: usb_app.bin
    
    rem Generate HEX file
    arm-none-eabi-objcopy -O ihex usb_f_h.elf usb_app.hex
    echo [Build] Created: usb_app.hex
) else (
    echo [Build] ERROR: usb_f_h.elf not found!
    echo [Build] Checking for other ELF files...
    dir *.elf
)

popd

rem Display final build status message
if "%BUILD_STATUS%"=="SUCCESS" (
    echo [Build] ================================================================
    echo [Build] BUILD SUCCESSFUL! The firmware has been compiled successfully.
    echo [Build] ================================================================
) else (
    echo [Build] ================================================================
    echo [Build] BUILD FAILED! Please fix the errors and try again.
    echo [Build] ================================================================
)

echo [Build] Build process completed!
