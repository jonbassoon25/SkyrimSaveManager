// Main script for SkyrimSaveManager

#include <thread>
#include <chrono>
#include <Windows.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <filesystem>
#include <shlobj.h>
#include <shellapi.h>

void LogDebugMsg(const std::string message) {
    RE::ConsoleLog::GetSingleton()->Print(message.c_str());
}

std::string GetIniPath() {
    char dllPathBuffer[MAX_PATH];
    HMODULE hMod = nullptr;

    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)GetIniPath, &hMod)) {
        GetModuleFileNameA(hMod, dllPathBuffer, MAX_PATH);
        std::string dllPath(dllPathBuffer);
        return dllPath.substr(0, dllPath.find_last_of("/\\")) + "\\SaveManager.ini";
    } else {
        return std::string("C:\\");
    }
}

class IniReader {
private:
    std::string iniPath;
    std::string iniSection;

public:
    IniReader(const std::string& path, const std::string& iniSection) {
        this->iniPath = path;
        this->iniSection = iniSection;
    }

    int ReadInt(const std::string& key, int default_) const {
        return static_cast<int>(GetPrivateProfileIntA(iniSection.c_str(), key.c_str(), default_, iniPath.c_str()));
    }

    bool ReadBool(const std::string& key, const std::string& default_) const {
        char buffer[6] = {};
        GetPrivateProfileStringA(iniSection.c_str(), key.c_str(), default_.c_str(), buffer, sizeof(buffer), iniPath.c_str());
        std::string value(buffer);
        return value == "1" || value == "true" || value == "True" || value == "TRUE";
    }

    float ReadFloat(const std::string& key, float default_) const {
        char buffer[32] = {};
        GetPrivateProfileStringA(iniSection.c_str(), key.c_str(), std::to_string(default_).c_str(), buffer, sizeof(buffer), iniPath.c_str());

        try {
            return std::stof(std::string(buffer));
        }
        catch (const std::exception&) {
            return default_;
        }
    }

    std::string ReadStr(const std::string& key, const std::string& default_) {
        char buffer[255] = {};
        GetPrivateProfileStringA(iniSection.c_str(), key.c_str(), default_.c_str(), buffer, sizeof(buffer), iniPath.c_str());
        return std::string(buffer);
    }
};

std::string GetSavePath() {
    PWSTR path = nullptr;
    std::string docPath = "C:\\";
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &path))) {
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, path, -1, NULL, 0, NULL, NULL);
        if (size_needed > 0) {
            char* buffer = new char[size_needed];
            WideCharToMultiByte(CP_UTF8, 0, path, -1, buffer, size_needed, NULL, NULL);
            std::string strPath(buffer);
            delete[] buffer;
            docPath = strPath;
        }
        docPath += "\\My Games\\Skyrim Special Edition\\";
        IniReader reader(docPath + "Skyrim.ini", "General");
        docPath += reader.ReadStr("SLocalSavePath", "Saves");

        CoTaskMemFree(path);
    }
    return docPath;
}


class SaveGame { // All save numbers are unique, but may be out of order by time
private:
    const std::string saveName;
    UINT32 saveNumber;
    UINT32 chainId;
    time_t saveTime;
    
    UINT32 CalcSaveNumber() {
        // The save number is always in the first entry in the save name
        // The save number is always followed by an _ and pre-fixed by "Save"
        for (int i = 4; i < saveName.length(); i++) {
            if (saveName[i] == '_') {
                return (UINT32) std::stoull(saveName.substr(4, i - 4));
            }
        }
        return 0;
    }
    
    time_t CalcSaveTime() {
        // Timestamp is the 7th entry in the format YYYYMMDDHHMMSS
        std::tm saveDate = { 0 };
        char entryCount = 1;
        for (size_t i = 0; i < saveName.length(); i++) {
            if (entryCount == 7) {
                saveDate.tm_year = std::stoi(saveName.substr(i, 4)) - 1900;
                saveDate.tm_mon = std::stoi(saveName.substr(i + 4, 2)) - 1;
                saveDate.tm_mday = std::stoi(saveName.substr(i + 6, 2));
                saveDate.tm_hour = std::stoi(saveName.substr(i + 8, 2));
                saveDate.tm_min = std::stoi(saveName.substr(i + 10, 2));
                saveDate.tm_sec = std::stoi(saveName.substr(i + 12, 2));
                break;
            }
            if (saveName[i] == '_') entryCount++;
        }
        return std::mktime(&saveDate);
    }

