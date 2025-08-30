SaveGame::SaveGame(const std::string& fileName) : name(fileName)
{
	saveTime = CalcSaveTime();
	gameId = CalcGameId();
}

/* 
    Calculates the save time of this save game from its file name
*/
time_t SaveGame::CalcSaveTime() const
{
    // Timestamp is the 7th entry in the format YYYYMMDDHHMMSS
    std::tm saveDate = { 0 };
    char entryCount = 1;
    for (size_t i = 0; i < name.length(); i++) {
        if (entryCount == 7) {
            // If the file name is incorrectly formatted,
            // which can happen if mods have locations with '_'
            // in their name, it will cause an error.
            try {
                saveDate.tm_year = std::stoul(name.substr(i, 4)) - 1900;
                saveDate.tm_mon = std::stoul(name.substr(i + 4, 2)) - 1;
                saveDate.tm_mday = std::stoul(name.substr(i + 6, 2));
                saveDate.tm_hour = std::stoul(name.substr(i + 8, 2));
                saveDate.tm_min = std::stoul(name.substr(i + 10, 2));
                saveDate.tm_sec = std::stoul(name.substr(i + 12, 2));
                break;
            }
            catch (...) {
                // TODO: Read metadata of the file instead of giving up
                return 0x0;
            }
        }
        if (name[i] == '_') entryCount++;
    }
    return std::mktime(&saveDate);
}

/*
    Calculates the game id of this save game from its file name
*/
UINT32 SaveGame::CalcGameId() const
{
    // The gameId is always the 2nd entry and is a 8 digit hex value
    UINT32 id = 0;
    char entryCount = 1;
    for (int i = 0; i < name.length(); i++) {
        if (entryCount == 2) {
            // If the file name is incorrectly formatted, it will cause an error.
            try {
                id = (UINT32) std::stoull(name.substr(i, 8), nullptr, 16);
                break;
            }
            catch (...) {
                return 0x0;
            }
        }
        if (name[i] == '_') entryCount++;
    }
    return id;
}

std::string SaveGame::GetName() const
{
    return name;
}
UINT32 SaveGame::GetGameId() const
{
    return gameId;
}
time_t SaveGame::GetSaveTime() const
{
    return saveTime;
}