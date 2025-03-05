from typing import Literal
from enum import Enum
class FileOperationType(Enum):
    COPY = 1
    MOVE = 2

class FileOperationResult(Enum):
    SUCCESS = 0
    NoIFileOperationInstance = -1
    FailToCreateIFileOperationInstance = 1
    FailToSetOperationFlags = 2
    FailToCreateSourceIShellItem = 3
    FailToCreateDestinationIShellItem = 4
    FailToAddOperation = 5
    FailToPerformOperation = 6
    WrongOperationType = 7
    FailToConfig = 8

class FileOperationSet:
    NoProgressUI: bool
    NoneUI: bool
    NoConfirmation: bool
    NoErrorUI: bool
    NoConfirmationForMakeDir: bool
    WarningIfPermanentDelete: bool

class ExplorerAPI:
    def __init__(self):
        pass

    def Config(self, set: FileOperationSet) -> FileOperationResult:
        pass

    def Init(self, set: FileOperationSet) -> FileOperationResult:
        pass

    def action(self, src: str, dst: str, action: FileOperationType) -> FileOperationResult:
        pass

    def rm(self, path: str) -> FileOperationResult:
        pass

    def copy(self, src: str, dst: str) -> FileOperationResult:
        pass

    def move(self, src: str, dst: str) -> FileOperationResult:
        pass