    UINT32 CalcChainId() {
        // Chain Id is always the 2nd entry and is 8 digit hex
        UINT32 saveId = 0;
        char entryCount = 1;
        for (int i = 0; i < saveName.length(); i++) {
            if (entryCount == 2) {
                saveId = (UINT32) std::stoull(saveName.substr(i, 8), nullptr, 16);
                break;
            }
            if (saveName[i] == '_') entryCount++;
        }
        return saveId;
    }

public:
    // Needs to compile with this so that unordered_map[SaveId] can be used in checks
    SaveGame() { throw std::out_of_range("Cannot construct a SaveGame without parameters"); }

    SaveGame(const std::string& fileName) : saveName(fileName) {
        // Try to stop Tod's intelligence from screwing over the plugin
        try {
            saveNumber = CalcSaveNumber();
        } catch (...) {
            LogDebugMsg("[SkyrimSaveManager] Error reading number of save:");
            LogDebugMsg(fileName);
            saveNumber = 0;
        }
        try {
            saveTime = CalcSaveTime(); // Possibly add option for using game time instead of irl time
        } catch (...) {
            // Possibly upgrade to reading metadata from the save as a backup
            //LogDebugMsg("[SkyrimSaveManager] Error reading time of save:");
            //LogDebugMsg(fileName);
            saveTime = 0;
        }
        try {
            chainId = CalcChainId();
        } catch (...) {
            LogDebugMsg("[SkyrimSaveManager] Error reading Id of save:");
            LogDebugMsg(fileName);
            chainId = 0; // Same as what Tod does when he can't read the save id
        }
    }
    std::string GetSaveName() {
        return saveName;
    }
    UINT32 GetChainId() {
        return chainId;
    }
    UINT32 GetNumber() {
        return saveNumber;
    }
    time_t GetTime() {
        return saveTime;
    }
};

// Documentation on user variables can be found in SaveManager.ini
struct UserVars {
    float pollTime;
    bool recycle;
    int primaryBlockCount;
    int secondaryBlockCount;
    float desiredSecondarySpacing;
    int tertiaryBlockCount;
    float desiredTertiarySpacing;
    int maxOverflow;
    float desiredOverflowSpacing;
};

class SaveChain {
private:
    UserVars userVars;
    std::string saveDir;

    std::vector<UINT32> primaryBlock;
    std::vector<UINT32> secondaryBlock;
    std::vector<UINT32> tertiaryBlock;
    std::vector<UINT32> overflow;
    std::unordered_map<UINT32, SaveGame> savesByNumber;

    void CleanPrimaryBlock() {
        // Move any overflow to the secondary block
        while (primaryBlock.size() > userVars.primaryBlockCount) {
            secondaryBlock.insert(secondaryBlock.begin(), primaryBlock.back());
            primaryBlock.pop_back();
        }
    }

    void CleanSecondaryBlock() {
        // Optimize to match the desired time spacing
        for (long long i = secondaryBlock.size() - 1; i >= 2; i--) {
            // If the time between the next next save and this save is less than or equal to the desired spacing
            if ((savesByNumber[secondaryBlock[i - 2]].GetTime() -
                savesByNumber[secondaryBlock[i]].GetTime()) <
                userVars.desiredSecondarySpacing * 3600)
            {
                // It is save to delete the save inbetween them
                DeleteSave(secondaryBlock, secondaryBlock[i - 1]);
            }
        }

        // Move any overflow to the tertiary block
        while (secondaryBlock.size() > userVars.secondaryBlockCount) {
            tertiaryBlock.insert(tertiaryBlock.begin(), secondaryBlock.back());
            secondaryBlock.pop_back();
        }
    }

