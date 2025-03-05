from WinFile import ExplorerAPI, FileOperationType, FileOperationResult, FileOperationSet

file_copier = ExplorerAPI()
res = file_copier.Init(FileOperationSet(NoProgressUI=False, NoneUI=False, NoConfirmation=True, NoErrorUI=False, NoConfirmationForMakeDir=True, WarningIfPermanentDelete=False))
res = file_copier.rm(r"F:\Windows_Data\Desktop_File\asd - Copy.zip")
print(res)

