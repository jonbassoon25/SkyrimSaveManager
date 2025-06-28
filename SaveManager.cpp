// Main script for SkyrimSaveManager

#include <thread>
#include <chrono>
#include <Windows.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

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
};

class SaveManager {
private:
    // Ini vars
    // Additional documentation can be found in SaveManager.ini
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

public:
    SaveManager() {
        IniReader reader(GetIniPath(), "SaveManager");

        pollTime = reader.ReadFloat("fPollTime", 1.0);

        recycle = reader.ReadBool("bRecycle", "false");
        recycleOnStart = reader.ReadBool("bRecycleOnStart", "true") || recycle;

        maxSize = reader.ReadInt("iMaxSize", -1);

        primaryBlockCount = reader.ReadInt("iPrimaryBlockCount", 16);

        secondaryBlockCount = reader.ReadInt("iSecondaryBlockCount", 32);
        desiredSecondarySpacing = reader.ReadFloat("fDesiredSecondarySpacing", 0.5);

        tertiaryBlockCount = reader.ReadInt("iTertiaryBlockCount", 64);
        desiredTertiarySpacing = reader.ReadFloat("fDesiredTertiarySpacing", 1.0);

        maxOverflow = reader.ReadInt("iMaxOverflow", -1);
        desiredOverflowSpacing = reader.ReadFloat("fDesiredOverflowSpacing", 4.0);
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