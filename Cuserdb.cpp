
#include "Cuserdb.h"
#include "ComLog.h"

CuserDB::CuserDB(char* szUserFileName)
{

    m_iError = VALID_USER_FILE_NAME;

    struct stat st = {0};

    if (stat("../Users/", &st) == -1) {
        mkdir("../Users/", 0700);
    }

    string strUserDBFile;
    strUserDBFile.clear();

    strUserDBFile = "../Users/";
    strUserDBFile += szUserFileName;

    m_ifd = open(strUserDBFile.c_str(), O_CREAT|O_RDWR|O_APPEND, S_IRWXU);

    USER_RECORD UserRecord = {0};

    m_iSizeOfUserRecord = sizeof (USER_RECORD);
    int iRecSize = 0;

    if (m_ifd == -1) {
        CComLog::instance().log("Open User DB File...Error Opening File: ", CComLog::Error);
        CComLog::instance().log(strUserDBFile, CComLog::Error);
        m_iError = INVALID_USER_FILE_NAME;

        // Set error code and exit
    }
    else {// load users in Map
        int nCount = 0;
        while ((iRecSize = read(m_ifd,  &UserRecord, m_iSizeOfUserRecord)) > 0) {
            m_itUserMap = m_UserMap.find(UserRecord.szUserName);
            if (m_itUserMap == m_UserMap.end()) { // Not found
                m_UserMap.insert(pair<string, USER_RECORD> ( UserRecord.szUserName, UserRecord));
                nCount++;
            }
        } // while
        string strMsg;
        strMsg = "Inserted: " + to_string(nCount) + " User Records";
        CComLog::instance().log(strMsg, CComLog::Info);
    } // else
    
    my_set.insert(1);
    my_set.insert(iRecSize);

}
//********************************************************************************************//
int CuserDB::GetError()
{
    return m_iError;

}
//********************************************************************************************//
CuserDB::~CuserDB()
{
    SaveUserFile();
    close(m_ifd);
    m_UserMap.clear();
}
//********************************************************************************************//
int CuserDB::VerifyUser(char* szUsername, char* szPassword)
{
    USER_RECORD UserRecord = {0};
    string strMsg;

    m_itUserMap = m_UserMap.find(szUsername);

    if (m_itUserMap == m_UserMap.end()) {
        strMsg.clear();
        strMsg = "User : " + string(szUsername) + " Invalid User Name";
        CComLog::instance().log(strMsg, CComLog::Info);

        return INVALID_USER_NAME;
    }

    UserRecord = m_itUserMap->second;

    if (strcmp(szPassword, UserRecord.szPassword)) {
        strMsg.clear();
        strMsg = "User : " + string(szUsername) + " Invalid Password";
        CComLog::instance().log(strMsg, CComLog::Info);
	strcpy(m_itUserMap->second.szLastMessage, "Loggin Failed");;
	strcpy(m_itUserMap->second.szLastLoginTime, CComLog::instance().GetFormatedDateTime());

      return INVALID_PASSWORD;
    }

    if (!UserRecord.bActive) {
        strMsg.clear();
        strMsg = "User : " + string(szUsername) + " Inactive";
        CComLog::instance().log(strMsg, CComLog::Info);
        return USER_INACTIVE;
    }
    strMsg = "User : " + string(szUsername) + " Successfull Login";
    CComLog::instance().log(strMsg, CComLog::Info);

    strcpy(m_itUserMap->second.szLastMessage, "Loggin Failed");;
    strcpy(m_itUserMap->second.szLastLoginTime, CComLog::instance().GetFormatedDateTime());

    return VALID_USER;
}
//********************************************************************************************//
int CuserDB::AddUser(char* szUsername, char* szPassword, int iGroupID, int iAccessLevel)
{
    USER_RECORD UserRecord = {0};
    pair <MapUserDB::iterator, bool> pairInsert;

    UserRecord.bActive = true;
    UserRecord.iAccessLevel = iAccessLevel;
    strcpy(UserRecord.szUserName, szUsername);
    strcpy(UserRecord.szPassword, szPassword);
    UserRecord.iGroupID = iGroupID;

    strcpy(UserRecord.szDateAccountCreated, CComLog::instance().GetFormatedDateTime());
    strcpy(UserRecord.szLastMessage, "Account Created");

    strcpy(UserRecord.szDateAccountCreated, CComLog::instance().GetFormatedDate());    
    
    pairInsert = m_UserMap.insert(pair<string, USER_RECORD> (szUsername, UserRecord));

    if (!pairInsert.second) {
        m_strLogMsg = "User: " + string(szUsername) + " NOT Added...Dupilcate Username ";
        CComLog::instance().log(m_strLogMsg, CComLog::Info);

        return USER_ALREADY_EXIST;
    }

    m_strLogMsg = "User: " + string(szUsername) + " Added ";
    CComLog::instance().log(m_strLogMsg, CComLog::Info);
    

    return VALID_USER;
}
//********************************************************************************************//
int CuserDB:: ChangeUserPassword(char* szUsername, char* szPassword)
{
    USER_RECORD UserRecord = {0};
    pair <MapUserDB::iterator, bool> pairInsert;

    m_itUserMap = m_UserMap.find(szUsername);

    if (m_itUserMap == m_UserMap.end()) {
        return INVALID_USER_NAME;
    }

    UserRecord = m_itUserMap->second;
    strcpy(UserRecord.szPassword, szPassword);

    m_UserMap.erase(m_itUserMap);

    pairInsert = m_UserMap.insert(pair<string, USER_RECORD> (szUsername, UserRecord));

    m_strLogMsg = "User: " + string(szUsername);
    if (pairInsert.second) {
        m_strLogMsg +=  " Password changed ";
        CComLog::instance().log(m_strLogMsg, CComLog::Info);
    }
    else {
        m_strLogMsg +=  " Error Modifiying... ";
        CComLog::instance().log(m_strLogMsg, CComLog::Error);
    }
    return true;
}
//********************************************************************************************//
int CuserDB::ModifyUser(char* szUsername, bool bActive, int iGroupID, int iAccessLevel)
{
    USER_RECORD UserRecord = {0};
    pair <MapUserDB::iterator, bool> pairInsert;

    m_itUserMap = m_UserMap.find(szUsername);

    if (m_itUserMap == m_UserMap.end()) {
        return INVALID_USER_NAME;
    }

    UserRecord = m_itUserMap->second;

    UserRecord.bActive = bActive;
    UserRecord.iAccessLevel = iAccessLevel;
    UserRecord.iGroupID = iGroupID;


    m_UserMap.erase(m_itUserMap);

    pairInsert = m_UserMap.insert(pair<string, USER_RECORD> (szUsername, UserRecord));

    m_strLogMsg = "User: " + string(szUsername);
    if (pairInsert.second) {
        m_strLogMsg +=  " Modified ";
        CComLog::instance().log(m_strLogMsg, CComLog::Info);
    }
    else {
        m_strLogMsg +=  " Error Modifiying... ";
        CComLog::instance().log(m_strLogMsg, CComLog::Error);
    }

    return true;
}
//********************************************************************************************//
int CuserDB::DeleteUser(char* szUsername)
{

    m_itUserMap = m_UserMap.find(szUsername);

    if (m_itUserMap == m_UserMap.end()) {
        return INVALID_USER_NAME;
    }

    m_UserMap.erase(m_itUserMap);
    m_strLogMsg = "User: " + string(szUsername) + " Deleted ";
    CComLog::instance().log(m_strLogMsg, CComLog::Info);

    return true;
}
//********************************************************************************************//
int CuserDB::ListDB()
{
    int iWrite = 0;

    for (m_itUserMap = m_UserMap.begin(); m_itUserMap != m_UserMap.end(); ++m_itUserMap) {
        cout <<  "Usename: " << m_itUserMap->second.szUserName << endl;
        cout <<  "Password: " << m_itUserMap->second.szPassword << endl;
        cout <<  "Group ID: " << m_itUserMap->second.iGroupID << endl;
        cout <<  "Access Level: " << m_itUserMap->second.iAccessLevel << endl;
        cout <<  "Active: " << m_itUserMap->second.bActive << endl;
        cout <<  "Last Login Time: " << m_itUserMap->second.szLastLoginTime << endl;
        cout <<  "Last Logout: " << m_itUserMap->second.szLastLogoutTime << endl;
        cout <<  "Last Message: " << m_itUserMap->second.szLastMessage << endl;
	cout << "---" << endl;
	cout <<  "Date Account Created: " << m_itUserMap->second.szDateAccountCreated << endl;
        cout <<  "Date Account Terminated: " << m_itUserMap->second.szDateAccountTerminated << endl;
	
	
	        
        iWrite++;
        cout<< "===============================================================================" << endl;
    }

    return  iWrite++;;
}
//********************************************************************************************//
int CuserDB::LoadUserFile()
{


    return true;
}
//********************************************************************************************//
int CuserDB::SaveUserFile()
{

    USER_RECORD UserRecord = {0};

    int iWrite = 0;

    for (m_itUserMap = m_UserMap.begin(); m_itUserMap != m_UserMap.end(); ++m_itUserMap) {
        iWrite = write(m_ifd, &m_itUserMap->second, sizeof (UserRecord));
        if ( iWrite <= 0)
            break;
    }

    return iWrite;
}
//********************************************************************************************//
