#pragma once
#include <string>

/*
Error handler 

*/

class ErrorLogger
{
private:
	unsigned int m_nrOfErrors = 0;
	ErrorLogger() {};
public:
	ErrorLogger(const ErrorLogger&) = delete;
	void operator=(ErrorLogger const&) = delete;

	static ErrorLogger& get()
	{
		static ErrorLogger instance;
		return instance;
	}

	void logError(const char errorMsg[])
	{

		std::string errorPrefix = ">>>>>>>>>>>>>>>>>>>>>>> ENGINE ERROR #" + std::to_string(m_nrOfErrors) + ": '";
		std::string errorMsgString = std::string(errorMsg);
		std::string errorSuffix = "' >>>>>>>>>>>>>>>>>>>>>>>\n";

		std::string fullError = errorPrefix + errorMsgString + errorSuffix;

		m_nrOfErrors++;

		OutputDebugStringA(fullError.c_str());
	}
};