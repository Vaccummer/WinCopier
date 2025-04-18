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
res = file_copier.move([r"F:\Windows_Data\Desktop\sss"], r"F:\Windows_Data\Desktop\123")
print(res)
