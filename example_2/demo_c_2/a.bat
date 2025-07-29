@echo off
setlocal enabledelayedexpansion

echo 开始处理文件重命名...
echo.

:: 处理所有 *_c.txt -> .c
for /r %%i in (*_c.txt) do (
    set "oldpath=%%~fi"
    set "newpath=!oldpath:_c.txt=.c!"
    echo 重命名: "!oldpath!" -> "!newpath!"
    move "!oldpath!" "!newpath!"
)

:: 处理所有 *_h.txt -> .h
for /r %%i in (*_h.txt) do (
    set "oldpath=%%~fi"
    set "newpath=!oldpath:_h.txt=.h!"
    echo 重命名: "!oldpath!" -> "!newpath!"
    move "!oldpath!" "!newpath!"
)

echo.
echo 文件重命名完成！
pause
