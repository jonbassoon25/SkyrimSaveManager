#include <filesystem>
#include <shlobj.h>
#include <shellapi.h>


GameManager::GameManager(const UserVars& userVariables, const std::string& saveDirectory)
	: userVars(userVariables), saveDir(saveDirectory) { }

void GameManager::RecycleFile(const std::string& path) const
{
    // Convert string to wstring
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    if (wlen == 0) return;

    std::wstring filePathW(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &filePathW[0], wlen);

    std::wstring doubleNullPath = filePathW + L'\0';

    SHFILEOPSTRUCTW fileOp = { 0 };
    fileOp.wFunc = FO_DELETE;
    fileOp.pFrom = doubleNullPath.c_str();
    fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;

    SHFileOperationW(&fileOp);
}

void GameManager::DeleteSave(std::vector<SaveGame>& affectedBlock, size_t index)
{
    // Get the save to be deleted
    SaveGame& saveToRemove = affectedBlock.at(index);

    // Delete the files associated with the save to be deleted
    std::string fileName = saveDir + "\\" + saveToRemove.GetName();
    if (userVars.recycle) {
        RecycleFile(fileName + ".ess");
        RecycleFile(fileName + ".skse"); // silently fails if non-existent
    }
    else {
        DeleteFileA((fileName + ".ess").c_str());
        DeleteFileA((fileName + ".skse").c_str()); // silently fails if non-existent
    }

    // Remove the save from it's block
    affectedBlock.erase(affectedBlock.begin() + index);
}

void GameManager::AddSave(SaveGame save)
{

}

bool GameManager::CheckBlockIntegrity(bool log = false) const
{
	return false;
}