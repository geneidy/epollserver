////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CComLog.cpp
#include <stdexcept>
#include "ComLog.h"

#include "time.h"
#include "string.h"



using namespace std;

const string CComLog::Debug = "[D]";
const string CComLog::Info  = "[I]";
const string CComLog::Error = "[E]";

const char* const  CComLog::kLogFileName = {""};

CComLog* CComLog::pInstance = nullptr;
mutex 	CComLog::sMutex;
////////////////////////////////////////////////////////////////////
CComLog& CComLog::instance()
{
    static Cleanup cleanup;

    lock_guard<mutex> guard(sMutex);
    if (pInstance == nullptr)
        pInstance = new CComLog();
    return *pInstance;
}
////////////////////////////////////////////////////////////////////
CComLog::Cleanup::~Cleanup()
{
    lock_guard<mutex> guard(CComLog::sMutex);
    delete CComLog::pInstance;
    CComLog::pInstance = nullptr;
}
////////////////////////////////////////////////////////////////////
CComLog::~CComLog()
{
    mOutputStream.close();
}
////////////////////////////////////////////////////////////////////
CComLog& CComLog::operator=(const CComLog&rhs )
{
  
  return *this;
}
////////////////////////////////////////////////////////////////////
CComLog::CComLog()
{
    std::string strkLogFileName;
    strkLogFileName.empty();
    
 
    struct stat st = {0};
    if (stat("../Logs/", &st) == -1) {
        mkdir("../Logs/", 0700);
    }

    strkLogFileName = "../Logs/"; 
    strkLogFileName +=   GetFormatedDate();
    strkLogFileName += "ComLog.txt";

    mOutputStream.open(strkLogFileName, std::fstream::in | std::fstream::out |std::fstream::app);
    if (!mOutputStream.good()) {
        throw runtime_error("Unable to initialize the CComLog!");
    }
}
////////////////////////////////////////////////////////////////////
void CComLog::log(const string& inMessage, const string& inLogLevel)
{
    lock_guard<mutex> guard(sMutex);
    logHelper(inMessage, inLogLevel);
}
////////////////////////////////////////////////////////////////////
void CComLog::log(const vector<string>& inMessages, const string& inLogLevel)
{
    lock_guard<mutex> guard(sMutex);
    for (size_t i = 0; i < inMessages.size(); i++) {
        logHelper(inMessages[i], inLogLevel);
    }
}
/////////////////////////////////////////////////////////////////////////////////////
void CComLog::logHelper(const std::string& inMessage, const std::string& inLogLevel)
{
    // Add logger time in the first column !!!
    time_t ltime = 0;
    struct tm stToday;

    char	szLogTime[SIZE_OF_DATE_TIME];
    time( &ltime );
    localtime_r( &ltime ,  &stToday);

    memset(szLogTime, 0 , SIZE_OF_DATE_TIME);
    strftime(szLogTime, SIZE_OF_DATE_TIME, "%Y-%m-%d- %H:%M:%S" , &stToday);
    mOutputStream << szLogTime << ": " << inLogLevel << " : " << inMessage << endl;
}
///////////////////////////////////////////////////////////////////////////////////
char* CComLog::GetFormatedDateTime()
{
    // Add logger time in the first column !!!
    time_t ltime = 0;
    struct tm stToday;

    time( &ltime );
    localtime_r( &ltime ,  &stToday);

    memset(szDateTime, 0 , SIZE_OF_DATE_TIME);
    strftime(szDateTime, 26, "%Y-%m-%d- %H:%M:%S" , &stToday);
    
    return szDateTime;
 }

///////////////////////////////////////////////////////////////////////////////////
char* CComLog::GetFormatedDate()
{
    struct tm stToday;
    time_t ltime = 0;

    time( &ltime );
    localtime_r( &ltime ,  &stToday);

    memset(m_szLogDate, 0 , SIZE_OF_FORMATED_DATE);
    strftime(m_szLogDate, SIZE_OF_FORMATED_DATE, "%Y-%m-%d-" , &stToday);
    return m_szLogDate;
}
////////////////////////////////////////////////////////////////////////////////////////
