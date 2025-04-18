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
#include <wil/com.h>

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
    FailToInitCOM = -16,
    NoIFileOperationInstance = -17,
    FailToCreateIFileOperationInstance = -18,
    OperationAborted = -19,
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

using FOR = FileOperationResult;
using TOR = std::variant<std::vector<std::pair<std::wstring, FOR>>, FOR>;

class ExplorerAPI
{
public:
    IFileOperation *pFileOp;
    bool isInit = false;
    ExplorerAPI(FileOperationSet set = FileOperationSet())
    {
        FOR rs = Init(set);
        if (rs != FOR::SUCCESS)
        {
            isInit = false;
            pFileOp = nullptr;
        }
        isInit = true;
    }

    ~ExplorerAPI()
    {
        if (pFileOp != nullptr)
        {
            pFileOp->Release();
            pFileOp = nullptr;
        }
        CoUninitialize();
    }

    DWORD GetFlags(FileOperationSet settings)
    {
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
        return flags;
    }

    FOR Init(FileOperationSet set)
    {
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (FAILED(hr))
        {
            return FOR::FailToInitCOM;
        }

        SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE); // 设置 DPI 感知
        HRESULT hr = CoCreateInstance(
            CLSID_FileOperation, // CLSID_FileOperation 是 IFileOperation 的类标识符
            nullptr,
            CLSCTX_ALL,            // 在所有上下文中创建
            IID_PPV_ARGS(&pFileOp) // 获取 IFileOperation 接口
        );
        DWORD flags = GetFlags(set);
        HRESULT hr = pFileOp->SetOperationFlags(flags);
        if (FAILED(hr))
        {
            return FOR::FailToCreateIFileOperationInstance;
        }

        DWORD rs = GetFlags(set);

        if (FAILED(hr))
        {
            return FOR::FailToSetOperationFlags;
        }
        return FOR::SUCCESS;
    }

    TOR src2dst(
        const std::vector<std::wstring> &srcs,
        const std::wstring &dst,
        FileOperationType action)
    {
        std::vector<std::pair<std::wstring, FOR>> results;
        switch (IsDirectory(dst))
        {
        case 1:
            break;
        case 0:
            return FileOperationResult::DstIsNotDir;
        case -1:
            return FileOperationResult::DstIsNotExists;
        default:
            return FileOperationResult::UnknownError;
        }

        wil::com_ptr<IShellItem> pDstItem;
        HRESULT hr = SHCreateItemFromParsingName(dst.c_str(), nullptr, IID_PPV_ARGS(&pDstItem));
        if (FAILED(hr))
        {
            return FOR::FailToCreDstShellItem;
        }

        pFileOp->SetOperationFlags(FOF_NO_UI | FOF_ALLOWUNDO);

        for (const auto &src : srcs)
        {
            if (!std::filesystem::exists(src))
            {
                results.emplace_back(std::pair(src, FOR::PathNotExists));
                continue;
            }

            wil::com_ptr<IShellItem> pSrcItem;
            hr = SHCreateItemFromParsingName(src.c_str(), nullptr, IID_PPV_ARGS(&pSrcItem));
            if (FAILED(hr))
            {
                results.emplace_back(std::pair(src, FOR::FailToCreSrcShellItem));
                continue;
            }

            switch (action)
            {
            case FileOperationType::COPY:
                hr = pFileOp->CopyItem(pSrcItem.get(), pDstItem.get(), nullptr, nullptr);
                break;
            case FileOperationType::MOVE:
                hr = pFileOp->MoveItem(pSrcItem.get(), pDstItem.get(), nullptr, nullptr);
                break;
            default:
                results.emplace_back(std::pair(src, FOR::WrongOperationType));
                continue;
            }

            results.emplace_back(std::pair(src, SUCCEEDED(hr) ? FOR::SUCCESS : FOR::FailToAddOperation));
        }

        hr = pFileOp->PerformOperations();
        if (FAILED(hr))
        {
            return FOR::FailToPerformOperation;
        }

        BOOL aborted = FALSE;
        pFileOp->GetAnyOperationsAborted(&aborted);
        if (aborted)
        {
            return FOR::OperationAborted;
        }

        return results;
    }

    TOR copy(std::vector<std::wstring> srcs, std::wstring dst)
    {
        return src2dst(srcs, dst, FileOperationType::COPY);
    }

    TOR move(std::vector<std::wstring> srcs, std::wstring dst)
    {
        return src2dst(srcs, dst, FileOperationType::MOVE);
    }

    TOR remove(std::vector<std::wstring> paths)
    {
        HRESULT hr;
        std::vector<std::pair<std::wstring, FOR>> result = {};
        for (auto path : paths)
        {
            if (!std::filesystem::exists(path))
            {
                result.push_back(std::pair(path, FOR::PathNotExists));
                continue;
            }

            wil::com_ptr<IShellItem> pItem;
            HRESULT hr = SHCreateItemFromParsingName(path.c_str(), NULL, IID_PPV_ARGS(&pItem));
            if (FAILED(hr))
            {
                result.push_back(std::pair(path, FOR::FailToCreSrcShellItem));
                continue;
            }
            hr = pFileOp->DeleteItem(pItem.get(), nullptr);
            result.push_back(std::pair(path, SUCCEEDED(hr) ? FOR::SUCCESS : FOR::FailToAddOperation));
        }
        hr = pFileOp->PerformOperations();
        if (FAILED(hr))
        {
            return FOR::FailToPerformOperation;
        }
        BOOL aborted = FALSE;
        pFileOp->GetAnyOperationsAborted(&aborted);
        if (aborted)
        {
            return FOR::OperationAborted;
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
