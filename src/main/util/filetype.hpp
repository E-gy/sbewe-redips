#pragma once

#include <string>

constexpr auto FPATHSEP = '/';

enum class FileType {
	NE, Directory, File
};

FileType getFileType(const std::string&);
