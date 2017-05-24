@echo off
SET OUT_DIR="%~pd0bin\."
@echo Copying ComPort files to output directory
copy "%~pd0components\common\ComPort\bin\win32\ComPort*.dll" %OUT_DIR%
copy "%~pd0components\common\ComPort\bin\win32\ComPort*.lib" %OUT_DIR%
@echo Copying test application binary to output directory.
copy "%~pd0source\sblAppEx\blinky_backdoor_select_btn2538.bin" %OUT_DIR%
copy "%~pd0source\sblAppEx\blinky_backdoor_select_btn2650.bin" %OUT_DIR%
copy "%~pd0source\sblAppEx\blinky_backdoor_select_btn2650_early.bin" %OUT_DIR%
