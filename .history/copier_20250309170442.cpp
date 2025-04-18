#include <windows.h>
#include <shobjidl.h>        // 包含 IFileOperation 接口定义
#include <shellscalingapi.h> // 包含 SetProcessDpiAwareness 函数
#include <shlobj.h>          // 包含 SHCreateItemFromParsingName 函数
#include <stdexcept>         // 包含 std::runtime_error
#include <filesystem>
#include <iostream>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/complex.h>
#include <string>

#pragma comment(lib, "shell32.lib") // 链接 shell32.lib

namespace py = pybind11;
namespace fs = std::filesystem;

enum class FileOperationType
{
    COPY = 1,
    MOVE = 2,
    RM = 3,
};

enum class FileOperationResult
{
    SUCCESS = 0,
    PathNotExists = -2,
    NoOperationInstance = -3,
    FaileToCreOperationInstance = -4,
    FailToSetOperationFlags = -5,
    FailToCreSrcShellItem = -6,
    FailToCreDstShellItem = -7,
    FailToAddOperation = -8,
    FailToPerformOperation = -9,
    WrongOperationType = -10,
    FailToConfig = -11,
    UpperDirNotExists = -12,
    DstIsNotDir = -13,
    DstIsNotExists = -14,
    UnknownError = -15,
};

struct FileOperationSet
{
    bool NoProgressUI;
    bool NoneUI;
    bool NoConfirmation;
    bool NoErrorUI;
    bool NoConfirmationForMakeDir;
    bool WarningIfPermanentDelete;
    FileOperationSet()
    {
        NoProgressUI = false;
        NoneUI = false;
        NoConfirmation = false;
        NoErrorUI = false;
        NoConfirmationForMakeDir = true;
        WarningIfPermanentDelete = false;
    }
    FileOperationSet(bool NoProgressUI, bool NoneUI, bool NoConfirmation, bool NoErrorUI, bool NoConfirmationForMakeDir, bool WarningIfPermanentDelete)
    {
        this->NoProgressUI = NoProgressUI;
        this->NoneUI = NoneUI;
        this->NoConfirmation = NoConfirmation;
        this->NoErrorUI = NoErrorUI;
        this->NoConfirmationForMakeDir = NoConfirmationForMakeDir;
        this->WarningIfPermanentDelete = WarningIfPermanentDelete;
    }
};

int IsDirectory(const std::wstring &path)
{
    DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        return -1;
    }
    if (attributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        return 1;
    }
    return 0;
}

std::wstring Extension(const std::wstring &path)
{
    fs::path p(path);
    return p.extension().wstring();
}

std::wstring BaseName(const std::wstring &path)
{
    fs::path p(path);
    return p.filename().wstring();
}

std::wstring Dirname(const std::wstring &path)
{
    fs::path p(path);
    return p.parent_path().wstring();
}

using FR = FileOperationResult;
using OR = std::variant<std::vector<std::pair<std::wstring, FR>>, FR>;

class ExplorerAPI
{
public:
    IFileOperation *pFileOp;
    ExplorerAPI()
    {
        // 初始化 COM 库
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (FAILED(hr))
        {
            throw std::runtime_error("COM initialization failed");
        }
    }

    ~ExplorerAPI()
    {
        // 反初始化 COM 库
        if (pFileOp != nullptr)
        {
            pFileOp->Release();
            pFileOp = nullptr;
        }
        CoUninitialize();
    }

    FileOperationResult Config(FileOperationSet settings)
    {
        if (pFileOp == nullptr)
        {
            return FileOperationResult::NoIFileOperationInstance;
        }
        DWORD flags = 0;
        if (settings.NoProgressUI)
            flags |= FOF_SILENT;
        if (settings.NoneUI)
            flags |= FOF_NO_UI;
        if (settings.NoConfirmation)
            flags |= FOF_NOCONFIRMATION;
        if (settings.NoErrorUI)
            flags |= FOF_NOERRORUI;
        if (settings.NoConfirmationForMakeDir)
            flags |= FOF_NOCONFIRMMKDIR;
        if (settings.WarningIfPermanentDelete)
            flags |= FOF_WANTNUKEWARNING;
        HRESULT hr = pFileOp->SetOperationFlags(flags);
        if (FAILED(hr))
        {
            return FileOperationResult::FailToSetOperationFlags;
        }
        return FileOperationResult::SUCCESS;
    }

