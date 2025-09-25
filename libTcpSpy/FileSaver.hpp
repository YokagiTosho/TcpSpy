#ifndef FILE_SAVER_HPP
#define FILE_SAVER_HPP

#include <fstream>

template<typename Obj>
class FileSaver {
public:
	FileSaver(LPCWSTR filePath)
		: m_file(filePath)
	{
	}

	virtual bool save(const Obj& obj) = 0;

	FileSaver(const FileSaver&) = delete;
	FileSaver(FileSaver&&) = delete;
protected:
	std::wofstream m_file;
};






#endif