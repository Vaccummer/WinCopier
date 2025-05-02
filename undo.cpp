#include <Windows.h>
#include <ShellAPI.h>
#include <iostream>
void UndoLastFileOperation()
{
    // 发送撤销命令（等效于 Ctrl+Z）
    SHELLEXECUTEINFO sei = {sizeof(sei)};
    sei.lpClass = "undo";
    sei.lpFile = "";
    sei.nShow = SW_HIDE; // 静默执行
    sei.fMask = SEE_MASK_INVOKEIDLIST;

    if (!ShellExecuteEx(&sei))
    {
        // 处理错误
        DWORD err = GetLastError();
        std::cout << "撤销失败，错误码: 0x" << std::hex << err << std::endl;
    }
}
int main()
{
    UndoLastFileOperation();
    return 0;
}
