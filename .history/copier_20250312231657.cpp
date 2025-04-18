#include <windows.h>
#include <shobjidl.h>        // 包含 IFileOperation 接口定义
#include <shellscalingapi.h> // 包含 SetProcessDpiAwareness 函数
#include <shlobj.h>          // 包含 SHCreateItemFromParsingName 函数
#include <filesystem>
#include <iostream>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/complex.h>
#include <string>
#include <wil/com.h>
#include <fmt/core.h>

constexpr char *AMLEVEL = "LEVEL";
constexpr char *AMERRORNAME = "ERRORNAME";
constexpr char *AMTARGET = "TARGET";
constexpr char *AMACTION = "ACTION";
constexpr char *AMMESSAGE = "MESSAGE";

constexpr char *AMCRITICAL = "CRITICAL";
constexpr char *AMERROR = "ERROR";
constexpr char *AMWARNING = "WARNING";
constexpr char *AMDEBUG = "DEBUG";
constexpr char *AMINFO = "INFO";

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
    COMInitFailed = -20,
};

struct FileOperationSet
{
    bool NoProgressUI;
    bool NoConfirmation;
    bool NoErrorUI;
    bool NoConfirmationForMakeDir;
    bool WarningIfPermanentDelete;
    bool RenameOnCollision;
    bool AllowAdminPrivilege;
    bool AllowUndo;
    FileOperationSet(bool NoProgressUI = false,
                     bool NoConfirmation = false,
                     bool NoErrorUI = false,
                     bool NoConfirmationForMakeDir = true,
                     bool WarningIfPermanentDelete = false,
                     bool RenameOnCollision = false,
                     bool AllowAdminPrivilege = true,
                     bool AllowUndo = true)
        : NoProgressUI(NoProgressUI),
          NoConfirmation(NoConfirmation),
          NoErrorUI(NoErrorUI),
          NoConfirmationForMakeDir(NoConfirmationForMakeDir),
          WarningIfPermanentDelete(WarningIfPermanentDelete),
          RenameOnCollision(RenameOnCollision),
          AllowAdminPrivilege(AllowAdminPrivilege),
          AllowUndo(AllowUndo) {}
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

std::string tostr(const std::wstring &wstr)
{
    return std::string(wstr.begin(), wstr.end());
}

std::string lpwstr2str(LPWSTR lpwstr)
{
    if (lpwstr == nullptr)
        return ""; // 处理空指针

    int bufferSize = WideCharToMultiByte(
        CP_UTF8,         // 目标编码（UTF-8）
        0,               // 标志（默认）
        lpwstr,          // 输入的宽字符串
        -1,              // 自动计算输入长度
        nullptr,         // 预计算输出缓冲区大小
        0,               // 预计算模式
        nullptr, nullptr // 默认字符处理
    );

    if (bufferSize == 0)
        return ""; // 转换失败

    std::string result(bufferSize, 0);
    WideCharToMultiByte(
        CP_UTF8, 0, lpwstr, -1,
        &result[0], bufferSize, nullptr, nullptr);

    result.pop_back(); // 移除末尾的 '\0'
    return result;
}

using FOR = FileOperationResult;
using TOR = std::variant<std::vector<std::pair<std::wstring, FOR>>, FOR>;

class ExplorerAPI
{
public:
    IFileOperation *pFileOp;
    FOR status;
    std::atomic<bool> is_trace;
    py::function trace_cb;

    std::string GetErrorMsg(HRESULT hr)
    {
        LPWSTR errorMessage = nullptr;
        std::string msg;
        DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS;
        DWORD dwError = HRESULT_CODE(hr);
        FormatMessageW(
            flags,
            nullptr,
            dwError,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&errorMessage,
            0,
            nullptr);
        if (errorMessage == nullptr)
        {
            msg = "";
            goto end;
        }
        msg = lpwstr2str(errorMessage);

    end:
        LocalFree(errorMessage);
        if (msg.empty())
        {
            return "Unknown Error";
        }
        return msg;
    }

    void pytrace(std::string level, std::string error_name = "", std::string target = "", std::string action = "", std::string msg = "")
    {
        if (is_trace.load(std::memory_order_acquire))
        {
            std::unordered_map<std::string, std::string> level_map = {
                {AMLEVEL, level},
                {AMERRORNAME, error_name},
                {AMTARGET, target},
                {AMACTION, action},
                {AMMESSAGE, msg},
            };
            {
                py::gil_scoped_acquire acquire;
                trace_cb(level_map);
            }
            // trace_cb(error_info);
        }
    }

