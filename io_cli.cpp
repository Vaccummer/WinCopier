#include <AMPath.hpp>
#include <argparse/argparse.hpp>
#include <filesystem>
#include <fmt/core.h>
#include <fmt/format.h>
#include <iostream>
#include <magic_enum/magic_enum.hpp>
#include <map>
#include <regex>
#include <shellscalingapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <string>
#include <wil/com.h>
#include <windows.h>
#pragma comment(lib, "shell32.lib");
#pragma comment(lib, "ole32");
#pragma comment(lib, "oleaut32");
#pragma comment(lib, "uuid");
#pragma comment(lib, "shcore");
#pragma comment(lib, "fmt");
#pragma comment(lib, "Advapi32");

constexpr char *AMLEVEL = "LEVEL";
constexpr char *AMERRORCODE = "ERRORCODE";
constexpr char *AMERRORNAME = "ERRORNAME";
constexpr char *AMTARGET = "TARGET";
constexpr char *AMACTION = "ACTION";
constexpr char *AMMESSAGE = "MESSAGE";

constexpr char *AMCRITICAL = "CRITICAL";
constexpr char *AMERROR = "ERROR";
constexpr char *AMWARNING = "WARNING";
constexpr char *AMDEBUG = "DEBUG";
constexpr char *AMINFO = "INFO";

constexpr int AMPYTRACEERROR = -198;
namespace fs = std::filesystem;

enum class FileOperationType
{
    COPY = 1,
    MOVE = 2,
    REMOVE = 3,
    RENAME = 4,
};

enum class FileOperationResult
{
    SUCCESS = 0,
    DstAlreadyExists = -1,
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
    PyTraceError = -21,
    FailToCreateDir = -22,
    InvalidArgument = -23,
};

enum class FileOperationStatus
{
    Perfect = 0,
    PartialSuccess = 1,
    NoOperation = 2,
    FinalError = -1,
    AllErrors = -2,
    Uninitialized = -3,
};

std::string GetECName(FileOperationResult error_code)
{
    return std::string(magic_enum::enum_name(error_code));
}

struct FileOperationSet
{
    bool NoProgressUI;
    bool AlwaysYes;
    bool NoErrorUI;
    bool NoMkdirInfo;
    bool DeleteWarning;
    bool RenameOnCollision;
    bool AllowAdmin;
    bool AllowUndo;
    bool Hardlink;
    bool ToRecycleBin;
    FileOperationSet()
        : NoProgressUI(false),
          AlwaysYes(false),
          NoErrorUI(false),
          NoMkdirInfo(true),
          DeleteWarning(false),
          RenameOnCollision(false),
          AllowAdmin(true),
          AllowUndo(true),
          Hardlink(false),
          ToRecycleBin(false)
    {
    }

    FileOperationSet(
        bool DeleteWarning,
        bool NoProgressUI = false,
        bool AlwaysYes = false,
        bool NoErrorUI = false,
        bool NoMkdirInfo = true,

        bool RenameOnCollision = false,
        bool AllowAdmin = true,
        bool AllowUndo = true,
        bool Hardlink = false,
        bool ToRecycleBin = false)
        : NoProgressUI(NoProgressUI),
          AlwaysYes(AlwaysYes),
          NoErrorUI(NoErrorUI),
          NoMkdirInfo(NoMkdirInfo),
          DeleteWarning(DeleteWarning),
          RenameOnCollision(RenameOnCollision),
          AllowAdmin(AllowAdmin),
          AllowUndo(AllowUndo),
          Hardlink(Hardlink),
          ToRecycleBin(ToRecycleBin)
    {
    }
};

bool is_absolute(const std::string &path)
{
    std::regex regex1("^[A-Za-z]:[/\\\\]");
    std::regex regex2("^/");
    std::regex regex3("^\\\\\\\\");
    return std::regex_match(path, regex1) || std::regex_match(path, regex2) || std::regex_match(path, regex3);
}

