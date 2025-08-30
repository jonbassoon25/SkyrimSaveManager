#pragma once

class IniReader
{
private:
	std::string iniPath;
	std::string iniSection;

public:
	IniReader(const std::string& path, const std::string& iniSection);

	int ReadInt(const std::string& key, int default_);
	bool ReadBool(const std::string& key, bool default_);
	double ReadDouble(const std::string& key, double default_);
	std::string ReadString(const std::string& key, const std::string& default_);
};