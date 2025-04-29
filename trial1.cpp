#include <Windows.h>
#include <ShlObj.h>
#include <shobjidl.h>
#include <ShellScalingApi.h>
void CopyFileWithUI()
{
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    IFileOperation *pFileOp;
    HRESULT hr = CoCreateInstance(
        CLSID_FileOperation,
        nullptr,
        CLSCTX_ALL,
        IID_PPV_ARGS(&pFileOp));

    if (SUCCEEDED(hr))
    {
        // 显式设置标志：不抑制 UI
        DWORD flags = FOFX_ADDUNDORECORD | FOFX_SHOWELEVATIONPROMPT | FOFX_NOMINIMIZEBOX; // 默认行为（显示对话框）
        pFileOp->SetOperationFlags(flags);

        IShellItem *psiFrom = nullptr;
        IShellItem *psiTo = nullptr;

        // 设置源文件和目标文件夹（示例路径，需替换为实际路径）
        SHCreateItemFromParsingName(L"D:\\Downloads\\CloudMusic", nullptr, IID_PPV_ARGS(&psiFrom));
        SHCreateItemFromParsingName(L"D:\\Document\\Desktop\\123", nullptr, IID_PPV_ARGS(&psiTo));

        // 添加复制操作
        pFileOp->CopyItem(psiFrom, psiTo, nullptr, nullptr);

        // 执行操作（此处应弹出进度对话框）
        hr = pFileOp->PerformOperations();

        // 清理资源
        psiFrom->Release();
        psiTo->Release();
        pFileOp->Release();
    }

    CoUninitialize();
}
int main()
{
    CopyFileWithUI();
    return 0;
}