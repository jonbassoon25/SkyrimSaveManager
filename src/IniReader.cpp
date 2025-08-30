#include <Windows.h>

IniReader::IniReader(const std::string& path, const std::string& iniSection)
	: iniPath(path), iniSection(iniSection) {}

/*
Reads the integer value of key from the file and section of this reader. 
If no value is found, the default is returned.
*/
int IniReader::ReadInt(const std::string& key, int default_)
{
	return static_cast<int>(GetPrivateProfileIntA(iniSection.c_str(), key.c_str(), default_, iniPath.c_str()));
}

/*
Reads the boolean value of key from the file and section of this reader.
If no value is found, the default is returned.
*/
bool IniReader::ReadBool(const std::string& key, bool default_)
{
	char buffer[8] = {};
	std::string value_str((default_) ? "true" : "false");
	GetPrivateProfileStringA(iniSection.c_str(), key.c_str(), value_str.c_str(), buffer, sizeof(buffer), iniPath.c_str());
	value_str = std::string(buffer);
	return value_str == "1" || value_str == "true" || value_str == "True" || value_str == "TRUE";
}

/*
Reads the double value of key from the file and section of this reader.
If no value is found, the default is returned.
*/
double IniReader::ReadDouble(const std::string& key, double default_)
{
	char buffer[32] = {};
	GetPrivateProfileStringA(iniSection.c_str(), key.c_str(), std::to_string(default_).c_str(), buffer, sizeof(buffer), iniPath.c_str());

	try {
		return std::stod(std::string(buffer));
	}
	catch (...) {
		return default_;
	}
}

/*
Reads the string value of key from the file and section of this reader.
If no value is found, the default is returned.
The maximum string length that can be read is 254
*/
std::string IniReader::ReadString(const std::string& key, const std::string& default_)
{
	char buffer[255] = {};
	GetPrivateProfileStringA(iniSection.c_str(), key.c_str(), default_.c_str(), buffer, sizeof(buffer), iniPath.c_str());
	return std::string(buffer);
}