    FileOperationResult Init(FileOperationSet set)
    {
        SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE); // 设置 DPI 感知
        HRESULT hr = CoCreateInstance(
            CLSID_FileOperation, // CLSID_FileOperation 是 IFileOperation 的类标识符
            nullptr,
            CLSCTX_ALL,            // 在所有上下文中创建
            IID_PPV_ARGS(&pFileOp) // 获取 IFileOperation 接口
        );
        if (FAILED(hr))
        {
            return FileOperationResult::FailToCreateIFileOperationInstance;
        }

        FileOperationResult rs = Config(set);
        switch (rs)
        {
        case FileOperationResult::SUCCESS:
            return FileOperationResult::SUCCESS;
        default:
            pFileOp->Release();
            pFileOp = nullptr;
            return rs;
        }
    }

    OR src2dst(std::vector<std::wstring> srcs, std::wstring dst, FileOperationType action)
    {
        HRESULT hr;
        std::vector<std::pair<std::wstring, FR>> result;

        // 检查目标路径是否为目录
        int rc = IsDirectory(dst);
        switch (rc)
        {
        case -1:
            return FileOperationResult::DstIsNotExists;
        case 0:
            return FileOperationResult::DstIsNotDir;
        case 1:
            break;
        default:
            return FileOperationResult::UnknownError;
        }

        // 使用智能指针管理 COM 对象
        CComPtr<IFileOperation> pFileOp;
        hr = CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pFileOp));
        if (FAILED(hr))
            return FR::FailToCreateFileOp;

        // 设置操作标志（如静默模式）
        pFileOp->SetOperationFlags(FOF_NO_UI | FOF_ALLOWUNDO);

        // 创建目标 ShellItem
        CComPtr<IShellItem> pDestinationItem;
        hr = SHCreateItemFromParsingName(dst.c_str(), nullptr, IID_PPV_ARGS(&pDestinationItem));
        if (FAILED(hr))
            return FR::FailToCreDstShellItem;

        // 遍历源路径
        for (const auto &src : srcs)
        {
            if (!fs::exists(src))
            {
                result.emplace_back(src, FR::PathNotExists);
                continue;
            }

            CComPtr<IShellItem> pSourceItem;
            hr = SHCreateItemFromParsingName(src.c_str(), nullptr, IID_PPV_ARGS(&pSourceItem));
            if (FAILED(hr))
            {
                result.emplace_back(src, FR::FailToCreSrcShellItem);
                continue;
            }

            // 执行操作
            switch (action)
            {
            case FileOperationType::COPY:
                hr = pFileOp->CopyItem(pSourceItem, pDestinationItem, nullptr, nullptr);
                break;
            case FileOperationType::MOVE:
                hr = pFileOp->MoveItem(pSourceItem, pDestinationItem, nullptr, nullptr);
                break;
            default:
                result.emplace_back(src, FR::WrongOperationType);
                continue;
            }

            if (SUCCEEDED(hr))
            {
                result.emplace_back(src, FR::SUCCESS);
            }
            else
            {
                result.emplace_back(src, FR::OperationFailed);
            }
        }

        // 执行所有操作
        hr = pFileOp->PerformOperations();
        BOOL bAborted = FALSE;
        pFileOp->GetAnyOperationsAborted(&bAborted);
        if (FAILED(hr) || bAborted)
        {
            return FR::FailToPerformOperation;
        }

        return result;
    }

    OR copy(std::vector<std::wstring> srcs, std::wstring dst)
    {
        return src2dst(srcs, dst, FileOperationType::COPY);
    }

    OR move(std::vector<std::wstring> srcs, std::wstring dst)
    {
        return src2dst(srcs, dst, FileOperationType::MOVE);
    }

    OR remove(std::vector<std::wstring> paths)
    {
        HRESULT hr;
        std::vector<std::pair<std::wstring, FR>> result = {};
        for (auto path : paths)
        {
            IShellItem *pItem = NULL;
            HRESULT hr = SHCreateItemFromParsingName(path.c_str(), NULL, IID_PPV_ARGS(&pItem));
            if (FAILED(hr))
            {
                result.push_back(std::pair(path, FR::FailToCreSrcShellItem));
                continue;
            }
            hr = pFileOp->DeleteItem(pItem, nullptr);
            if (FAILED(hr))
            {
                pItem->Release();
                result.push_back(std::pair(path, FR::FailToAddOperation));
                continue;
            }
            result.push_back(std::pair(path, FR::SUCCESS));
        }
        hr = pFileOp->PerformOperations();
        if (FAILED(hr))
        {
            return FR::FailToPerformOperation;
        }
        return result;
    }
};

