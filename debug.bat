@echo off
echo 启动Android调试...

REM 设置端口转发
echo 设置端口转发...
adb forward tcp:1234 tcp:1234

echo 请先在Android设备上运行: ./lldb-server platform --server --listen "*:1234"
echo 然后按任意键继续...
pause

echo 启动lldb调试器...
lldb
echo platform select remote-android
echo platform connect connect://localhost:1234
echo process attach --name com.kr.test
echo breakpoint set --name Trimpline
echo breakpoint set --name AfterHookTrampoline
echo continue