template <typename... Args>
fs::path join(Args &&...args)
{
    std::vector<std::string> segments;
    fs::path combined;

    auto process_arg = [&](auto &&arg)
    {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::filesystem::path>)
        {
            segments.push_back(arg.string());
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            std::string s = std::forward<decltype(arg)>(arg);
            if (s.empty())
            {
                return;
            }
            segments.push_back(s);
        }
        else if constexpr (std::is_same_v<T, std::vector<std::string>>)
        {
            segments.insert(segments.end(), arg.begin(), arg.end());
        }
    };

    (process_arg(std::forward<Args>(args)), ...);

    if (segments.empty())
        return "";

    for (auto &seg : segments)
    {
        if (combined.empty())
        {
            combined = seg;
        }
        else
        {
            combined /= seg;
        }
    }
    return combined.lexically_normal();
}

std::vector<std::string> split(const std::string &path)
{
    std::vector<std::string> segments;
    fs::path p(path);
    for (const auto &seg : p)
    {
        segments.push_back(seg.string());
    }
    return segments;
}

std::string realpath(const std::string &path, bool force_absolute = false)
{
    std::string cwd = fs::current_path().string();
    std::vector<std::string> parts = split(path);
    if (parts[0] == "." || parts[0] == "~")
    {
        parts.erase(parts.begin());
        std::vector<std::string> parts_t = split(cwd);
        parts.insert(parts.begin(), parts_t.begin(), parts_t.end());
    }
    else if (!is_absolute(path))
    {
        std::vector<std::string> parts_t = split(cwd);
        parts.insert(parts.begin(), parts_t.begin(), parts_t.end());
    }
    std::vector<std::string> new_parts;
    for (const auto &part : parts)
    {
        if (part == ".")
        {
            continue;
        }
        else if (part == "..")
        {
            if (!new_parts.empty())
            {
                new_parts.pop_back();
            }
        }
        else
        {
            new_parts.push_back(part);
        }
    }
    return join(new_parts).string();
}

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

std::wstring ansi_to_wstring(const std::string &str)
{
    int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
    if (len <= 0)
        return L"";
    std::wstring result(len - 1, 0); // 去掉结尾的null字符
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &result[0], len);
    return result;
}

std::string wstring_to_ansi(const std::wstring &wstr)
{
    int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0)
        return "";
    std::string result(len - 1, 0);
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &result[0], len, nullptr, nullptr);
    return result;
}

std::wstring utf8_to_wstring(const std::string &str)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (len <= 0)
        return L"";
    std::wstring result(len - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], len);
    return result;
}

std::string wstring_to_utf8(const std::wstring &wstr)
{
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0)
        return "";
    std::string result(len - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], len, nullptr, nullptr);
    return result;
}

std::string wstr2str(const std::wstring &wstr)
{
    int code_page = GetACP();
    if (code_page == CP_UTF8)
    {
        return wstring_to_utf8(wstr);
    }
    else
    {
        return wstring_to_ansi(wstr);
    }
}

bool is_valid_utf8(const std::string &str)
{
    int remaining = 0;
    for (unsigned char c : str)
    {
        if (remaining > 0)
        {
            if ((c & 0xC0) != 0x80)
                return false;
            --remaining;
        }
        else
        {
            if ((c & 0x80) == 0x00)
                continue; // 单字节 0xxxxxxx
            else if ((c & 0xE0) == 0xC0)
                remaining = 1; // 双字节 110xxxxx
            else if ((c & 0xF0) == 0xE0)
                remaining = 2; // 三字节 1110xxxx
            else if ((c & 0xF8) == 0xF0)
                remaining = 3; // 四字节 11110xxx
            else
                return false;
        }
    }
    return (remaining == 0);
}

