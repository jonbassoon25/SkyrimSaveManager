// Main script for SkyrimSaveManager

#include <thread>
#include <chrono>
#include <Windows.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <filesystem>
#include <ShlObj.h>

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
        docPath += "\\My Games\\Skyrim Special Edition\\Saves";
        CoTaskMemFree(path);
    }
    return docPath;
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
};


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
        for (int i = 0; i < saveName.length(); i++) {
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
            LogDebugMsg("[SkyrimSaveManager] Error reading time of save:");
            LogDebugMsg(fileName);
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
};

// Documentation on user variables can be found in SaveManager.ini
struct UserVars {
    float pollTime;
    bool recycle;
    bool recycleOnStart;
    int maxSize;
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

    std::list<UINT32> primaryBlock;
    std::list<UINT32> secondaryBlock;
    std::list<UINT32> tertiaryBlock;
    std::list<UINT32> overflow;
    std::unordered_map<UINT32, SaveGame> savesByNumber;

public:
    SaveChain(UserVars& iniVariables) : userVars(iniVariables) {}

    void addSave(SaveGame save) {
        savesByNumber.emplace(save.GetNumber(), std::move(save));
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
        userVars.recycleOnStart = reader.ReadBool("bRecycleOnStart", "true") || userVars.recycle;
        userVars.maxSize = reader.ReadInt("iMaxSize", -1);
        userVars.primaryBlockCount = reader.ReadInt("iPrimaryBlockCount", 16);
        userVars.secondaryBlockCount = reader.ReadInt("iSecondaryBlockCount", 32);
        userVars.desiredSecondarySpacing = reader.ReadFloat("fDesiredSecondarySpacing", 0.5);
        userVars.tertiaryBlockCount = reader.ReadInt("iTertiaryBlockCount", 64);
        userVars.desiredTertiarySpacing = reader.ReadFloat("fDesiredTertiarySpacing", 1.0);
        userVars.maxOverflow = reader.ReadInt("iMaxOverflow", -1);
        userVars.desiredOverflowSpacing = reader.ReadFloat("fDesiredOverflowSpacing", 4.0);

        // Find and group every game instance based on save Ids
        unsigned long long saveCount = 0;
        unsigned gameCount = 0;
        for (const auto& entry : std::filesystem::directory_iterator(GetSavePath())) {
            if (entry.is_regular_file() && entry.path().extension() == ".ess") {
                // SKSE save mirrors are assumed to not exist without a .ess counterpart
                // If the first 4 letters of the filename are not "Save" then move on (Autosave / Quicksave)
                std::string saveName = entry.path().filename().string();
                if (saveName.length() <= 4 || saveName.substr(0, 4) != "Save") continue;
                
                saveCount++;
                SaveGame curSave(saveName);
                auto found = saveChainsById.find(curSave.GetChainId());
                if (found != saveChainsById.end()) {
                    SaveChain& chain = found->second;
                    chain.addSave(std::move(curSave));
                } else {
                    gameCount++;
                    SaveChain chain(userVars);
                    UINT32 chainId = curSave.GetChainId();
                    chain.addSave(std::move(curSave));
                    saveChainsById.emplace(chainId, std::move(chain));
                }
            }
        }
        LogDebugMsg(std::to_string(saveCount) + " saves detected from " + std::to_string(gameCount) + " games.");
    }


};


void RunSaveManager() {
    SaveManager manager;


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