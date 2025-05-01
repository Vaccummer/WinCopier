import os

os.add_dll_directory(r"E:\Compiler\vcpkg\installed\x64-windows\bin")
from WinFile import (
    ExplorerAPI,
    FileOperationType,
    FileOperationResult,
    FileOperationSet,
)

file_copier = ExplorerAPI()
# res = file_copier.Init(
#     FileOperationSet(
#         NoProgressUI=False,
#         NoneUI=False,
#         NoConfirmation=False,
#         NoErrorUI=False,
#         NoConfirmationForMakeDir=True,
#         WarningIfPermanentDelete=False,
#     )
# )

#
res = file_copier.Copy([r"D:\models\ZhipuAI"], r"F:\Windows_Data\Desktop\123")
print(res)
