import os

os.add_dll_directory(r"D:\Compiler\vcpkg\installed\x64-windows\bin")

from WinFile import (
    ExplorerAPI,
    FileOperationType,
    FileOperationResult,
    FileOperationSet,
)

file_copier = ExplorerAPI(FileOperationSet(NoProgressUI=False))
# res = file_copier.Init(
#     FileOperationSet(
#         NoProgressUI=False,
#         NoConfirmation=False,
#         NoErrorUI=False,
#         NoConfirmationForMakeDir=True,
#         WarningIfPermanentDelete=False,
#         AllowUndo=True,
#     )
# )
res = file_copier.Copy(
    [r"D:\Document\Desktop\创新源泉与能力"], r"D:\Document\Desktop\我的哈哈"
)
res = file_copier.Re([r"D:\Document\Desktop\123\CloudMusic"])
print(res)
