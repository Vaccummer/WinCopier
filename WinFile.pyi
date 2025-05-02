from __future__ import annotations
import typing
__all__ = ['ExplorerAPI', 'FileOperationResult', 'FileOperationSet', 'FileOperationStatus', 'FileOperationType', 'SingleFileOperation']
class ExplorerAPI:
    @typing.overload
    def Clone(self, src: str, dst: str, mkdir: bool = True, tmp_set: FileOperationSet = None) -> tuple[FileOperationResult, str]:
        ...
    @typing.overload
    def Clone(self, srcs_dst: dict[str, str], mkdir: bool = True, tmp_set: FileOperationSet = None) -> tuple[FileOperationStatus, list[tuple[str, tuple[FileOperationResult, str]]]]:
        ...
    @typing.overload
    def Conduct(self, action: FileOperationType, src: str, dst_dir: str = '', dst_name: str = '', mkdir: bool = False, tmp_set: FileOperationSet = None) -> tuple[FileOperationResult, str]:
        ...
    @typing.overload
    def Conduct(self, operation: SingleFileOperation, tmp_set: FileOperationSet = None) -> tuple[FileOperationResult, str]:
        ...
    @typing.overload
    def Conduct(self, operations: list[SingleFileOperation], tmp_set: FileOperationSet = None) -> tuple[FileOperationStatus, list[tuple[str, tuple[FileOperationResult, str]]]]:
        ...
    def Config(self, set: FileOperationSet) -> tuple[FileOperationResult, str]:
        ...
    @typing.overload
    def Copy(self, src: str, dst_dir: str, mkdir: bool = True, tmp_set: FileOperationSet = None) -> tuple[FileOperationResult, str]:
        ...
    @typing.overload
    def Copy(self, srcs: list[str], dst: str, mkdir: bool = True, tmp_set: FileOperationSet = None) -> tuple[FileOperationStatus, list[tuple[str, tuple[FileOperationResult, str]]]]:
        ...
    def GetSettings(self) -> FileOperationSet:
        ...
    def Init(self, set: FileOperationSet, pycb: typing.Any = None) -> tuple[FileOperationResult, str]:
        ...
    @typing.overload
    def Move(self, src: str, dst_dir: str, mkdir: bool = True, tmp_set: FileOperationSet = None) -> tuple[FileOperationResult, str]:
        ...
    @typing.overload
    def Move(self, srcs: list[str], dst: str, mkdir: bool = True, tmp_set: FileOperationSet = None) -> tuple[FileOperationStatus, list[tuple[str, tuple[FileOperationResult, str]]]]:
        ...
    @typing.overload
    def PendOperation(self, operation: SingleFileOperation) -> tuple[FileOperationResult, str]:
        ...
    @typing.overload
    def PendOperation(self, src: str, dst_dir: str, dst_name: str, action: FileOperationType, mkdir: bool = True) -> tuple[FileOperationResult, str]:
        ...
    @typing.overload
    def Remove(self, path: str, tmp_set: FileOperationSet = None) -> tuple[FileOperationResult, str]:
        ...
    @typing.overload
    def Remove(self, paths: list[str], tmp_set: FileOperationSet = None) -> tuple[FileOperationStatus, list[tuple[str, tuple[FileOperationResult, str]]]]:
        ...
    @typing.overload
    def Rename(self, src: str, new_name: str, tmp_set: FileOperationSet = None) -> tuple[FileOperationResult, str]:
        ...
    @typing.overload
    def Rename(self, srcs_new_names: dict[str, str], tmp_set: FileOperationSet = None) -> tuple[FileOperationStatus, list[tuple[str, tuple[FileOperationResult, str]]]]:
        ...
    @typing.overload
    def Replace(self, src: str, dst: str, tmp_set: FileOperationSet = None) -> tuple[FileOperationResult, str]:
        ...
    @typing.overload
    def Replace(self, srcs_dsts: dict[str, str], tmp_set: FileOperationSet = None) -> tuple[FileOperationStatus, list[tuple[str, tuple[FileOperationResult, str]]]]:
        ...
    def __init__(self, set: FileOperationSet = ...) -> None:
        ...
    @property
    def Tracer(self) -> AMTracer:
        ...
