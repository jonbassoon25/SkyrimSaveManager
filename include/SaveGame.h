#pragma once

class SaveGame
{
private:
	std::string name;
	UINT32 gameId;
	time_t saveTime;

	time_t CalcSaveTime() const;
	UINT32 CalcGameId() const;

public:
	SaveGame(const std::string& fileName);

	std::string GetName() const;
	UINT32 GetGameId() const;
	time_t GetSaveTime() const;
};