/* 
 * This file is part of the EPollServer (https://github.com/geneidy/epollserver).
 * Copyright (c) 2017 geneidy.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <mutex>

#define SIZE_OF_FORMATED_DATE 12 + 1
#define SIZE_OF_DATE_TIME 26 + 1
// Definition of a multithread safe singleton logger class
class CComLog
{
public:
	static const std::string Debug;
	static const std::string Info;
	static const std::string Error;

	// Returns a reference to the singleton CComLog object
	static CComLog &instance();

	// Logs a single message at the given log level
	void log(const std::string &inMessage,
			 const std::string &inLogLevel);

	// Logs a vector of messages at the given log level
	void log(const std::vector<std::string> &inMessages,
			 const std::string &inLogLevel);

	char *GetFormatedDateTime();
	char *GetFormatedDate();

protected:
	// Static variable for the one-and-only instance
	static CComLog *pInstance;

	// Constant for the filename
	static const char *const kLogFileName;

	// Data member for the output stream
	std::fstream mOutputStream;

	// Embedded class to make sure the single CComLog
	// instance gets deleted on program shutdown.
	friend class Cleanup;
	class Cleanup
	{
	public:
		~Cleanup();
	};

	// Logs message. The thread should own a lock on sMutex
	// before calling this function.
	void logHelper(const std::string &inMessage,
				   const std::string &inLogLevel);

private:
	char szDateTime[SIZE_OF_DATE_TIME];
	char m_szLogDate[SIZE_OF_FORMATED_DATE];
	CComLog();
	virtual ~CComLog();
	CComLog(const CComLog &);
	CComLog &operator=(const CComLog &);
	static std::mutex sMutex;
};
