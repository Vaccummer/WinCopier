from __future__ import annotations
import typing
__all__ = ['ExplorerAPI', 'FileOperationResult', 'FileOperationSet', 'FileOperationType']
class ExplorerAPI:
    def Config(self, set: FileOperationSet) -> FileOperationResult:
        ...
    def Copy(self, src: list[str], dst: str) -> list[tuple[str, FileOperationResult]] | FileOperationResult:
        ...
    def Init(self, set: FileOperationSet, pycb: typing.Any = None) -> FileOperationResult:
        ...
    def Move(self, src: list[str], dst: str) -> list[tuple[str, FileOperationResult]] | FileOperationResult:
        ...
    def Remove(self, path: list[str]) -> list[tuple[str, FileOperationResult]] | FileOperationResult:
        ...
    def SetPyTrace(self, pycb: typing.Any = None) -> None:
        ...
    def __init__(self, set: FileOperationSet = ...) -> None:
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
    """
    DstIsNotDir: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.DstIsNotDir: -13>
    DstIsNotExists: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.DstIsNotExists: -14>
    FailToAddOperation: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.FailToAddOperation: -8>
    FailToConfig: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.FailToConfig: -11>
    FailToCreDstShellItem: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.FailToCreDstShellItem: -7>
    FailToCreSrcShellItem: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.FailToCreSrcShellItem: -6>
    FailToCreateIFileOperationInstance: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.FailToCreateIFileOperationInstance: -18>
    FailToInitCOM: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.FailToInitCOM: -16>
    FailToPerformOperation: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.FailToPerformOperation: -9>
    FailToSetOperationFlags: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.FailToSetOperationFlags: -5>
    FaileToCreOperationInstance: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.FaileToCreOperationInstance: -4>
    NoIFileOperationInstance: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.NoIFileOperationInstance: -17>
    OperationAborted: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.OperationAborted: -19>
    PathNotExists: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.PathNotExists: -2>
    SUCCESS: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.SUCCESS: 0>
    UnknownError: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.UnknownError: -15>
    UpperDirNotExists: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.UpperDirNotExists: -12>
    WrongOperationType: typing.ClassVar[FileOperationResult]  # value = <FileOperationResult.WrongOperationType: -10>
    __members__: typing.ClassVar[dict[str, FileOperationResult]]  # value = {'SUCCESS': <FileOperationResult.SUCCESS: 0>, 'PathNotExists': <FileOperationResult.PathNotExists: -2>, 'FaileToCreOperationInstance': <FileOperationResult.FaileToCreOperationInstance: -4>, 'FailToSetOperationFlags': <FileOperationResult.FailToSetOperationFlags: -5>, 'FailToCreSrcShellItem': <FileOperationResult.FailToCreSrcShellItem: -6>, 'FailToCreDstShellItem': <FileOperationResult.FailToCreDstShellItem: -7>, 'FailToAddOperation': <FileOperationResult.FailToAddOperation: -8>, 'FailToPerformOperation': <FileOperationResult.FailToPerformOperation: -9>, 'WrongOperationType': <FileOperationResult.WrongOperationType: -10>, 'FailToConfig': <FileOperationResult.FailToConfig: -11>, 'UpperDirNotExists': <FileOperationResult.UpperDirNotExists: -12>, 'DstIsNotDir': <FileOperationResult.DstIsNotDir: -13>, 'DstIsNotExists': <FileOperationResult.DstIsNotExists: -14>, 'UnknownError': <FileOperationResult.UnknownError: -15>, 'FailToInitCOM': <FileOperationResult.FailToInitCOM: -16>, 'NoIFileOperationInstance': <FileOperationResult.NoIFileOperationInstance: -17>, 'FailToCreateIFileOperationInstance': <FileOperationResult.FailToCreateIFileOperationInstance: -18>, 'OperationAborted': <FileOperationResult.OperationAborted: -19>}
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
    AllowAdminPrivilege: bool
    AllowUndo: bool
    NoConfirmation: bool
    NoConfirmationForMakeDir: bool
    NoErrorUI: bool
    NoProgressUI: bool
    RenameOnCollision: bool
    WarningIfPermanentDelete: bool
    def __init__(self, NoProgressUI: bool = False, NoConfirmation: bool = False, NoErrorUI: bool = False, NoConfirmationForMakeDir: bool = True, WarningIfPermanentDelete: bool = False, RenameOnCollision: bool = False, AllowAdminPrivilege: bool = True, AllowUndo: bool = True) -> None:
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
