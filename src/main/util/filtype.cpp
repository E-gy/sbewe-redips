#include "filetype.hpp"

#include <sys/stat.h>

FileType getFileType(const std::string& f){
	struct ::stat finf;
	if(::stat(f.c_str(), &finf) < 0) return FileType::NE;
	return S_ISDIR(finf.st_mode) ? FileType::Directory : FileType::File;
}
