GameManager::GameManager(const UserVars& userVariables, const std::string& saveDirectory)
	: userVars(userVariables), saveDir(saveDirectory) { }

void GameManager::RecycleFile(const std::string& path) const
{

}

void GameManager::DeleteFile(const std::string& path) const
{

}

void GameManager::DeleteSave(const std::vector<SaveGame>& affectedBlock, size_t index)
{

}

void GameManager::AddSave(const SaveGame& save)
{

}

bool GameManager::CheckBlockIntegrity(bool log = false) const
{
	return false;
}