    void SetPyTrace(py::object pycb = py::none())
    {
        if (pycb.is_none())
        {
            is_trace = false;
            trace_cb = py::function();
        }
        else
        {
            trace_cb = py::cast<py::function>(pycb);
            is_trace = true;
        }
    }

    ExplorerAPI(FileOperationSet set = FileOperationSet())
    {
        status = Init(set);
        if (status != FOR::SUCCESS)
        {
            pFileOp = nullptr;
        }
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
        if (settings.NoConfirmation)
            flags |= FOF_NOCONFIRMATION;
        if (settings.NoErrorUI)
            flags |= FOF_NOERRORUI;
        if (settings.NoConfirmationForMakeDir)
            flags |= FOF_NOCONFIRMMKDIR;
        if (settings.WarningIfPermanentDelete)
            flags |= FOF_WANTNUKEWARNING;
        if (settings.RenameOnCollision)
            flags |= FOF_RENAMEONCOLLISION;
        if (settings.AllowAdminPrivilege)
            flags |= FOFX_SHOWELEVATIONPROMPT;
        if (settings.AllowUndo)
            flags |= FOFX_ADDUNDORECORD;
        return flags;
    }

    FOR Config(FileOperationSet set)
    {
        if (pFileOp == nullptr)
        {
            return status;
        }
        DWORD flags = GetFlags(set);
        HRESULT hr = pFileOp->SetOperationFlags(flags);
        if (FAILED(hr))
        {
            pytrace(AMCRITICAL, "ConfigFailed", "ExplorerAPI", "SetOperationFlags", GetErrorMsg(hr));
            return FOR::FailToConfig;
        }
        return FOR::SUCCESS;
    }

    FOR Init(FileOperationSet set, py::object pycb = py::none())
    {
        SetPyTrace(pycb);
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

        if (FAILED(hr))
        {
            pytrace(AMCRITICAL, "InitFailed", "COMToolkit", "CoInitializeEx", GetErrorMsg(hr));
            return FOR::FailToInitCOM;
        }

        SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE); // 设置 DPI 感知

        hr = CoCreateInstance(
            CLSID_FileOperation, // CLSID_FileOperation 是 IFileOperation 的类标识符
            nullptr,
            CLSCTX_ALL,            // 在所有上下文中创建
            IID_PPV_ARGS(&pFileOp) // 获取 IFileOperation 接口
        );

        if (FAILED(hr))
        {
            pytrace(AMCRITICAL, "InstanceCreateFailed", "COMInstance", "CoCreateInstance", GetErrorMsg(hr));
            return FOR::FailToCreateIFileOperationInstance;
        }

        return Config(set);
    }

    TOR src2dst(
        const std::vector<std::wstring> &srcs,
        const std::wstring &dst,
        FileOperationType action)
    {
        if (pFileOp == nullptr)
        {
            return status;
        }
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
            pytrace(AMERROR, "DstShellItemCreateFailed", tostr(dst), "SHCreateItemFromParsingName", GetErrorMsg(hr));
            return FOR::FailToCreDstShellItem;
        }

        pFileOp->SetOperationFlags(FOF_NO_UI | FOF_ALLOWUNDO);

