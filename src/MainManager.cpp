#include <filesystem>
#include <shlobj.h>
#include <iomanip>

#define iniName "SaveManager.ini"
#define undefinedLocalPath "___UNDEFINED_LOCAL_PATH___"

/* Determines the location of the SaveManager.ini file based on the path where this library is running from */
std::string MainManager::CalcIniPath()
{
    char pathBuffer[MAX_PATH];
    HMODULE hMod = nullptr;

    // Try to determine the location of this object
    // Increments the reference count of this library
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR) CalcIniPath, &hMod))
    {
        GetModuleFileNameA(hMod, pathBuffer, MAX_PATH);

        // Decrement the reference count of this library
        FreeLibrary(hMod);
        
        // Return the ini path
        std::string iniPath(pathBuffer);
        iniPath = iniPath.substr(0, iniPath.find_last_of("/\\")) + "\\" + iniName;
        if (std::filesystem::exists(iniPath))
        {
            return iniPath;
        }
        else {
            throw std::runtime_error("Could not find SaveManager.ini at " + iniPath);
        }
    }
    // We were unable to find the location of this library
    throw std::runtime_error("Unable to determine the path to SkyrimSaveManager.dll.");
}

std::string MainManager::CalcDocumentsPath()
{
    PWSTR path = nullptr;
    std::string docPath = "C:\\";
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &path)))
    {
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, path, -1, NULL, 0, NULL, NULL);
        if (size_needed > 0) {
            char* buffer = new char[size_needed];
            WideCharToMultiByte(CP_UTF8, 0, path, -1, buffer, size_needed, NULL, NULL);
            std::string strPath(buffer);
            delete[] buffer;
            docPath = strPath;
        }
        docPath += "\\My Games\\Skyrim Special Edition\\";

        CoTaskMemFree(path);
    }
    return docPath;
}

std::string MainManager::CalcLocalSavePath(const std::string& documentsPath = CalcDocumentsPath())
{
    std::string localSavePath(undefinedLocalPath);
    if (std::filesystem::exists(documentsPath + "SkyrimCustom.ini")) {
        IniReader reader(documentsPath + "SkyrimCustom.ini", "General");
        localSavePath = reader.ReadString("SLocalSavePath", undefinedLocalPath);
    }
    if (localSavePath == undefinedLocalPath && std::filesystem::exists(documentsPath + "Skyrim.ini")) {
        IniReader reader(documentsPath + "Skyrim.ini", "General");
        localSavePath = reader.ReadString("SLocalSavePath", undefinedLocalPath);
    }
    if (localSavePath == undefinedLocalPath) {
        localSavePath = "Saves/";
    }

    return localSavePath;
}

std::string MainManager::CalcSavePath()
{
    std::string documentsPath = CalcDocumentsPath();
    return documentsPath + CalcLocalSavePath(documentsPath);
}

MainManager::MainManager()
{
    // Load user variables
    IniReader reader(CalcIniPath(), "SaveManager");

    // Load ini vars
    userVariables.pollTime = reader.ReadFloat("fPollTime", 1.0);
    userVariables.recycle = reader.ReadBool("bRecycle", "false");
    userVariables.primaryBlockCount = reader.ReadInt("iPrimaryBlockCount", 16);
    userVariables.secondaryBlockCount = reader.ReadInt("iSecondaryBlockCount", 32);
    userVariables.desiredSecondarySpacing = reader.ReadFloat("fDesiredSecondarySpacing", 0.5);
    userVariables.tertiaryBlockCount = reader.ReadInt("iTertiaryBlockCount", 64);
    userVariables.desiredTertiarySpacing = reader.ReadFloat("fDesiredTertiarySpacing", 1.0);
    userVariables.maxOverflow = reader.ReadInt("iMaxOverflow", -1);
    userVariables.desiredOverflowSpacing = reader.ReadFloat("fDesiredOverflowSpacing", 4.0);
}

void MainManager::Refresh()
{

}

void MainManager::Reset()
{
    GamesById.clear();

    // Find and group every game instance based on game Ids
    std::string saveDirectory = CalcSavePath();
    for (const auto& entry : std::filesystem::directory_iterator(saveDirectory))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".ess")
        {
            // SKSE save mirrors are assumed to not exist without a .ess counterpart
            // If the first 4 letters of the filename are not "Save" then move on (Autosave / Quicksave)
            std::string saveName = entry.path().stem().string();
            if (saveName.length() <= 4 || saveName.substr(0, 4) != "Save") continue;

            SaveGame currentSave(saveName);
            auto gameHashPtr = GamesById.find(currentSave.GetGameId());
            if (gameHashPtr != GamesById.end())
            {
                GameManager& manager = gameHashPtr->second;
                manager.AddSave(std::move(currentSave));
            }
            else {
                GameManager newManager(userVariables, saveDirectory);
                UINT32 gameId = currentSave.GetGameId();
                newManager.AddSave(std::move(currentSave));
                GamesById.emplace(gameId, std::move(newManager));
            }
        }
    }

    // Check integrity of each game instance
    for (auto& gameInstancePair : GamesById) {
        if (!gameInstancePair.second.CheckBlockIntegrity(false))
        {
            throw std::runtime_error(std::format("Problem with the integrity of blocks after reset for game with id {:X}", gameInstancePair.first));
        }
    }
}

float MainManager::GetPollTime()
{
	return userVariables.pollTime;
}