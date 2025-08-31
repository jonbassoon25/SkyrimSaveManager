#pragma once

class MainManager
{
private:
	UserVars userVariables;
	std::unordered_map<UINT32, GameManager> GamesById;

	std::string CalcIniPath() const;
	std::string CalcDocumentsPath() const;
	std::string CalcLocalSavePath() const;
	std::string CalcSavePath() const;


public:
	MainManager();

	void Refresh();
	void Reset();

	float GetPollTime();
};