std::wstring str2wstr(const std::string &str)
{
    std::vector<std::string> parts = split(str);
    std::string new_path;
    if (parts.size() == 1)
    {
        new_path = parts[0];
    }
    else
    {
        new_path = join(parts).string();
    }
    if (is_valid_utf8(new_path))
    {
        return utf8_to_wstring(new_path);
    }
    return ansi_to_wstring(new_path);
}

std::wstring WinPathFormat(const std::string &path)
{
    std::string new_str = realpath(path);
    std::wstring wstr = str2wstr(new_str);
    std::replace(wstr.begin(), wstr.end(), L'/', L'\\');
    return wstr;
}

std::string lpwstr2str(LPWSTR lpwstr)
{
    if (lpwstr == nullptr)
        return "";
    std::wstring wstr(lpwstr);
    return wstr2str(wstr);
}

using FOR = FileOperationResult;
using ECM = std::pair<FOR, std::string>;
using PECM = std::pair<std::string, ECM>;
using TOR = std::pair<FileOperationStatus, std::vector<PECM>>;
using sptr = std::shared_ptr<FileOperationSet>;

struct SingleFileOperation
{
    FileOperationType action;
    std::string src;
    std::string dst_dir;
    std::string dst_name;
    bool mkdir;
    SingleFileOperation(FileOperationType action, std::string src, std::string dst_dir, std::string dst_name, bool mkdir)
        : action(action), src(src), dst_dir(dst_dir), dst_name(dst_name), mkdir(mkdir)
    {
    }
};

class ExplorerAPI
{
private:
    IFileOperation *pFileOp;
    FOR status;
    std::string g_error_msg = "";
    FileOperationSet settings;

    bool IsFileNameValid(const std::string &name)
    {
        std::regex illegal_chars("[\\/:*?\"<>|]");
        return !std::regex_search(name, illegal_chars);
    }

    void trace(std::string level, FOR error_code, std::string target, std::string action, std::string message)
    {
    }

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

    DWORD GetFlags(FileOperationSet settings)
    {
        DWORD flags = 0;
        if (settings.NoProgressUI)
            flags |= FOF_SILENT;
        if (settings.AlwaysYes)
            flags |= FOF_NOCONFIRMATION;
        if (settings.NoErrorUI)
            flags |= FOF_NOERRORUI;
        if (settings.NoMkdirInfo)
            flags |= FOF_NOCONFIRMMKDIR;
        if (settings.DeleteWarning)
            flags |= FOF_WANTNUKEWARNING;
        if (settings.RenameOnCollision)
            flags |= FOF_RENAMEONCOLLISION;
        if (settings.AllowAdmin)
            flags |= FOFX_SHOWELEVATIONPROMPT;
        if (settings.AllowUndo)
            flags |= FOFX_ADDUNDORECORD;
        if (settings.Hardlink)
            flags |= FOFX_PREFERHARDLINK;
        if (settings.ToRecycleBin)
            flags |= FOFX_RECYCLEONDELETE;
        return flags;
    }

    ECM Base1OP(FileOperationType action, std::string src, std::string dst_dir, std::string dst_name, bool mkdir, sptr tmp_set = nullptr)
    {
        ECM ecm = PendOperation(src, dst_dir, dst_name, action, mkdir);
        if (ecm.first != FOR::SUCCESS)
        {
            return ecm;
        }
        HRESULT hr;

        if (tmp_set)
        {
            FileOperationSet ori_set = settings;
            Config(*tmp_set);
            hr = pFileOp->PerformOperations();
            Config(ori_set);
        }
        else
        {
            hr = pFileOp->PerformOperations();
        }
        if (FAILED(hr))
        {
            return ECM(FOR::FailToPerformOperation, GetErrorMsg(hr));
        }
        return ECM(FOR::SUCCESS, "");
    }

