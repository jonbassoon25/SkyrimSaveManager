#pragma once

class IniReader
{
private:
	std::string iniPath;
	std::string iniSection;

public:
	IniReader(const std::string& path, const std::string& iniSection);

	int ReadInt(const std::string& key, int default_) const;
	bool ReadBool(const std::string& key, bool default_) const;
	double ReadDouble(const std::string& key, double default_) const;
	std::string ReadString(const std::string& key, const std::string& default_) const;
};