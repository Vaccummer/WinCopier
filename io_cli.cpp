#include "AMTools.hpp"
#include <AMPath.hpp>
#include <CLI11.hpp>
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
          ToRecycleBin(true)
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
        msg = AMPathTools::AMstr(errorMessage);

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
        ECM ecm = PendOperation(action, src, dst_dir, dst_name, mkdir);
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
    ExplorerAPI()
    {
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
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (FAILED(hr))
        {
            g_error_msg = fmt::format("Failed to create IFileOperation instance: {}", GetErrorMsg(hr));
            this->trace(AMCRITICAL, FOR::FailToCreateIFileOperationInstance, "COMInstance", "CoCreateInstance", g_error_msg);
            return {FOR::FailToInitCOM, g_error_msg};
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
        return PendOperation(operation.action, operation.src, operation.dst_dir, operation.dst_name, operation.mkdir);
    }

    ECM PendOperation(FileOperationType action, std::string src, std::string dst_dir, std::string dst_name, bool mkdir)
    {
        std::string srcf = AMPath::realpath(src, false, "\\");
        std::string dstf;
        if (!std::filesystem::exists(srcf))
        {
            this->trace(AMWARNING, FOR::PathNotExists, src, "CheckArguments", "Source path does not exist");
            return ECM(FOR::PathNotExists, "Source path does not exist");
        }

        if (dst_dir.empty())
        {
            if (action != FileOperationType::REMOVE && action != FileOperationType::RENAME)
            {
                this->trace(AMWARNING, FOR::InvalidArgument, src, "CheckArguments", "Destination name is empty and operation is not remove");
                return ECM(FOR::InvalidArgument, "Destination name is required");
            }
        }
        else
        {
            dstf = AMPath::realpath(dst_dir, false, "\\");
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
        if (action == FileOperationType::REMOVE || action == FileOperationType::RENAME)
        {
            src_wstr = AMPathTools::AMstr(srcf);
            wil::com_ptr<IShellItem> pItem;
            HRESULT hr = SHCreateItemFromParsingName(src_wstr.c_str(), nullptr, IID_PPV_ARGS(&pItem));
            if (FAILED(hr))
            {
                msg = fmt::format("Failed to create source shell item: {}", GetErrorMsg(hr));
                this->trace(AMERROR, FOR::FailToCreSrcShellItem, src, "PerformOperations", msg);
                return ECM(FOR::FailToCreSrcShellItem, msg);
            }
            if (action == FileOperationType::REMOVE)
            {
                hr = pFileOp->DeleteItem(pItem.get(), nullptr);
                if (FAILED(hr))
                {
                    msg = fmt::format("Failed to add operation: {}", GetErrorMsg(hr));
                    this->trace(AMERROR, FOR::FailToAddOperation, src, "AddOperation", msg);
                    return ECM(FOR::FailToAddOperation, msg);
                }
                return ECM(FOR::SUCCESS, "");
            }
            else if (action == FileOperationType::RENAME)
            {
                if (!IsFileNameValid(dst_name))
                {
                    this->trace(AMWARNING, FOR::InvalidArgument, src, "CheckArguments", "Destination name contains invalid characters");
                    return ECM(FOR::InvalidArgument, "Destination name contains invalid characters");
                }
                if (dst_name.empty())
                {
                    this->trace(AMWARNING, FOR::InvalidArgument, src, "CheckArguments", "Destination name is empty");
                    return ECM(FOR::InvalidArgument, "Destination name is empty");
                }
                std::wstring dst_name_wstr = AMPathTools::AMstr(dst_name);
                hr = pFileOp->RenameItem(pItem.get(), dst_name_wstr.c_str(), nullptr);
                if (FAILED(hr))
                {
                    msg = fmt::format("Failed to add Rename operation: {}", GetErrorMsg(hr));
                    this->trace(AMERROR, FOR::FailToAddOperation, src, "AddOperation", msg);
                    return ECM(FOR::FailToAddOperation, msg);
                }
                return ECM(FOR::SUCCESS, "");
            }
        }
        else
        {
            src_wstr = AMPathTools::AMstr(srcf);
            dst_dir_wstr = AMPathTools::AMstr(dstf);
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
            std::wstring dst_name_wstr = dst_name.empty() ? L"" : AMPathTools::AMstr(dst_name);
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
        dst = AMPath::realpath(dst, false, "\\");
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
            src_path = AMPath::realpath(src, false, "\\");
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
        std::string dst_dir = fs::path(AMPath::realpath(dst, false, "\\")).parent_path().string();
        std::string dst_name = fs::path(AMPath::realpath(dst, false, "\\")).filename().string();
        return Base1OP(FileOperationType::MOVE, src, dst_dir, dst_name, true, tmp_set);
    }

    TOR Replace(std::map<std::string, std::string> &srcs_dsts, sptr tmp_set = nullptr)
    {
        std::vector<SingleFileOperation> operations;
        for (auto [src, dst] : srcs_dsts)
        {
            std::string dst_dir = fs::path(AMPath::realpath(dst, false, "\\")).parent_path().string();
            std::string dst_name = fs::path(AMPath::realpath(dst, false, "\\")).filename().string();
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

    ECM Conduct(sptr tmp_set = nullptr)
    {
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
        std::cout << "hr: " << hr << std::endl;
        if (FAILED(hr))
        {
            hr = hr & 0xFFFFFFFF;
            amprint(GetErrorMsg(hr));

            if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED) || hr == COPYENGINE_E_USER_CANCELLED)
            {
                amprint("OperationAborted");
                return ECM(FOR::OperationAborted, "");
            }
            return ECM(FOR::FailToPerformOperation, GetErrorMsg(hr));
        }
        return ECM(FOR::SUCCESS, "");
    }
};

namespace CliPara
{
    const std::unordered_map<std::string, std::vector<char>> AvailableFuntions =
        {{"cp", {'r', 'm', 'f', 'q'}},
         {"cl", {'f', 'm', 'q'}},
         {"mv", {'r', 'm', 'q'}},
         {"rp", {'f', 'm', 'q'}},
         {"rn", {'f', 'q'}},
         {"rm", {'p', 'r', 'q'}},
         {"new", {'m', 'f', 'q'}}};

    // std::pair<std::string, std::vector<char>> ParsingArg(int argc, char **argv)
    // {
    //     std::string func = "";
    //     std::vector<char> options{};
    //     std::vector<std::string> position_args{};
    // }

    struct Options
    {
        bool only_file;
        bool only_dir;
        bool force;
        bool quiet;
        bool regex;
        bool mkdir;
        bool permanent;
        bool newname;
        AMPathTools::ENUMS::SearchType srh;
        FileOperationSet set;
        std::shared_ptr<std::function<void(std::string, std::string, std::string, std::string)>> cb_ptr;
        Options(bool only_file, bool only_dir, bool force, bool quiet, bool regex, bool mkdir, bool permanent, bool newname) : only_file(only_file), only_dir(only_dir), force(force), quiet(quiet), regex(regex), mkdir(mkdir), permanent(permanent), newname(newname)
        {
            if (quiet)
            {
                this->set.NoErrorUI = true;
                this->set.NoProgressUI = true;
                this->set.DeleteWarning = false;
            }
            if (permanent)
            {
                this->set.ToRecycleBin = false;
            }
            if (newname)
            {
                this->set.RenameOnCollision = true;
            }
            if (force)
            {
                this->set.AlwaysYes = true;
            }

            if (only_file && !only_dir)
            {
                this->srh = AMPathTools::ENUMS::SearchType::File;
            }
            else if (!only_file && only_dir)
            {
                this->srh = AMPathTools::ENUMS::SearchType::Directory;
            }
            else
            {
                this->srh = AMPathTools::ENUMS::SearchType::All;
            }
        }

        void MsgRecord(std::string src, std::string error_name, std::string error_msg) {}
    };

    enum class FuntionType
    {
        Unknown = 0,
        COPY = 1,
        CLONE = 2,
        MOVE = 3,
        REPLACE = 4,
        REMOVE = 5,
        NEW = 6,
        RENAME = 7,
    };
}

namespace CliFunc
{
    void OriCallback(std::string src, std::string error, std::string msg) {}

    void CopyMove(FileOperationType oper, std::vector<std::string> &paths, CliPara::Options &opt, std::vector<SingleFileOperation> &tasks, std::shared_ptr<std::function<void(std::string, std::string, std::string)>> cb)
    {
        std::vector<std::string> srcs;
        std::string dst;
        if (paths.size() == 0)
        {
            return;
        }
        else if (paths.size() == 1)
        {
            srcs = paths;
            dst = std::filesystem::current_path().string();
        }
        else
        {
            dst = paths.back();
            srcs = paths;
            srcs.pop_back();
        }

        for (auto src : srcs)
        {
            for (auto path : AMPath::find(src, opt.srh, opt.regex, opt.quiet, cb))
            {
                amprint("path_matched: ", path);
                tasks.emplace_back(SingleFileOperation(oper, src, dst, "", opt.mkdir));
            }
        }
    }

    void Remove(FileOperationType oper, std::vector<std::string> &paths, CliPara::Options &opt, std::vector<SingleFileOperation> &tasks, std::shared_ptr<std::function<void(std::string, std::string, std::string)>> cb)
    {
        for (auto src : paths)
        {
            for (auto path : AMPath::find(src, opt.srh, opt.regex, opt.quiet, cb))
            {
                tasks.emplace_back(SingleFileOperation(FileOperationType::REMOVE, src, "", "", opt.mkdir));
            }
        }
    }

    void New(std::vector<std::string> &paths, CliPara::Options &opt, std::vector<SingleFileOperation> &tasks)
    {
        std::string templete_store = std::getenv("AMIO_TEMPLETE_STORE");
        if (templete_store.empty())
        {
            std::cerr << "EnvVarNotSetError: $AMIO_TEMPLETE_STORE is not set!" << std::endl;
            exit(-1);
        }
        if (!fs::exists(templete_store))
        {
            std::cerr << "EnvVarNotSetError: Templete store path: " << templete_store << " not exists!" << std::endl;
            exit(-1);
        }
        if (!fs::is_directory(templete_store))
        {
            std::cerr << "PathError: Templete store path: " << templete_store << " is not a directory!" << std::endl;
            exit(-1);
        }
        if (paths.empty())
        {
            return;
        }
        std::unordered_map<std::string, std::string> templetes;

        for (auto path : fs::directory_iterator(templete_store))
        {
            if (!fs::is_directory(path))
            {
                templetes[path.path().extension().string()] = path.path().string();
            }
        }
        std::string t_ext;
        for (auto path : paths)
        {
            path = AMPath::realpath(path, false, "\\");
            t_ext = AMPath::extension(path);
            if (templetes.find(t_ext) != templetes.end())
            {
                tasks.emplace_back(SingleFileOperation(FileOperationType::COPY, templetes[t_ext], AMPath::dirname(path), AMPath::basename(path), opt.mkdir));
            }
            else if (templetes.find(".txt") != templetes.end())
            {
                tasks.emplace_back(SingleFileOperation(FileOperationType::COPY, templetes[".txt"], AMPath::dirname(path), AMPath::basename(path), opt.mkdir));
            }
        }
    }
}

int main(int argc, char **argv)
{
    using task = SingleFileOperation;
    using op = FileOperationType;
    using EC = FileOperationResult;
    using CB = std::function<void(std::string, std::string, std::string)>;

    CLI::App app{"AMIO"};

    bool use_regex = false;
    bool mkdir = false;
    bool conflict_newname = false;
    bool force_overlap = false;
    bool quiet = false;
    bool permanent_delete = false;
    bool only_file = false;
    bool only_dir = false;

    std::vector<std::string> cp_paths;
    CLI::App *copy_cmd = app.add_subcommand("cp", "Copy path to a certain directory");
    copy_cmd->add_option("Srcs&Dst", cp_paths, "If only one path, copy to cwd or the last one serves as dst dir")
        ->required();
    copy_cmd->add_flag("-m,--mkdir", mkdir, "Make dir when dst dir not exists");
    copy_cmd->add_flag("-f,--file", only_file, "Only Match Files when search path(Won't exclude exact directory path in input)");
    copy_cmd->add_flag("-d,--dir", only_file, "Only Match Directories when search path(Won't exclude exact directory path in input)");
    copy_cmd->add_flag("-o,--overlap", force_overlap, "Overlap path when dst path already exists");
    copy_cmd->add_flag("-n,--new", conflict_newname, "Create new name when dst path already exists ");
    copy_cmd->add_flag("-r,--regex", use_regex, "User regex to find paths, use <> to wrap your pattern");
    copy_cmd->add_flag("-q,--quiet", quiet, "No UI, auto cre new name when conflict");

    std::vector<std::string> cl_paths;
    CLI::App *clone_cmd = app.add_subcommand("cl", "Clone src to dst");
    clone_cmd->add_option("Src&Dst", cl_paths, "Target Path and Destination")
        ->required()
        ->expected(2);
    clone_cmd->add_flag("-m,--mkdir", mkdir, "Make dir when dst dir not exists");
    clone_cmd->add_flag("-o,--overlap", force_overlap, "Overlap path when dst path already exists");
    clone_cmd->add_flag("-n,--new", conflict_newname, "Create new name when dst path already exists");
    clone_cmd->add_flag("-q,--quiet", quiet, "No UI, auto cre new name when conflict");

    std::vector<std::string> mv_paths;
    CLI::App *move_cmd = app.add_subcommand("mv", "Move path to a certain directory");
    move_cmd->add_option("Srcs&Dst", mv_paths, "If only one path, move to cwd or the last one serves as dst dir")
        ->required();
    move_cmd->add_flag("-m,--mkdir", mkdir, "Make dir when dst dir not exists");
    move_cmd->add_flag("-f,--file", only_file, "Only Match Files when search path(Won't exclude exact directory path in input)");
    move_cmd->add_flag("-d,--dir", only_file, "Only Match Directories when search path(Won't exclude exact directory path in input)");
    move_cmd->add_flag("-o,--overlap", force_overlap, "Overlap path when dst path already exists");
    move_cmd->add_flag("-n,--new", conflict_newname, "Create new name when dst path already exists ");
    move_cmd->add_flag("-r,--regex", use_regex, "User regex to find paths, use <> to wrap your pattern");
    move_cmd->add_flag("-q,--quiet", quiet, "No UI, auto cre new name when conflict");

    std::vector<std::string> mr_paths;
    CLI::App *replace_cmd = app.add_subcommand("mr", "Move and Replace");
    replace_cmd->add_option("Src&Dst", mr_paths, "Move the fist path to second's upper dir and replace it")
        ->required()
        ->expected(2);
    replace_cmd->add_flag("-m, --mkdir", mkdir, "Make dir when dst dir not exists");
    replace_cmd->add_flag("-o,--overlap", force_overlap, "Overlap path when dst path already exists");
    replace_cmd->add_flag("-n,--new", conflict_newname, "Create new name when dst path already exists");
    replace_cmd->add_flag("-q,--quiet", quiet, "No UI, auto cre new name when conflict");

    std::vector<std::string> rm_paths;
    CLI::App *remove_cmd = app.add_subcommand("rm", "Remove paths");
    remove_cmd->add_option("Paths", rm_paths, "The paths to be removed")
        ->required();
    remove_cmd->add_flag("-r,--regex", use_regex, "Use regex to find paths, use <> to wrap your pattern");
    remove_cmd->add_flag("-q,--quite", quiet, "Use regex to find paths, use <> to wrap your pattern");
    remove_cmd->add_flag("-p,--permanent", use_regex, "Directly delete path rather than move to Recycle Bin (But UNDO is still available)");

    std::vector<std::string> rn_paths;
    CLI::App *rename_cmd = app.add_subcommand("rn", "Rename path to a new name");
    rename_cmd->add_option("src&newname", rn_paths, "Rename the fist path to second's name")
        ->required()
        ->expected(2);
    rename_cmd->add_flag("-o,--overlap", force_overlap, "Overlap path when dst path already exists");

    std::vector<std::string> new_paths;
    CLI::App *new_cmd = app.add_subcommand("new", "Create a new file");
    new_cmd->add_option("dsts", new_paths, "The paths to be created")
        ->required();
    new_cmd->add_flag("-m,--mkdir", mkdir, "Make dir when dst dir not exists");
    new_cmd->add_flag("-q,--quiet", quiet, "No UI, auto cre new name when conflict");
    new_cmd->add_flag("-o,--overlap", force_overlap, "Overlap path when dst path already exists");

    try
    {
        CLI11_PARSE(app, argc, argv);
    }
    catch (const CLI::ParseError &e)
    {
        return app.exit(e);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Arg Parse Error: " << e.what() << std::endl;
        exit(-1);
    }

    CliPara::Options opt{only_file, only_dir, force_overlap, quiet, use_regex, mkdir, permanent_delete, conflict_newname};
    std::shared_ptr<CB> call_ptr = nullptr;
    if (!opt.quiet)
    {
        call_ptr = std::make_shared<CB>(CliFunc::OriCallback);
    }

    std::vector<task> TASKS;
    if (copy_cmd->parsed())
    {
        CliFunc::CopyMove(op::COPY, cp_paths, opt, TASKS, call_ptr);
    }
    else if (clone_cmd->parsed())
    {
        TASKS.emplace_back(task(op::COPY, mr_paths[0], AMPath::dirname(mr_paths[1]), AMPath::basename(mr_paths[1]), opt.mkdir));
    }
    else if (move_cmd->parsed())
    {
        CliFunc::CopyMove(op::MOVE, mv_paths, opt, TASKS, call_ptr);
    }
    else if (replace_cmd->parsed())
    {
        TASKS.emplace_back(task(op::MOVE, mr_paths[0], AMPath::dirname(mr_paths[1]), AMPath::basename(mr_paths[1]), opt.mkdir));
    }
    else if (remove_cmd->parsed())
    {
        CliFunc::Remove(op::REMOVE, rm_paths, opt, TASKS, call_ptr);
    }
    else if (rename_cmd->parsed())
    {
        amprint("rename", rn_paths[0], rn_paths[1]);
        TASKS.emplace_back(task(op::RENAME, rn_paths[0], "", rn_paths[1], opt.mkdir));
    }
    else if (new_cmd->parsed())
    {
        CliFunc::New(new_paths, opt, TASKS);
    }
    else
    {
        std::cerr << "ParsingError: No valid Funtion name provided!" << std::endl;
        exit(-1);
    }

    if (TASKS.empty())
    {
        std::cerr << "TaskLoadError: No tasks get!" << std::endl;
        exit(-2);
    }
    auto exp = ExplorerAPI();
    ECM ecm = exp.Init(opt.set);
    if (ecm.first != EC::SUCCESS)
    {
        std::cerr << GetECName(ecm.first) << ": " << ecm.second;
        exit(static_cast<int>(ecm.first));
    }
    bool has_task = false;
    for (auto task_i : TASKS)
    {
        ecm = exp.PendOperation(task_i);
        if (ecm.first != EC::SUCCESS)
        {
            std::cerr << GetECName(ecm.first) << ": " << ecm.second << std::endl;
        }
        else
        {
            has_task = true;
        }
    }
    if (!has_task)
    {
        std::cerr << "TaskLoadError: All tasks load failed!" << std::endl;
        exit(-2);
    }
    ecm = exp.Conduct();
    if (ecm.first != EC::SUCCESS)
    {
        std::cerr << GetECName(ecm.first) << ": " << ecm.second;
    }
}