    TOR BaseMultiOP(std::vector<SingleFileOperation> &operations, sptr tmp_set = nullptr)
    {
        if (!pFileOp)
        {
            return {FileOperationStatus::Uninitialized, {PECM("", ECM(FOR::NoIFileOperationInstance, "No IFileOperation instance"))}};
        }
        if (operations.empty())
        {
            return {FileOperationStatus::NoOperation, {PECM("", ECM(FOR::InvalidArgument, "No operations"))}};
        }
        std::vector<PECM> results;
        bool no_task = true;
        for (auto operation : operations)
        {
            ECM ecm = PendOperation(operation);
            if (ecm.first != FOR::SUCCESS)
            {
                results.emplace_back(PECM(operation.src, ecm));
            }
            no_task = false;
        }
        if (no_task)
        {
            return {FileOperationStatus::AllErrors, results};
        }
        HRESULT hr;
        if (tmp_set)
        {
            FileOperationSet ori_set = settings;
            Config(*tmp_set);
            hr = pFileOp->PerformOperations();
            Config(ori_set);
        }
        else
        {
            hr = pFileOp->PerformOperations();
        }
        if (FAILED(hr))
        {
            this->trace(AMERROR, FOR::FailToPerformOperation, "ExplorerAPI", "PerformOperations", GetErrorMsg(hr));
            results.emplace_back(PECM("", ECM(FOR::FailToPerformOperation, GetErrorMsg(hr))));
            return {FileOperationStatus::FinalError, results};
        }
        return {results.empty() ? FileOperationStatus::Perfect : FileOperationStatus::PartialSuccess, results};
    }

public:
    ExplorerAPI(FileOperationSet set = FileOperationSet(), unsigned int buffer_capacity = 10)
    {
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (FAILED(hr))
        {
            status = FOR::FailToInitCOM;
            return;
        }
        Init(set);
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

    FileOperationSet GetSettings()
    {
        return settings;
    }

    ECM Config(FileOperationSet set)
    {
        if (pFileOp == nullptr)
        {
            return ECM(status, g_error_msg);
        }
        DWORD flags = GetFlags(set);
        HRESULT hr = pFileOp->SetOperationFlags(flags);
        if (FAILED(hr))
        {
            status = FOR::FailToConfig;
            g_error_msg = fmt::format("Failed to set operation flags: {}", GetErrorMsg(hr));
            this->trace(AMCRITICAL, FOR::FailToConfig, "ExplorerAPI", "SetOperationFlags", g_error_msg);
            return ECM(FOR::FailToConfig, g_error_msg);
        }
        this->settings = set;
        status = FOR::SUCCESS;
        g_error_msg = "";
        return ECM(FOR::SUCCESS, "");
    }

    ECM Init(FileOperationSet set)
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
            g_error_msg = fmt::format("Failed to create IFileOperation instance: {}", GetErrorMsg(hr));
            this->trace(AMCRITICAL, FOR::FailToCreateIFileOperationInstance, "COMInstance", "CoCreateInstance", g_error_msg);
            return ECM(FOR::FailToCreateIFileOperationInstance, g_error_msg);
        }
        if (!pFileOp)
        {
            g_error_msg = "Failed to create IFileOperation instance";
            this->trace(AMCRITICAL, FOR::NoIFileOperationInstance, "COMInstance", "CoCreateInstance", g_error_msg);
            return ECM(FOR::NoIFileOperationInstance, g_error_msg);
        }
        status = FOR::SUCCESS;
        g_error_msg = "";

