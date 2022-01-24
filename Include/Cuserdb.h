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

#ifndef CUSERDB_H
#define CUSERDB_H

#include <sys/mman.h>
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

enum eUsers
{
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

typedef struct SUserRecord
{
  char szUserName[SIZE_USERNAME];
  char szPassword[SIZE_PASSWORD];
  int iGroupID; // in case you want to assign something to the whole group
  int iAccessLevel;

  bool bActive;

  char szLastLoginTime[SIZE_OF_DATE_TIME];
  char szLastLogoutTime[SIZE_OF_DATE_TIME];

  char szDateAccountCreated[SIZE_OF_FORMATED_DATE];
  char szDateAccountTerminated[SIZE_OF_FORMATED_DATE];

  char szLastMessage[MAX_PATH];

} USER_RECORD;

typedef unordered_map<string /*username*/, USER_RECORD> MapUserDB; // <Stock Symbol  Book Levels>

class CuserDB
{
private:
  MapUserDB m_UserMap;
  MapUserDB::iterator m_itUserMap;

  unordered_set<int> my_set;
  int m_ifd;
  int m_iError;
  int m_iSizeOfUserRecord;

public:
  string m_strLogMsg;

  CuserDB(char *szUserFileName);
  ~CuserDB();
  int LoadUserFile();
  int SaveUserFile();

  int AddUser(char *szUsername, char *szPassword, int iGroupID, int iAccessLevel);
  int ChangeUserPassword(char *szUsername, char *szPassword);

  int ModifyUser(char *szUsername, bool bActive, int iGroupID, int iAccessLevel);
  int DeleteUser(char *szUsername);
  int ListDB();

  int VerifyUser(char *szUsername, char *szPassword);

  int GetError();
};

#endif // CUSERDB_H
