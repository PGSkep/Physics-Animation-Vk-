#ifndef	LOGGER_H
#define LOGGER_H

#include <fstream>
#include <string>

#define LOG_COUT
#define LOG_SAVE_ALWAYS

#ifdef LOG_COUT
#include <iostream>
#endif

class Logger
{
	std::ofstream file;
	std::string name;

public:

	void Start(const char* _logFilename)
	{
		if (file.is_open())
			file.close();
		file = std::ofstream(_logFilename);
		name = _logFilename;
	}
	void Set(const char* _logFilename)
	{
		if (file.is_open())
			file.close();
		file = std::ofstream(_logFilename, std::fstream::app);
		name = _logFilename;
	}
	void Close()
	{
		file.close();
	}

	template <typename T>
	Logger &operator<<(const T &_data)
	{
		file << _data;

		#ifdef LOG_SAVE_ALWAYS
		Close();
		Set(name.c_str());
		#endif

		#ifdef LOG_COUT
		std::cout << _data;
		#endif

		return *this;
	}

	~Logger()
	{
		file.close();
	}
};

#endif