        return Config(set);
    }

    ECM PendOperation(SingleFileOperation &operation)
    {
        return PendOperation(operation.src, operation.dst_dir, operation.dst_name, operation.action, operation.mkdir);
    }

    ECM PendOperation(std::string src, std::string dst_dir, std::string dst_name, FileOperationType action, bool mkdir)
    {
        std::string srcf = realpath(src);
        std::string dstf;
        if (!std::filesystem::exists(srcf))
        {
            this->trace(AMWARNING, FOR::PathNotExists, src, "CheckArguments", "Source path does not exist");
            return ECM(FOR::PathNotExists, "Source path does not exist");
        }

        if (dst_dir.empty())
        {
            if (action != FileOperationType::REMOVE)
            {
                this->trace(AMWARNING, FOR::InvalidArgument, src, "CheckArguments", "Destination name is empty and operation is not remove");
                return ECM(FOR::InvalidArgument, "Destination name is required");
            }
        }
        else
        {
            dstf = realpath(dst_dir);
            if (!std::filesystem::exists(dstf))
            {
                if (!mkdir)
                {
                    this->trace(AMWARNING, FOR::PathNotExists, dst_dir, "CheckArguments", "Destination path does not exist");
                    return ECM(FOR::PathNotExists, "Destination path does not exist");
                }
                else
                {
                    try
                    {
                        std::filesystem::create_directories(dstf);
                    }
                    catch (const std::filesystem::filesystem_error &e)
                    {
                        return ECM(FOR::FailToCreateDir, e.what());
                    }
                }
            }
            else if (!std::filesystem::is_directory(dstf))
            {
                return ECM(FOR::DstIsNotDir, "Destination path is not a directory");
            }
        }
        std::wstring src_wstr;
        std::wstring dst_dir_wstr;
        std::string msg;
        HRESULT hr;
        if (action == FileOperationType::REMOVE)
        {
            src_wstr = WinPathFormat(srcf);
            wil::com_ptr<IShellItem> pItem;
            HRESULT hr = SHCreateItemFromParsingName(src_wstr.c_str(), nullptr, IID_PPV_ARGS(&pItem));
            if (FAILED(hr))
            {
                msg = fmt::format("Failed to create source shell item: {}", GetErrorMsg(hr));
                this->trace(AMERROR, FOR::FailToCreSrcShellItem, src, "PerformOperations", msg);
                return ECM(FOR::FailToCreSrcShellItem, msg);
            }
            hr = pFileOp->DeleteItem(pItem.get(), nullptr);
            if (FAILED(hr))
            {
                msg = fmt::format("Failed to add operation: {}", GetErrorMsg(hr));
                this->trace(AMERROR, FOR::FailToAddOperation, src, "AddOperation", msg);
                return ECM(FOR::FailToAddOperation, msg);
            }
            return ECM(FOR::SUCCESS, "");
        }
        else
        {
            src_wstr = WinPathFormat(srcf);
            dst_dir_wstr = WinPathFormat(dstf);
            if (!IsFileNameValid(dst_name))
            {
                this->trace(AMWARNING, FOR::InvalidArgument, src, "CheckArguments", "Destination name contains invalid characters");
                return ECM(FOR::InvalidArgument, "Destination name contains invalid characters");
            }
            if (dst_name.empty() && action == FileOperationType::RENAME)
            {
                this->trace(AMWARNING, FOR::InvalidArgument, src, "CheckArguments", "Destination name is empty and operation is rename");
                return ECM(FOR::InvalidArgument, "Destination name is required in Rename operation");
            }
            std::wstring dst_name_wstr = dst_name.empty() ? L"" : str2wstr(dst_name);
            wil::com_ptr<IShellItem> pSrcItem;
            hr = SHCreateItemFromParsingName(src_wstr.c_str(), nullptr, IID_PPV_ARGS(&pSrcItem));
            if (FAILED(hr))
            {
                msg = fmt::format("Failed to create source shell item: {}", GetErrorMsg(hr));
                this->trace(AMERROR, FOR::FailToCreSrcShellItem, src, "PerformOperations", msg);
                return ECM(FOR::FailToCreSrcShellItem, msg);
            }
            wil::com_ptr<IShellItem> pDstItem;
            hr = SHCreateItemFromParsingName(dst_dir_wstr.c_str(), nullptr, IID_PPV_ARGS(&pDstItem));
            if (FAILED(hr))
            {
                msg = fmt::format("Failed to create destination shell item: {}", GetErrorMsg(hr));
                this->trace(AMERROR, FOR::FailToCreDstShellItem, dst_dir, "PerformOperations", msg);
                return ECM(FOR::FailToCreDstShellItem, msg);
            }

            if (action == FileOperationType::MOVE)
            {
                hr = pFileOp->MoveItem(pSrcItem.get(), pDstItem.get(), dst_name_wstr.c_str(), nullptr);
                if (FAILED(hr))
                {
                    msg = fmt::format("Failed to add Move operation: {}", GetErrorMsg(hr));
                    this->trace(AMERROR, FOR::FailToAddOperation, src, "AddOperation", msg);
                    return ECM(FOR::FailToAddOperation, msg);
                }
            }
            else if (action == FileOperationType::COPY)
            {
                hr = pFileOp->CopyItem(pSrcItem.get(), pDstItem.get(), dst_name_wstr.c_str(), nullptr);
                if (FAILED(hr))
                {
                    msg = fmt::format("Failed to add Copy operation: {}", GetErrorMsg(hr));
                    this->trace(AMERROR, FOR::FailToAddOperation, src, "AddOperation", msg);
                    return ECM(FOR::FailToAddOperation, msg);
                }
            }
            else if (action == FileOperationType::RENAME)
            {
                hr = pFileOp->RenameItem(pSrcItem.get(), dst_name_wstr.c_str(), nullptr);
                if (FAILED(hr))
                {
                    msg = fmt::format("Failed to add Rename operation: {}", GetErrorMsg(hr));
                    this->trace(AMERROR, FOR::FailToAddOperation, src, "AddOperation", msg);
                    return ECM(FOR::FailToAddOperation, msg);
                }
            }

            return ECM(FOR::SUCCESS, "");
        }
    }

    ECM Copy(std::string src, std::string dst_dir, bool mkdir = true, sptr tmp_set = nullptr)
    {
        return Base1OP(FileOperationType::COPY, src, dst_dir, "", mkdir, tmp_set);
    }

    TOR Copy(std::vector<std::string> &srcs, std::string dst, bool mkdir = true, sptr tmp_set = nullptr)
    {
        std::vector<SingleFileOperation> operations;
        for (auto src : srcs)
        {
            operations.emplace_back(SingleFileOperation(FileOperationType::COPY, src, dst, "", mkdir));
        }
        return BaseMultiOP(operations);
    }

    ECM Clone(std::string src, std::string dst, bool mkdir = true, sptr tmp_set = nullptr)
    {
        dst = realpath(dst);
        std::string dst_dir = fs::path(dst).parent_path().string();
        std::string dst_name = fs::path(dst).filename().string();
        return Base1OP(FileOperationType::COPY, src, dst_dir, dst_name, mkdir, tmp_set);
    }

    TOR Clone(std::map<std::string, std::string> &srcs_dst, bool mkdir = true, sptr tmp_set = nullptr)
    {
        std::vector<SingleFileOperation> operations;
        std::string src_path;
        std::string dst_dir;
        std::string dst_name;
        for (auto [src, dst] : srcs_dst)
        {
            src_path = realpath(src);
            dst_dir = fs::path(dst).parent_path().string();
            dst_name = fs::path(dst).filename().string();
            operations.emplace_back(SingleFileOperation(FileOperationType::COPY, src_path, dst_dir, dst_name, mkdir));
        }
        return BaseMultiOP(operations);
    }

    ECM Move(std::string src, std::string dst_dir, bool mkdir = true, sptr tmp_set = nullptr)
    {
        return Base1OP(FileOperationType::MOVE, src, dst_dir, "", mkdir, tmp_set);
    }

    TOR Move(std::vector<std::string> &srcs, std::string dst, bool mkdir = true, sptr tmp_set = nullptr)
    {
        std::vector<SingleFileOperation> operations;
        for (auto src : srcs)
        {
            operations.emplace_back(SingleFileOperation(FileOperationType::MOVE, src, dst, "", mkdir));
        }
        return BaseMultiOP(operations);
    }

    ECM Remove(std::string path, sptr tmp_set = nullptr)
    {
        return Base1OP(FileOperationType::REMOVE, path, "", "", false, tmp_set);
    }

    TOR Remove(std::vector<std::string> &paths, sptr tmp_set = nullptr)
    {
        std::vector<SingleFileOperation> operations;
        for (auto path : paths)
        {
            operations.emplace_back(SingleFileOperation(FileOperationType::REMOVE, path, "", "", false));
        }
        return BaseMultiOP(operations);
    }

    ECM Rename(std::string src, std::string new_name, sptr tmp_set = nullptr)
    {
        return Base1OP(FileOperationType::RENAME, src, src, new_name, false, tmp_set);
    }

    TOR Rename(std::map<std::string, std::string> &srcs_new_names, sptr tmp_set = nullptr)
    {
        std::vector<SingleFileOperation> operations;
        for (auto [src, new_name] : srcs_new_names)
        {
            operations.emplace_back(SingleFileOperation(FileOperationType::RENAME, src, src, new_name, false));
        }
        return BaseMultiOP(operations);
    }

    ECM Replace(std::string src, std::string dst, sptr tmp_set = nullptr)
    {
        std::string dst_dir = fs::path(realpath(dst)).parent_path().string();
        std::string dst_name = fs::path(realpath(dst)).filename().string();
        return Base1OP(FileOperationType::MOVE, src, dst_dir, dst_name, true, tmp_set);
    }

    TOR Replace(std::map<std::string, std::string> &srcs_dsts, sptr tmp_set = nullptr)
    {
        std::vector<SingleFileOperation> operations;
        for (auto [src, dst] : srcs_dsts)
        {
            std::string dst_dir = fs::path(realpath(dst)).parent_path().string();
            std::string dst_name = fs::path(realpath(dst)).filename().string();
            operations.emplace_back(SingleFileOperation(FileOperationType::MOVE, src, dst_dir, dst_name, true));
        }
        return BaseMultiOP(operations);
    }

    ECM Conduct(FileOperationType action, std::string src, std::string dst_dir = "", std::string dst_name = "", bool mkdir = false, sptr tmp_set = nullptr)
    {
        return Base1OP(action, src, dst_dir, dst_name, mkdir, tmp_set);
    }

    ECM Conduct(SingleFileOperation &operation, sptr tmp_set = nullptr)
    {
        return Base1OP(operation.action, operation.src, operation.dst_dir, operation.dst_name, operation.mkdir, tmp_set);
    }

    TOR Conduct(std::vector<SingleFileOperation> &operations, sptr tmp_set = nullptr)
    {
        return BaseMultiOP(operations, tmp_set);
    }
};

int main()
{
    auto res = AMPath::find("D:\\CodeLib\\**\\amsftp\\", AMPathTools::ENUMS::SearchType::File, true);
    if (std::holds_alternative<std::vector<std::string>>(res))
    {
        for (auto &path : std::get<std::vector<std::string>>(res))
        {
            std::cout << path << std::endl;
        }
    }
    else if (std::holds_alternative<std::pair<std::string, std::string>>(res))
    {
        auto [path, error] = std::get<std::pair<std::string, std::string>>(res);
        std::cout << path << ": " << error << std::endl;
    }
    // auto res = AMPath::split("\\\\wsl.localhost\\Ubuntu-22.04\\mnt\\m");
    // std::cout << "res.size(): " << res.size() << std::endl;
    // for (auto &path : res)
    // {
    //     std::cout << path << std::endl;
    // }
    // std::cout << AMPath::join("C:/", "Users\\", "/Adm\\/i\\n\\yes/ok", "\\/a\\//") << std::endl;
    // for (auto &entry : fs::directory_iterator("D:"))
    // {
    //     std::cout << entry.path().string() << std::endl;
    // }
    return 0;
}
