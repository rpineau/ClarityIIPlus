//
//  ClarityIIPlus.h
//  CClarityIIPlus
//
//  Created by Rodolphe Pineau on 2021-04-13
//  ClarityIIPlus X2 plugin

#ifndef __ClarityIIPlus__
#define __ClarityIIPlus__
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <memory.h>

#ifdef SB_MAC_BUILD
#include <unistd.h>
#endif

#ifdef SB_WIN_BUILD
#include <time.h>
#endif

#include <math.h>
#include <string.h>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <thread>
#include <ctime>
#include <cmath>
#include <future>
#include <mutex>


#include "../../licensedinterfaces/sberrorx.h"

#include "StopWatch.h"


#define PLUGIN_VERSION      1.0

// #define PLUGIN_DEBUG 3

#define FILE_CHECK_INTERVAL 10  // in seconds
#define inHg_to_mBar  33.86389

// error codes
enum ClarityIIPlusErrors {PLUGIN_OK=0, NOT_CONNECTED, CANT_CONNECT, BAD_CMD_RESPONSE, COMMAND_FAILED, COMMAND_TIMEOUT, PARSE_FAILED};

enum ClarityIIPlusWindUnits {KPH=0, MPS, MPH};
enum ClarityIIPlusTempUnits {CEL=0, FRH, KEL};

class CClarityIIPlus
{
public:
    CClarityIIPlus();
    ~CClarityIIPlus();

    int         Connect();
    void        Disconnect(void);
    bool        IsConnected(void) { return m_bIsConnected; }
    void        getFirmware(std::string &sFirmware);


    int         getData();

    void getClarityIIDataFileName(std::string &fName);
    void setClarityIIDataFileName(std::string fName);

    int     getTempUnit();
    int     getWindSpeedUnit();
    double  getSkyTemp();
    double  getAmbientTemp();
    double  getSensorTemp();
    double  getWindSpeed();
    double  getHumidity();
    double  getDewPointTemp();
    int     getHeaterPower();
    int     getRainFlag();
    int     getWetlag();
    int     getTimeSinceGoodData();

    int     getCloudCondition();
    int     getWindCondition();
    int     getRainCondition();
    int     getLightCondition();
    bool    getNeedClose();
    bool    getAlert();
    double  getSQM();
    
    bool    isSqmAvailable();

#ifdef PLUGIN_DEBUG
    void  log(const std::string sLogLine);
#endif

protected:

    int             readDataFile();

    bool            m_bIsConnected;

    // ClarityIIPlus variables
    // "New format" with SQM appended:
    // Date       Time        T V   SkyT   AmbT   SenT   Wind Hum  DewPt Hea R W Since Now()Day's   c w r d C A   SQM
    // 2005-06-03 02:07:23.34 C K  -28.5   18.7   22.5   45.3  75   10.3   3 0 0 00004 038506.08846 1 2 1 0 0 0 +21.88
    // 012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
    // 0         1         2         3         4         5         6         7         8         9        10        11

    int                 m_nTempUnit;
    int                 m_nWinSpeedUnit;
    std::atomic<double> m_dSkyTemp;
    std::atomic<double> m_dTemp;
    std::atomic<double> m_dSensorTemp;
    std::atomic<double> m_dWindSpeed;
    std::atomic<double> m_dPercentHumdity;
    std::atomic<double> m_dDewPointTemp;
    std::atomic<int>    m_nheaterPower;
    
    std::atomic<int>    m_nRainFlag;
    std::atomic<int>    m_nWetFlag;
    std::atomic<int>    m_nGoodDataSince;
    
    // std::atomic<double> m_dBarometricPressure;
    std::atomic<int>    m_nCloudCondition;
    std::atomic<int>    m_nWindCondition;
    std::atomic<int>    m_nRainCondition;
    std::atomic<int>    m_nDaylightCondition;
    std::atomic<bool>   m_bNeedClose;
    std::atomic<bool>   m_bAlert;
    std::atomic<double> m_dSQM;

    std::atomic<bool>   m_bSafe;
    std::atomic<bool>   m_bHasSqm;
    
    std::string     m_sDataFilePath;
    std::ifstream   m_Datafile;
    std::string     m_sClarityIIData;
    std::string&    trim(std::string &str, const std::string &filter );
    std::string&    ltrim(std::string &str, const std::string &filter);
    std::string&    rtrim(std::string &str, const std::string &filter);

    
#ifdef PLUGIN_DEBUG
    // timestamp for logs
    const std::string getTimeStamp();
    std::ofstream m_sLogFile;
    std::string m_sLogfilePath;
    std::string m_sPlatform;
#endif

};

#endif
