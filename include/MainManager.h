#pragma once

class MainManager
{
private:
	UserVars userVariables;
	std::unordered_map<UINT32, GameManager> GamesById;

	static std::string CalcIniPath();
	static std::string CalcDocumentsPath();
	static std::string CalcLocalSavePath(const std::string& documentsPath);
	static std::string CalcSavePath();


public:
	MainManager();

	void Refresh();
	void Reset();

	float GetPollTime();
};