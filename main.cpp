#include "sys/resource.h"
#include "main.h"

#include "ComLog.h"
#include "EPollServer.h"
#include <iostream>


//    ./QuantCom U  UserDB.qtx     	// to start the server in user Database mode
//    ./QuantCom  UserDB.qtx     	// to start the server in communication mode

// Test with this line in telnet for Login, spaces are part of the login message to pad for the full length of field  (without the last dot):
//L Amro        Amro        .
//L Mohamed     Mohamed     .
//L Heba        Lipo        .
//L Abdo        Abdo        .

using namespace std;

#define	    SIZE_NAME  15

////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc,char* argv[])
{
    // nfds is number of events (number of returned fd)

    EPOLL_CTOR_LIST SEpoll_Ctor;
    
    //  initialize the structure here to construct the server
    /**************************************************************/
    // You Modify it to read from an ini file 
    /***************************************************************/
    SEpoll_Ctor.iLoadFactor =  10;
    SEpoll_Ctor.nReadThreads = 100;
    SEpoll_Ctor.nWriteThreads = 100;
    SEpoll_Ctor.iNumOFileDescriptors = 1000;
    strcpy(SEpoll_Ctor.szServerPort, "9998");
    SEpoll_Ctor.iTimeOut = 1000;
//    SEpoll_Ctor.Local_addr = 127.0.0.1;
    SEpoll_Ctor.MaxByte = 1000;
    SEpoll_Ctor.MaxEvents = 1000;
    SEpoll_Ctor.Open_Max = 1000;
    //

    CComLog::instance().log("===========================================================================================================================", CComLog::Info);
    CComLog::instance().log("Starting EPOll Server", CComLog::Info);
    CComLog::instance().log("===========================================================================================================================", CComLog::Info);

    if (argc == 3) { // Look for 'U' for DB          //  <U  UserDB.qtx>  for example

        cout << "Running in user Database mode" << endl;;
        cout << "User Filename: " << argv[2];

        CuserDB* pCuserDB = nullptr;
        char szUserDBFile[MAX_PATH];

        memset(szUserDBFile, '\0', MAX_PATH);
        strcpy(szUserDBFile, argv[2]);
        pCuserDB = new CuserDB(szUserDBFile);

        if (!strncmp(argv[1], "U", 1)) {  // 'U' in upper case
            CComLog::instance().log("Starting Server for User Database Management", CComLog::Info);

            char szUserName[SIZE_NAME];
            char szPassword[SIZE_NAME];

            memset( szUserName, '\0', SIZE_NAME);
            memset( szPassword, '\0', SIZE_NAME);

            int bActive = 0, iGroupID = 0, iAccessLevel = 0;

            int iRet = 0;

            int iSelection = 5;
            while (iSelection != 0) {
                cout << endl;
                cout << "1  Add <username> <password>  <group ID>  <acess level> " << endl;
                cout << "2  Modify <username> <active>  <iGroupID>  <Access Level > " << endl;
                cout << "3  Change password <username> <new password>  " << endl;
                cout << "4  Delete <username> " << endl;
                cout << "5  List DB " << endl;
                cout << "0  Quit  " << endl;
                cout << endl<< endl;;

                cout << "Enter Selection:" ;
                cin.clear();
                cin >> iSelection;
                cout << iSelection  << endl;

                switch (iSelection) {
                case 1:
                    cout << "Add User" << endl;
                    cout << "Enter UserName  Password  GroupID  iAccessLevel" << endl;
                    cin >> szUserName >> szPassword >> iGroupID >> iAccessLevel;

                    iRet =  pCuserDB->AddUser(szUserName, szPassword, iGroupID, iAccessLevel);
                    if (iRet == USER_ALREADY_EXIST)
                        cout << "User Already Exist" << endl  << endl;
                    else
                        cout << "User Added "  << endl  << endl;
                    break;

                case 2:
                    cout << "Modify User" << endl;
                    cout << "Enter UserName  Active (0/1)  Group ID  Access Level " << endl;

                    cin >> szUserName >> bActive >> iGroupID >> iAccessLevel ;
                    pCuserDB->ModifyUser(szUserName, bActive, iGroupID, iAccessLevel);
                    break;

                case 3:
                    cout << "Change User Passwordr" << endl;
                    cout << "Enter UserName  Password " << endl;

                    cin >> szUserName >> szPassword ;
//		    cout << "Enter UserName  Password " << endl;
                    iRet = pCuserDB->ChangeUserPassword(szUserName, szPassword);
                    if(iRet == INVALID_USER_NAME)
                        cout << "Invalid Username" << endl<< endl;
                    else
                        cout << "Password changed "  << endl << endl;
                    break;


                case 4:
                    cout << "Delete User" << endl;
                    cout << "Enter UserName" << endl;

                    cin >> szUserName;
                    iRet = pCuserDB->DeleteUser(szUserName);
                    if (iRet == INVALID_USER_NAME)
                        cout << "Invalid User name" <<  endl << endl;
                    else
                        cout << "User Deleted " << endl << endl;
                    break;

                case 5:
                    cout << "Listing Users" << endl << endl;
                    iRet = pCuserDB->ListDB();
                    cout << endl << endl;
                    cout << to_string(iRet) << " Users" << endl;
                    break;

                default:
                    continue;
                    break;

                } // switch
                if (iSelection == 0) {
                    pCuserDB->SaveUserFile();
                    delete pCuserDB;

                    cout << " Successfull Termination";
                    exit(EXIT_SUCCESS);
                    break; // unreachable code
                } // if (iSelection == 0)
            }//      while (iSelection != 0)
        }//if (!strncmp(argv[1], "U", 1)) {  // 'U' in upper case
        if (pCuserDB) {
            delete pCuserDB;
            exit(EXIT_SUCCESS);
        }
    }  //    if (argc == 3 0)  // Look for "U" for user db mode

    // argc == 2           //   <./QuantCom UserDB.qtx>  for example
    if (!argv[1] ) {
        // ::TODO error message
        exit(EXIT_FAILURE);
    }

    strcpy(SEpoll_Ctor.szUserFileName, argv[1]);

    CEpollServer* pCEpoll = nullptr;
    pCEpoll = new CEpollServer(SEpoll_Ctor);
    
    if (!pCEpoll) {
        CComLog::instance().log("Failure Getting an instance of EPOll Server", CComLog::Error);
        exit(EXIT_FAILURE);
    }

    if (pCEpoll->GetError() == INVALID_USER_FILE_NAME) {  // can't continue
        CComLog::instance().log("Invalid User File Name", CComLog::Error);
	delete pCEpoll;
        exit(EXIT_FAILURE);
    }

    if (pCEpoll->GetError() > 100)
    {
        CComLog::instance().log("Error instanializing EPOll Server", CComLog::Error);
        delete pCEpoll;
        exit(EXIT_FAILURE);
    }

    pCEpoll->PrepListener();
    if (pCEpoll->GetError() > 100) {
        CComLog::instance().log("Failure to Listen EPOll Server", CComLog::Error);
        delete pCEpoll;
        exit(EXIT_FAILURE);
    }
    pCEpoll->ProcessEpoll();   // main driver loop here
    if (pCEpoll->GetError() > 100) {
        CComLog::instance().log("Failure to Process EPOll Server", CComLog::Error);
        delete pCEpoll;
        exit(EXIT_FAILURE);
    }

    TASK_QUEUE TQueue =   pCEpoll->GetQueueStatus();  // Monitor

//     pCEpoll->TerminateThreads();  // Called by the destructor

    delete pCEpoll;

    CComLog::instance().log("Success Termination of EPOll Server", CComLog::Info);

    exit(EXIT_SUCCESS);
}
////////////////////////////////////////////////////////////////////////////////////////////
