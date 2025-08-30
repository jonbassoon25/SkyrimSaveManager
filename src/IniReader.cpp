#include <Windows.h>

IniReader::IniReader(const std::string& path, const std::string& iniSection)
{
	this->iniPath = path;
	this->iniSection = iniSection;
}

int IniReader::ReadInt(const std::string& key, int default_)
{
	return static_cast<int>(GetPrivateProfileIntA(iniSection.c_str(), key.c_str(), default_, iniPath.c_str()));
}

bool IniReader::ReadBool(const std::string& key, bool default_)
{
	char buffer[8] = {};
	std::string value_str((default_) ? "true" : "false");
	GetPrivateProfileStringA(iniSection.c_str(), key.c_str(), value_str.c_str(), buffer, sizeof(buffer), iniPath.c_str());
	value_str = std::string(buffer);
	return value_str == "1" || value_str == "true" || value_str == "True" || value_str == "TRUE";
}

double IniReader::ReadDouble(const std::string& key, double default_)
{
	char buffer[32] = {};
	GetPrivateProfileStringA(iniSection.c_str(), key.c_str(), std::to_string(default_).c_str(), buffer, sizeof(buffer), iniPath.c_str());

	try {
		return std::stod(std::string(buffer));
	}
	catch (const std::exception&) {
		return default_;
	}
}

std::string IniReader::ReadString(const std::string& key, const std::string& default_)
{
	char buffer[255] = {};
	GetPrivateProfileStringA(iniSection.c_str(), key.c_str(), default_.c_str(), buffer, sizeof(buffer), iniPath.c_str());
	return std::string(buffer);
}