    void CleanTertiaryBlock() {
        // Optimize to match the desired time spacing
        for (long long i = tertiaryBlock.size() - 1; i >= 2; i--) {
            // If the time between the next next save and this save is less than or equal to the desired spacing
            if ((savesByNumber[tertiaryBlock[i - 2]].GetTime() -
                savesByNumber[tertiaryBlock[i]].GetTime()) <
                userVars.desiredTertiarySpacing * 3600)
            {
                // It is save to delete the save inbetween them
                DeleteSave(tertiaryBlock, tertiaryBlock[i - 1]);
            }
        }

        // Move any overflow to the tertiary block
        while (tertiaryBlock.size() > userVars.tertiaryBlockCount) {
            overflow.insert(overflow.begin(), tertiaryBlock.back());
            tertiaryBlock.pop_back();
        }
    }

    void CleanOverflow() {
        // Match the desired time spacing
        for (long long i = overflow.size() - 1; i >= 2; i--) {
            // If the time between the next next save and this save is less than or equal to the desired spacing
            if ((savesByNumber[overflow[i - 2]].GetTime() -
                savesByNumber[overflow[i]].GetTime()) <
                userVars.desiredOverflowSpacing * 3600)
            {
                // It is save to delete the save inbetween them
                DeleteSave(overflow, overflow[i - 1]);
            }
        }
        // Delete any excess
        while (userVars.maxOverflow >= 0 && overflow.size() > userVars.maxOverflow) {
            UINT32 saveToDelete = overflow.back();
            DeleteSave(overflow, saveToDelete);
        }
    }

    bool RecycleFile(const std::string& filePathAnsi) {
        // Convert string to wstring
        int wlen = MultiByteToWideChar(CP_UTF8, 0, filePathAnsi.c_str(), -1, nullptr, 0);
        if (wlen == 0) return false;

        std::wstring filePathW(wlen, 0);
        MultiByteToWideChar(CP_UTF8, 0, filePathAnsi.c_str(), -1, &filePathW[0], wlen);

        std::wstring doubleNullPath = filePathW + L'\0';

        SHFILEOPSTRUCTW fileOp = { 0 };
        fileOp.wFunc = FO_DELETE;
        fileOp.pFrom = doubleNullPath.c_str();
        fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;

        int result = SHFileOperationW(&fileOp);
        return (result == 0 && !fileOp.fAnyOperationsAborted);
    }

    void DeleteSave(std::vector<UINT32>& affectedBlock, UINT32 SaveNumber) {
        // Collect the save to be deleted
        SaveGame saveToRemove = std::move(savesByNumber[SaveNumber]);
        // Remove the save from the SaveGame lookup map
        savesByNumber.erase(SaveNumber);

        // Remove the save number from it's block
        auto saveIt = std::find(affectedBlock.begin(), affectedBlock.end(), SaveNumber);
        assert(saveIt != affectedBlock.end());
        affectedBlock.erase(saveIt);

        // Remove the save's associated files
        std::string fileName = saveDir + "\\" + saveToRemove.GetSaveName();
        if (userVars.recycle) {
            RecycleFile(fileName + ".ess");
            RecycleFile(fileName + ".skse"); // silently fails if non-existent
        }
        else {
            bool esmDeleted = DeleteFileA((fileName + ".ess").c_str());
            DeleteFileA((fileName + ".skse").c_str()); // silently fails if non-existent
            if (!esmDeleted) LogDebugMsg("Failed to delete file: " + fileName);
        }
    }

public:
    SaveChain(UserVars& iniVariables, const std::string& saveDir) : userVars(iniVariables), saveDir(saveDir) {}