        for (const auto &src : srcs)
        {
            if (!std::filesystem::exists(src))
            {
                pytrace(AMWARNING, "SrcPathNotExists", tostr(src), "std::filesystem::exists", "Source path does not exist");
                results.emplace_back(std::pair(src, FOR::PathNotExists));
                continue;
            }

            wil::com_ptr<IShellItem> pSrcItem;
            hr = SHCreateItemFromParsingName(src.c_str(), nullptr, IID_PPV_ARGS(&pSrcItem));
            if (FAILED(hr))
            {
                pytrace(AMERROR, "SrcShellItemCreateFailed", tostr(src), "SHCreateItemFromParsingName", GetErrorMsg(hr));
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
            switch (action)
            {
            case FileOperationType::COPY:
                pytrace(AMERROR, "CopyOperationFailed", fmt::format("[]->{}", dst), "PerformOperations", GetErrorMsg(hr));
                break;
            case FileOperationType::MOVE:
                pytrace(AMERROR, "MoveOperationFailed", fmt::format("[]->{}", dst), "PerformOperations", GetErrorMsg(hr));
                break;
            }
            return FOR::FailToPerformOperation;
        }

        BOOL aborted = FALSE;
        pFileOp->GetAnyOperationsAborted(&aborted);
        if (aborted)
        {
            pytrace(AMINFO, "", fmt::format("[]->{}", dst), "PerformOperations", "Operation aborted");
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
        if (pFileOp == nullptr)
        {
            return status;
        }
        HRESULT hr;
        std::vector<std::pair<std::wstring, FOR>> result = {};
        for (auto path : paths)
        {
            if (!std::filesystem::exists(path))
            {
                result.emplace_back(std::pair(path, FOR::PathNotExists));
                continue;
            }

            wil::com_ptr<IShellItem> pItem;
            HRESULT hr = SHCreateItemFromParsingName(path.c_str(), NULL, IID_PPV_ARGS(&pItem));
            if (FAILED(hr))
            {
                result.emplace_back(std::pair(path, FOR::FailToCreSrcShellItem));
                continue;
            }
            hr = pFileOp->DeleteItem(pItem.get(), nullptr);
            result.emplace_back(std::pair(path, SUCCEEDED(hr) ? FOR::SUCCESS : FOR::FailToAddOperation));
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
        .def(py::init<bool, bool, bool, bool, bool, bool, bool, bool>(),
             py::arg("NoProgressUI") = false,
             py::arg("NoConfirmation") = false,
             py::arg("NoErrorUI") = false,
             py::arg("NoConfirmationForMakeDir") = true,
             py::arg("WarningIfPermanentDelete") = false,
             py::arg("RenameOnCollision") = false,
             py::arg("AllowAdminPrivilege") = true,
             py::arg("AllowUndo") = true)
        .def_readwrite("NoProgressUI", &FileOperationSet::NoProgressUI)
        .def_readwrite("NoConfirmation", &FileOperationSet::NoConfirmation)
        .def_readwrite("NoErrorUI", &FileOperationSet::NoErrorUI)
        .def_readwrite("NoConfirmationForMakeDir", &FileOperationSet::NoConfirmationForMakeDir)
        .def_readwrite("WarningIfPermanentDelete", &FileOperationSet::WarningIfPermanentDelete)
        .def_readwrite("RenameOnCollision", &FileOperationSet::RenameOnCollision)
        .def_readwrite("AllowAdminPrivilege", &FileOperationSet::AllowAdminPrivilege)
        .def_readwrite("AllowUndo", &FileOperationSet::AllowUndo);
    py::enum_<FileOperationResult>(m, "FileOperationResult")
        .value("SUCCESS", FileOperationResult::SUCCESS)
        .value("PathNotExists", FileOperationResult::PathNotExists)
        .value("FaileToCreOperationInstance", FileOperationResult::FaileToCreOperationInstance)
        .value("FailToSetOperationFlags", FileOperationResult::FailToSetOperationFlags)
        .value("FailToCreSrcShellItem", FileOperationResult::FailToCreSrcShellItem)
        .value("FailToCreDstShellItem", FileOperationResult::FailToCreDstShellItem)
        .value("FailToAddOperation", FileOperationResult::FailToAddOperation)
        .value("FailToPerformOperation", FileOperationResult::FailToPerformOperation)
        .value("WrongOperationType", FileOperationResult::WrongOperationType)
        .value("FailToConfig", FileOperationResult::FailToConfig)
        .value("UpperDirNotExists", FileOperationResult::UpperDirNotExists)
        .value("DstIsNotDir", FileOperationResult::DstIsNotDir)
        .value("DstIsNotExists", FileOperationResult::DstIsNotExists)
        .value("UnknownError", FileOperationResult::UnknownError)
        .value("FailToInitCOM", FileOperationResult::FailToInitCOM)
        .value("NoIFileOperationInstance", FileOperationResult::NoIFileOperationInstance)
        .value("FailToCreateIFileOperationInstance", FileOperationResult::FailToCreateIFileOperationInstance)
        .value("OperationAborted", FileOperationResult::OperationAborted);
    py::class_<ExplorerAPI>(m, "ExplorerAPI")
        .def(py::init<FileOperationSet>(), py::arg("set") = FileOperationSet())
        .def("remove", &ExplorerAPI::remove, py::arg("path"))
        .def("copy", &ExplorerAPI::copy, py::arg("src"), py::arg("dst"))
        .def("move", &ExplorerAPI::move, py::arg("src"), py::arg("dst"))
        .def("config", &ExplorerAPI::Config, py::arg("set"));
}
