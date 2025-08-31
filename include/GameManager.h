#pragma once


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


class GameManager
{
private:
    const UserVars& userVars;
    std::string saveDir;

    std::vector<SaveGame> primaryBlock;
    std::vector<SaveGame> secondaryBlock;
    std::vector<SaveGame> tertiaryBlock;
    std::vector<SaveGame> overflow;

    void RecycleFile(const std::string& path) const;
    void DeleteFile(const std::string& path) const;
    void DeleteSave(const std::vector<SaveGame>& affectedBlock, size_t index);

public:
    GameManager(const UserVars& userVariables, const std::string& saveDirectory);
    
    void AddSave(const SaveGame& save);
    bool CheckBlockIntegrity(bool log) const;
};