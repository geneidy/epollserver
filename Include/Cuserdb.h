/*
 * Copyright 2017 <copyright holder> <email>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef CUSERDB_H
#define CUSERDB_H

#include  <sys/mman.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>

using namespace std;

#include <string>
#include <string.h>
#include <unordered_map>
#include <unordered_set>
#include "ComLog.h"

#define SIZE_USERNAME 15
#define SIZE_PASSWORD 15

#define MAX_PATH 256


enum eUsers {
    INVALID_USER_FILE_NAME,
    VALID_USER_FILE_NAME,
    INVALID_LOGIN_MESSAGE,
    INVALID_USER_NAME,
    INVALID_PASSWORD,
    USER_ALREADY_EXIST,
    EXCEEDED_NUMBER_OF_CONNECTIONS,    
    USER_INACTIVE,
    VALID_USER
};


typedef struct SUserRecord {
    char szUserName[SIZE_USERNAME];
    char szPassword[SIZE_PASSWORD];
    int iGroupID;  // in case you want to assign something to the whole group
    int iAccessLevel;

    bool bActive;

    char  szLastLoginTime[SIZE_OF_DATE_TIME ];
    char  szLastLogoutTime[SIZE_OF_DATE_TIME ];

    char  szDateAccountCreated[SIZE_OF_FORMATED_DATE];
    char  szDateAccountTerminated[SIZE_OF_FORMATED_DATE];

    char  szLastMessage[MAX_PATH ];

} USER_RECORD;


typedef unordered_map<string /*username*/, USER_RECORD  > MapUserDB;  // <Stock Symbol  Book Levels>

class CuserDB 
{
private:
  MapUserDB m_UserMap;
  MapUserDB::iterator m_itUserMap;
  
  unordered_set <int> my_set;
  int m_ifd;
  int m_iError;
  int m_iSizeOfUserRecord;
  
public:
  
  string m_strLogMsg;
  
    CuserDB(char* szUserFileName);
    ~CuserDB();
    int LoadUserFile();
    int SaveUserFile();

    int AddUser(char* szUsername, char* szPassword, int iGroupID, int iAccessLevel);
    int ChangeUserPassword(char* szUsername, char* szPassword);
    
    int ModifyUser(char* szUsername, bool bActive, int iGroupID, int iAccessLevel);
    int DeleteUser(char* szUsername);
    int ListDB();                    
  
    int VerifyUser(char* szUsername, char* szPassword);
    
    int GetError();
    
};

#endif // CUSERDB_H