class FileOperationResult:
    """
    Members:
    
      SUCCESS
    
      PathNotExists
    
      FaileToCreOperationInstance
    
      FailToSetOperationFlags
    
      FailToCreSrcShellItem
    
      FailToCreDstShellItem
    
      FailToAddOperation
    
      FailToPerformOperation
    
      WrongOperationType
    
      FailToConfig
    
      UpperDirNotExists
    
      DstIsNotDir
    
      DstIsNotExists
    
      UnknownError
    
      FailToInitCOM
    
      NoIFileOperationInstance
    
      FailToCreateIFileOperationInstance
    
      OperationAborted
    
      COMInitFailed
    
      PyTraceError
    
      FailToCreateDir
    
      InvalidArgument
    """
    COMInitFailed: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.COMInitFailed: -20>
    DstIsNotDir: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.DstIsNotDir: -13>
    DstIsNotExists: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.DstIsNotExists: -14>
    FailToAddOperation: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.FailToAddOperation: -8>
    FailToConfig: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.FailToConfig: -11>
    FailToCreDstShellItem: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.FailToCreDstShellItem: -7>
    FailToCreSrcShellItem: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.FailToCreSrcShellItem: -6>
    FailToCreateDir: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.FailToCreateDir: -22>
    FailToCreateIFileOperationInstance: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.FailToCreateIFileOperationInstance: -18>
    FailToInitCOM: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.FailToInitCOM: -16>
    FailToPerformOperation: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.FailToPerformOperation: -9>
    FailToSetOperationFlags: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.FailToSetOperationFlags: -5>
    FaileToCreOperationInstance: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.FaileToCreOperationInstance: -4>
    InvalidArgument: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.InvalidArgument: -23>
    NoIFileOperationInstance: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.NoIFileOperationInstance: -17>
    OperationAborted: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.OperationAborted: -19>
    PathNotExists: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.PathNotExists: -2>
    PyTraceError: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.PyTraceError: -21>
    SUCCESS: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.SUCCESS: 0>
    UnknownError: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.UnknownError: -15>
    UpperDirNotExists: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.UpperDirNotExists: -12>
    WrongOperationType: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.WrongOperationType: -10>
    __members__: typing.ClassVar[dict[str, FileOperationResult]]  # value = {'SUCCESS': <FileOperationResult.SUCCESS: 0>, 'PathNotExists': <FileOperationResult.PathNotExists: -2>, 'FaileToCreOperationInstance': <FileOperationResult.FaileToCreOperationInstance: -4>, 'FailToSetOperationFlags': <FileOperationResult.FailToSetOperationFlags: -5>, 'FailToCreSrcShellItem': <FileOperationResult.FailToCreSrcShellItem: -6>, 'FailToCreDstShellItem': <FileOperationResult.FailToCreDstShellItem: -7>, 'FailToAddOperation': <FileOperationResult.FailToAddOperation: -8>, 'FailToPerformOperation': <FileOperationResult.FailToPerformOperation: -9>, 'WrongOperationType': <FileOperationResult.WrongOperationType: -10>, 'FailToConfig': <FileOperationResult.FailToConfig: -11>, 'UpperDirNotExists': <FileOperationResult.UpperDirNotExists: -12>, 'DstIsNotDir': <FileOperationResult.DstIsNotDir: -13>, 'DstIsNotExists': <FileOperationResult.DstIsNotExists: -14>, 'UnknownError': <FileOperationResult.UnknownError: -15>, 'FailToInitCOM': <FileOperationResult.FailToInitCOM: -16>, 'NoIFileOperationInstance': <FileOperationResult.NoIFileOperationInstance: -17>, 'FailToCreateIFileOperationInstance': <FileOperationResult.FailToCreateIFileOperationInstance: -18>, 'OperationAborted': <FileOperationResult.OperationAborted: -19>, 'COMInitFailed': <FileOperationResult.COMInitFailed: -20>, 'PyTraceError': <FileOperationResult.PyTraceError: -21>, 'FailToCreateDir': <FileOperationResult.FailToCreateDir: -22>, 'InvalidArgument': <FileOperationResult.InvalidArgument: -23>}
    def __eq__(self, other: typing.Any) -> bool:
        ...
    def __getstate__(self) -> int:
        ...
    def __hash__(self) -> int:
        ...
    def __index__(self) -> int:
        ...
    def __init__(self, value: int) -> None:
        ...
    def __int__(self) -> int:
        ...
    def __ne__(self, other: typing.Any) -> bool:
        ...
    def __repr__(self) -> str:
        ...
    def __setstate__(self, state: int) -> None:
        ...
    def __str__(self) -> str:
        ...
    @property
    def name(self) -> str:
        ...
    @property
    def value(self) -> int:
        ...