    void AddSave(SaveGame save) {
        // Verify that the given save does not already exist
        auto it = savesByNumber.find(save.GetNumber());
        if (it != savesByNumber.end()) {
            // This happens because of bugged savefiles.
            // Could delete them, but ignoring them is safer.
            return;
        }

        savesByNumber.emplace(save.GetNumber(), save);

        // Find the correct block for the save to be in
        std::vector<UINT32>* curBlockPtr = nullptr;
        if (primaryBlock.size() < userVars.primaryBlockCount ||
            save.GetTime() > savesByNumber[primaryBlock.back()].GetTime())
        {
            curBlockPtr = &primaryBlock;
        }
        else if (userVars.secondaryBlockCount > 0 &&
            (secondaryBlock.size() < userVars.secondaryBlockCount ||
             save.GetTime() > savesByNumber[secondaryBlock.back()].GetTime()))
        {
            curBlockPtr = &secondaryBlock;
        }
        else if (userVars.tertiaryBlockCount > 0 &&
            (tertiaryBlock.size() < userVars.tertiaryBlockCount ||
             save.GetTime() > savesByNumber[tertiaryBlock.back()].GetTime()))
        {
            curBlockPtr = &tertiaryBlock;
        }
        else {
            curBlockPtr = &overflow;
        }

        // Place the save in the correct spot inside it's block

        // If the block is empty, it will be sorted with the 1 item
        if (curBlockPtr->empty()) {
            curBlockPtr->emplace_back(save.GetNumber());
        }
        else {
            // Binary insertion
            long long lowIndex = 0;
            long long highIndex = curBlockPtr->size() - 1;
            while (lowIndex <= highIndex) {
                long long midIndex = (lowIndex + highIndex) / 2;
                time_t midVal = savesByNumber[curBlockPtr->at(midIndex)].GetTime();

                if (save.GetTime() < midVal) {
                    lowIndex = midIndex + 1;
                }
                else {
                    highIndex = midIndex - 1;
                }
            }
            curBlockPtr->insert(curBlockPtr->begin() + lowIndex, save.GetNumber());
        }

        //CheckBlockIntegrity(true);
        UpdateSaveBlocks();
    }

    void UpdateSaveBlocks() {
        assert(CheckBlockIntegrity());

        if (primaryBlock.size() > userVars.primaryBlockCount) {
            CleanPrimaryBlock();
        }
        if (secondaryBlock.size() > userVars.secondaryBlockCount) {
            CleanSecondaryBlock();
        }
        if (tertiaryBlock.size() > userVars.tertiaryBlockCount) {
            CleanTertiaryBlock();
        }
        if (overflow.size() > userVars.maxOverflow) {
            CleanOverflow();
        }
    }
    
    bool CheckBlockIntegrity(bool log = false) {
        // Check to see if all blocks are sorted correctly
        bool primarySorted = std::is_sorted(primaryBlock.begin(), primaryBlock.end(), [this](UINT32 a, UINT32 b) {
            return savesByNumber[a].GetTime() > savesByNumber[b].GetTime();
        });
        bool secondarySorted = std::is_sorted(secondaryBlock.begin(), secondaryBlock.end(), [this](UINT32 a, UINT32 b) {
            return savesByNumber[a].GetTime() > savesByNumber[b].GetTime();
        });
        bool tertiarySorted = std::is_sorted(tertiaryBlock.begin(), tertiaryBlock.end(), [this](UINT32 a, UINT32 b) {
            return savesByNumber[a].GetTime() > savesByNumber[b].GetTime();
        });
        bool overflowSorted = std::is_sorted(overflow.begin(), overflow.end(), [this](UINT32 a, UINT32 b) {
            return savesByNumber[a].GetTime() > savesByNumber[b].GetTime();
        });
        if (log) {
            if (!primarySorted) LogDebugMsg("Primary block not sorted.");
            if (!secondarySorted) LogDebugMsg("Secondary block not sorted.");
            if (!tertiarySorted) LogDebugMsg("Tertiary block not sorted.");
            if (!overflowSorted) LogDebugMsg("Overflow not sorted.");
        }

        // Check to see if blocks are ordered correctly
        bool ps_order = ((primaryBlock.empty() || secondaryBlock.empty()) ||
            (savesByNumber[primaryBlock.back()].GetTime() >= savesByNumber[secondaryBlock.front()].GetTime()));
        bool st_order = ((secondaryBlock.empty() || tertiaryBlock.empty()) ||
            (savesByNumber[secondaryBlock.back()].GetTime() >= savesByNumber[tertiaryBlock.front()].GetTime()));
        bool to_order = ((tertiaryBlock.empty() || overflow.empty()) ||
            (savesByNumber[tertiaryBlock.back()].GetTime() >= savesByNumber[overflow.front()].GetTime()));
        if (log) {
            if (!ps_order) LogDebugMsg("Primary-Secondary blocks not aligned.");
            if (!st_order) LogDebugMsg("Secondary-Tertiary blocks not aligned.");
            if (!to_order) LogDebugMsg("Tertiary-Overflow blocks not aligned.");
        }

        return primarySorted && secondarySorted && tertiarySorted && overflowSorted && ps_order && st_order && to_order;
    }
};


