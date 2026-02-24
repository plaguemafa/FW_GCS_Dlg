@echo off
chcp 65001 >nul
echo 正在结束资源管理器以清除图标缓存...
taskkill /f /im explorer.exe 2>nul
timeout /t 2 /nobreak >nul
echo 删除图标缓存文件...
del /a /q "%localappdata%\IconCache.db" 2>nul
del /a /f /q "%localappdata%\Microsoft\Windows\Explorer\iconcache*.db" 2>nul
echo 正在重启资源管理器...
start explorer.exe
echo 完成。请到 exe 所在目录查看图标是否已更新。
pause