class FileOperationSet:
    AllowAdmin: bool
    AllowUndo: bool
    AlwaysYes: bool
    DeleteWarning: bool
    Hardlink: bool
    IsDefault: bool
    NoErrorUI: bool
    NoMkdirInfo: bool
    NoProgressUI: bool
    RenameOnCollision: bool
    ToRecycleBin: bool
    def __init__(self, NoProgressUI: bool = False, NoConfirmation: bool = False, NoErrorUI: bool = False, NoMkdirInfo: bool = True, DeleteWarning: bool = False, RenameOnCollision: bool = False, AllowAdmin: bool = True, AllowUndo: bool = True, Hardlink: bool = False, ToRecycleBin: bool = False) -> None:
        ...
class FileOperationStatus:
    """
    Members:
    
      Perfect
    
      PartialSuccess
    
      NoOperation
    
      FinalError
    
      AllErrors
    
      Uninitialized
    """
    AllErrors: typing.ClassVar[FileOperationStatus]  # value = <FileOperationStatus.AllErrors: -2>
    FinalError: typing.ClassVar[FileOperationStatus]  # value = <FileOperationStatus.FinalError: -1>
    NoOperation: typing.ClassVar[FileOperationStatus]  # value = <FileOperationStatus.NoOperation: 2>
    PartialSuccess: typing.ClassVar[FileOperationStatus]  # value = <FileOperationStatus.PartialSuccess: 1>
    Perfect: typing.ClassVar[FileOperationStatus]  # value = <FileOperationStatus.Perfect: 0>
    Uninitialized: typing.ClassVar[FileOperationStatus]  # value = <FileOperationStatus.Uninitialized: -3>
    __members__: typing.ClassVar[dict[str, FileOperationStatus]]  # value = {'Perfect': <FileOperationStatus.Perfect: 0>, 'PartialSuccess': <FileOperationStatus.PartialSuccess: 1>, 'NoOperation': <FileOperationStatus.NoOperation: 2>, 'FinalError': <FileOperationStatus.FinalError: -1>, 'AllErrors': <FileOperationStatus.AllErrors: -2>, 'Uninitialized': <FileOperationStatus.Uninitialized: -3>}
    def __eq__(self, other: typing.Any) -> bool:
        ...
    def __getstate__(self) -> int:
        ...
    def __hash__(self) -> int:
        ...
    def __index__(self) -> int:
        ...
    def __init__(self, value: int) -> None:
        ...
    def __int__(self) -> int:
        ...
    def __ne__(self, other: typing.Any) -> bool:
        ...
    def __repr__(self) -> str:
        ...
    def __setstate__(self, state: int) -> None:
        ...
    def __str__(self) -> str:
        ...
    @property
    def name(self) -> str:
        ...
    @property
    def value(self) -> int:
        ...
class FileOperationType:
    """
    Members:
    
      COPY
    
      MOVE
    
      REMOVE
    """
    COPY: typing.ClassVar[FileOperationType]  # value = <FileOperationType.COPY: 1>
    MOVE: typing.ClassVar[FileOperationType]  # value = <FileOperationType.MOVE: 2>
    REMOVE: typing.ClassVar[FileOperationType]  # value = <FileOperationType.REMOVE: 3>
    __members__: typing.ClassVar[dict[str, FileOperationType]]  # value = {'COPY': <FileOperationType.COPY: 1>, 'MOVE': <FileOperationType.MOVE: 2>, 'REMOVE': <FileOperationType.REMOVE: 3>}
    def __eq__(self, other: typing.Any) -> bool:
        ...
    def __getstate__(self) -> int:
        ...
    def __hash__(self) -> int:
        ...
    def __index__(self) -> int:
        ...
    def __init__(self, value: int) -> None:
        ...
    def __int__(self) -> int:
        ...
    def __ne__(self, other: typing.Any) -> bool:
        ...
    def __repr__(self) -> str:
        ...
    def __setstate__(self, state: int) -> None:
        ...
    def __str__(self) -> str:
        ...
    @property
    def name(self) -> str:
        ...
    @property
    def value(self) -> int:
        ...
class SingleFileOperation:
    action: FileOperationType
    dst_dir: str
    dst_name: str
    mkdir: bool
    src: str