class SaveManager {
private:
    // User variables
    UserVars userVars;

    // Datastructure for all save chains
    std::unordered_map<UINT32, SaveChain> saveChainsById;

public:
    SaveManager() {
        IniReader reader(GetIniPath(), "SaveManager");

        // Load ini vars
        userVars.pollTime = reader.ReadFloat("fPollTime", 1.0);
        userVars.recycle = reader.ReadBool("bRecycle", "false");
        userVars.primaryBlockCount = reader.ReadInt("iPrimaryBlockCount", 16);
        userVars.secondaryBlockCount = reader.ReadInt("iSecondaryBlockCount", 32);
        userVars.desiredSecondarySpacing = reader.ReadFloat("fDesiredSecondarySpacing", 0.5);
        userVars.tertiaryBlockCount = reader.ReadInt("iTertiaryBlockCount", 64);
        userVars.desiredTertiarySpacing = reader.ReadFloat("fDesiredTertiarySpacing", 1.0);
        userVars.maxOverflow = reader.ReadInt("iMaxOverflow", -1);
        userVars.desiredOverflowSpacing = reader.ReadFloat("fDesiredOverflowSpacing", 4.0);

        // Clamp user input
        if (userVars.primaryBlockCount < 1) userVars.primaryBlockCount = 1;
        if (userVars.secondaryBlockCount < 0) userVars.secondaryBlockCount = 0;
        if (userVars.tertiaryBlockCount < 0) userVars.tertiaryBlockCount = 0;

        reset();
    }

    void reset() {
        saveChainsById.clear();
        // Find and group every game instance based on save Ids
        for (const auto& entry : std::filesystem::directory_iterator(GetSavePath())) {
            if (entry.is_regular_file() && entry.path().extension() == ".ess") {
                // SKSE save mirrors are assumed to not exist without a .ess counterpart
                // If the first 4 letters of the filename are not "Save" then move on (Autosave / Quicksave)
                std::string saveName = entry.path().stem().string();
                if (saveName.length() <= 4 || saveName.substr(0, 4) != "Save") continue;
                
                SaveGame curSave(saveName);
                auto found = saveChainsById.find(curSave.GetChainId());
                if (found != saveChainsById.end()) {
                    SaveChain& chain = found->second;
                    chain.AddSave(std::move(curSave));
                } else {
                    SaveChain chain(userVars, GetSavePath());
                    UINT32 chainId = curSave.GetChainId();
                    chain.AddSave(std::move(curSave));
                    saveChainsById.emplace(chainId, std::move(chain));
                }
            }
        }

        // Check integrety of each game instance
        for (auto& gameInstancePair : saveChainsById) {
            assert(gameInstancePair.second.CheckBlockIntegrity());
        }
    }

    float GetPollTime() {
        return userVars.pollTime;
    }
};


void RunSaveManager() {
    SaveManager manager;

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds((int) (manager.GetPollTime() * 60)));
        manager.reset();
    }
}

SKSEPluginLoad(const SKSE::LoadInterface *skse) {
    SKSE::Init(skse);

    // Start subprocess for save management after other mods are loaded
    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
        if (message->type == SKSE::MessagingInterface::kDataLoaded) {
            std::thread task(RunSaveManager);
            task.detach();
        }
    });

    return true;
}