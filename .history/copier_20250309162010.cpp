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
    SourceNotExists = -2,
    NoIFileOperationInstance = -1,
    FailToCreateIFileOperationInstance = 1,
    FailToSetOperationFlags = 2,
    FailToCreateSourceIShellItem = 3,
    FailToCreateDestinationIShellItem = 4,
    FailToAddOperation = 5,
    FailToPerformOperation = 6,
    WrongOperationType = 7,
    FailToConfig = 8,
    UpperDirNotExists = 9,
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

struct OperationTask
{
    std::wstring src;
    std::wstring dst;
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
using OT = OperationTask;
using OR = std::vector<std::pair<std::pair<std::wstring, std::wstring>, FR>>;

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

    FileOperationResult copy(std::vector<std::wstring> srcs, std::wstring dst)
    {
        HRESULT hr;
        OR result = {};

        hr = SHCreateItemFromParsingName(task.dst.c_str(), nullptr, IID_PPV_ARGS(&pDestinationItem));
        if (FAILED(hr))
        {
            result.push_back(std::pair(std::pair(task.src, task.dst), FileOperationResult::FailToCreateDestinationIShellItem));
            pSourceItem->Release();
            continue;
        }

        for (auto task : tasks)
        {
            IShellItem *pSourceItem = nullptr;
            IShellItem *pDestinationItem = nullptr;

            hr = SHCreateItemFromParsingName(task.src.c_str(), nullptr, IID_PPV_ARGS(&pSourceItem));
            if (FAILED(hr))
            {
                result.push_back(std::pair(std::pair(task.src, task.dst), FileOperationResult::FailToCreateSourceIShellItem));
                continue;
            }

            hr = pFileOp->CopyItem(pSourceItem, pDestinationItem, nullptr, nullptr);
            if (FAILED(hr))
            {
                pSourceItem->Release();
                pDestinationItem->Release();
                result.push_back(std::pair(std::pair(task.src, task.dst), FileOperationResult::FailToAddOperation));
                continue;
            }
        }
        hr = pFileOp->PerformOperations();
        if (FAILED(hr))
        {
            result.push_back(std::pair(std::pair(task.src, task.dst), FileOperationResult::FailToPerformOperation));
        }
        return FileOperationResult::SUCCESS;
    }

    FileOperationResult move(const std::wstring &src, const std::wstring &dst)
    {
        std::wstring dst_dir = dst;
        std::wstring dstname = src;
        switch (IsDirectory(src))
        {
        case -1:
            return FileOperationResult::SourceNotExists;
        case 0:
            if (Extension(dst).empty())
            {
                dst_dir = dst;
                dstname = BaseName(src);
            }
            else
            {
                dst_dir = Dirname(dst);
                dstname = BaseName(dst);
            }
            break;
        }
        if (IsDirectory(dst_dir) == -1)
        {
            return FileOperationResult::UpperDirNotExists;
        }
        IShellItem *pItemMoveFrom = NULL;
        IShellItem *pItemMoveTo = NULL;
        HRESULT hr = SHCreateItemFromParsingName(src.c_str(), NULL, IID_PPV_ARGS(&pItemMoveFrom));
        if (FAILED(hr))
        {
            return FileOperationResult::FailToCreateSourceIShellItem;
        }

        hr = SHCreateItemFromParsingName(dst_dir.c_str(), NULL, IID_PPV_ARGS(&pItemMoveTo));
        if (FAILED(hr))
        {
            pItemMoveFrom->Release();
            return FileOperationResult::FailToCreateDestinationIShellItem;
        }

        hr = pFileOp->MoveItem(pItemMoveFrom, pItemMoveTo, dstname.c_str(), nullptr);
        if (FAILED(hr))
        {
            pItemMoveTo->Release();
            pItemMoveFrom->Release();
            return FileOperationResult::FailToAddOperation;
        }

        // Perform all operations
        hr = pFileOp->PerformOperations();
        if (FAILED(hr))
        {
            pItemMoveTo->Release();
            pItemMoveFrom->Release();
            return FileOperationResult::FailToPerformOperation;
        }
        pItemMoveTo->Release();
        pItemMoveFrom->Release();

        return FileOperationResult::SUCCESS;
    }

    FileOperationResult action(const std::wstring &src, const std::wstring &dst, FileOperationType action)
    {
        if (pFileOp == nullptr)
        {
            return FileOperationResult::FailToCreateIFileOperationInstance;
        }
        switch (action)
        {
        case FileOperationType::COPY:
            return copy(src, dst);
        case FileOperationType::MOVE:
            return move(src, dst);
        case FileOperationType::RM:
            return rm(src);
        default:
            return FileOperationResult::WrongOperationType;
        }
    }

    FileOperationResult remove(const std::wstring &path)
    {
        IShellItem *pItem = NULL;
        HRESULT hr = SHCreateItemFromParsingName(path.c_str(), NULL, IID_PPV_ARGS(&pItem));
        if (FAILED(hr))
        {
            return FileOperationResult::FailToCreateSourceIShellItem;
        }
        hr = pFileOp->DeleteItem(pItem, nullptr);
        if (FAILED(hr))
        {
            pItem->Release();
            return FileOperationResult::FailToAddOperation;
        }
        hr = pFileOp->PerformOperations();
        if (FAILED(hr))
        {
            pItem->Release();
            return FileOperationResult::FailToPerformOperation;
        }
        pItem->Release();
        return FileOperationResult::SUCCESS;
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