PYBIND11_MODULE(WinFile, m)
{
    py::enum_<FileOperationType>(m, "FileOperationType")
        .value("COPY", FileOperationType::COPY)
        .value("MOVE", FileOperationType::MOVE)
        .value("REMOVE", FileOperationType::RM);
    py::class_<FileOperationSet>(m, "FileOperationSet")
        .def(py::init<>())
        .def(py::init<bool, bool, bool, bool, bool, bool>(), py::arg("NoProgressUI"), py::arg("NoneUI"), py::arg("NoConfirmation"), py::arg("NoErrorUI"), py::arg("NoConfirmationForMakeDir"), py::arg("WarningIfPermanentDelete"))
        .def_readwrite("NoProgressUI", &FileOperationSet::NoProgressUI)
        .def_readwrite("NoneUI", &FileOperationSet::NoneUI)
        .def_readwrite("NoConfirmation", &FileOperationSet::NoConfirmation)
        .def_readwrite("NoErrorUI", &FileOperationSet::NoErrorUI)
        .def_readwrite("NoConfirmationForMakeDir", &FileOperationSet::NoConfirmationForMakeDir)
        .def_readwrite("WarningIfPermanentDelete", &FileOperationSet::WarningIfPermanentDelete);
    py::enum_<FileOperationResult>(m, "FileOperationResult")
        .value("SUCCESS", FileOperationResult::SUCCESS)
        .value("NoIFileOperationInstance", FileOperationResult::NoIFileOperationInstance)
        .value("FailToCreateIFileOperationInstance", FileOperationResult::FailToCreateIFileOperationInstance)
        .value("FailToSetOperationFlags", FileOperationResult::FailToSetOperationFlags)
        .value("FailToCreateSourceIShellItem", FileOperationResult::FailToCreateSourceIShellItem)
        .value("FailToCreateDestinationIShellItem", FileOperationResult::FailToCreateDestinationIShellItem)
        .value("FailToAddOperation", FileOperationResult::FailToAddOperation)
        .value("FailToPerformOperation", FileOperationResult::FailToPerformOperation)
        .value("WrongOperationType", FileOperationResult::WrongOperationType)
        .value("FailToConfig", FileOperationResult::FailToConfig)
        .value("UpperDirNotExists", FileOperationResult::UpperDirNotExists)
        .value("SourceNotExists", FileOperationResult::SourceNotExists);
    py::class_<ExplorerAPI>(m, "ExplorerAPI")
        .def(py::init<>())
        .def("Config", &ExplorerAPI::Config, py::arg("set"))
        .def("Init", &ExplorerAPI::Init, py::arg("set"))
        .def("action", &ExplorerAPI::action, py::arg("src"), py::arg("dst"), py::arg("action"))
        .def("rm", &ExplorerAPI::rm, py::arg("path"))
        .def("copy", &ExplorerAPI::copy, py::arg("src"), py::arg("dst"))
        .def("move", &ExplorerAPI::move, py::arg("src"), py::arg("dst"));
}
