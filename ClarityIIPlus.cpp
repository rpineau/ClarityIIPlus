//
//  ClarityIIPlus.cpp
//  CClarityIIPlus
//
//  Created by Rodolphe Pineau on 2021-04-13
//  ClarityIIPlus X2 plugin

#include "ClarityIIPlus.h"

CClarityIIPlus::CClarityIIPlus()
{
    // set some sane values
    m_bIsConnected = false;
    m_sDataFilePath.clear();
    
    m_nTempUnit = CEL;
    m_nWinSpeedUnit = KPH;
    m_dSkyTemp = -999;
    m_dTemp = -999;
    m_dSensorTemp = -999;
    m_dWindSpeed = 0;
    m_dPercentHumdity =0;
    m_dDewPointTemp = 0;
    m_nheaterPower = 0;
    m_nRainFlag = 0;
    m_nWetFlag = 0;
    m_nGoodDataSince = 0;
    m_nCloudCondition = 0;
    m_nWindCondition = 0;
    m_nRainCondition = 0;
    m_nDaylightCondition = 0;
    m_bNeedClose = false;
    m_bAlert = false;
    m_dSQM = 0;
    m_bHasSqm = false;

#ifdef PLUGIN_DEBUG
#if defined(SB_WIN_BUILD)
    m_sLogfilePath = getenv("HOMEDRIVE");
    m_sLogfilePath += getenv("HOMEPATH");
    m_sLogfilePath += "\\X2_ClarityIIPlus.txt";
    m_sPlatform = "Windows";
#elif defined(SB_LINUX_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/X2_ClarityIIPlus.txt";
    m_sPlatform = "Linux";
#elif defined(SB_MAC_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/X2_ClarityIIPlus.txt";
    m_sPlatform = "macOS";
#endif
    m_sLogFile.open(m_sLogfilePath, std::ios::out |std::ios::trunc);
#endif

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [CClarityIIPlus] Version " << std::fixed << std::setprecision(2) << PLUGIN_VERSION << " build " << __DATE__ << " " << __TIME__ << " on "<< m_sPlatform << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [CClarityIIPlus] Constructor Called." << std::endl;
    m_sLogFile.flush();
#endif
}

CClarityIIPlus::~CClarityIIPlus()
{
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [~CClarityIIPlus] Called." << std::endl;
    m_sLogFile.flush();
#endif

    if(m_bIsConnected) {
        Disconnect();
    }

#ifdef    PLUGIN_DEBUG
    // Close LogFile
    if(m_sLogFile.is_open())
        m_sLogFile.close();
#endif
}

int CClarityIIPlus::Connect()
{
    int nErr = SB_OK;
    std::string sDummy;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Called." << std::endl;
    m_sLogFile.flush();
#endif


    m_bIsConnected = true;
    if(m_sDataFilePath.size()) {
        nErr = getData();
        if (nErr) {
            m_bIsConnected = false;
            return ERR_COMMNOLINK;
        }
    }

    return nErr;
}


void CClarityIIPlus::Disconnect()
{
    if(m_bIsConnected) {
        m_bIsConnected = false;

#ifdef PLUGIN_DEBUG
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Disconnect] Disconnected." << std::endl;
        m_sLogFile.flush();
#endif
    }
}


void CClarityIIPlus::getFirmware(std::string &sFirmware)
{
    sFirmware.assign("V1.0");
}


#pragma mark - Getter / Setter
int CClarityIIPlus::getTempUnit()
{
    return m_nTempUnit;
}

int CClarityIIPlus::getWindSpeedUnit()
{
    return m_nWinSpeedUnit;
}


double CClarityIIPlus::getSkyTemp()
{
    return m_dSkyTemp;
}

double CClarityIIPlus::getAmbientTemp()
{
    return m_dTemp;
}

double CClarityIIPlus::getSensorTemp()
{
    return m_dSensorTemp;
}

double CClarityIIPlus::getWindSpeed()
{
    return m_dWindSpeed;
}

double CClarityIIPlus::getHumidity()
{
    return m_dPercentHumdity;
}

double CClarityIIPlus::getDewPointTemp()
{
    return m_dDewPointTemp;
}

int CClarityIIPlus::getHeaterPower()
{
    return m_nheaterPower;
}

int CClarityIIPlus::getRainFlag()
{
    return m_nRainFlag;
}

int CClarityIIPlus::getWetlag()
{
    return m_nWetFlag;
}

int CClarityIIPlus::getTimeSinceGoodData()
{
    return m_nGoodDataSince;
}

bool CClarityIIPlus::getNeedClose()
{
    return m_bNeedClose;
}

int CClarityIIPlus::getCloudCondition()
{
    return m_nCloudCondition;
}

int CClarityIIPlus::getWindCondition()
{
    return m_nWindCondition;
}

int CClarityIIPlus::getRainCondition()
{
    return m_nRainCondition;
}

int CClarityIIPlus::getLightCondition()
{
    return m_nDaylightCondition;
}

bool CClarityIIPlus::getAlert()
{
    return m_bAlert;
}

double CClarityIIPlus::getSQM()
{
    return m_dSQM;
}

bool CClarityIIPlus::isSqmAvailable()
{
    return m_bHasSqm;
}


int CClarityIIPlus::getData()
{
    int nErr = PLUGIN_OK;
    std::string sTmp;
    
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] Called." << std::endl;
    m_sLogFile.flush();
#endif

    m_sClarityIIData.clear();
    nErr =  readDataFile();
    if(nErr)
        return nErr;

    if(!m_sDataFilePath.size())
        return ERR_F_DOESNOTEXIST;
    
    if(!m_sClarityIIData.size())
        return ERR_CMDFAILED;
    
    // "New format" with SQM appended:
    // Date       Time        T V   SkyT   AmbT   SenT   Wind Hum  DewPt Hea R W Since Now()Day's   c w r d C A   SQM
    // 2005-06-03 02:07:23.34 C K  -28.5   18.7   22.5   45.3  75   10.3   3 0 0 00004 038506.08846 1 2 1 0 0 0 +21.88
    // 012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
    // 0         1         2         3         4         5         6         7         8         9        10        11
    m_nTempUnit = m_sClarityIIData.at(23)=='C'?CEL:FRH;
    m_nWinSpeedUnit = m_sClarityIIData.at(25)=='K'?KPH:MPH;
    m_dSkyTemp = std::stod(m_sClarityIIData.substr(27,6));
    m_dTemp = std::stod(m_sClarityIIData.substr(34,6));
    m_dSensorTemp = std::stod(m_sClarityIIData.substr(41,6));
    m_dWindSpeed = std::stod(m_sClarityIIData.substr(48,6));
    m_dPercentHumdity = std::stod(m_sClarityIIData.substr(55,3));
    m_dDewPointTemp = std::stod(m_sClarityIIData.substr(59,6));
    m_nheaterPower = std::stoi(m_sClarityIIData.substr(66,3));
    m_nRainFlag = int(m_sClarityIIData.at(70)) - 48; // 48->'0'
    m_nWetFlag = int(m_sClarityIIData.at(72)) - 48;
    m_nGoodDataSince = std::stoi(m_sClarityIIData.substr(74,5));
    m_nCloudCondition = int(m_sClarityIIData.at(93)) - 48;
    m_nWindCondition = int(m_sClarityIIData.at(95)) - 48;
    m_nRainCondition = int(m_sClarityIIData.at(97)) - 48;
    m_nDaylightCondition = int(m_sClarityIIData.at(99)) - 48;
    m_bNeedClose = m_sClarityIIData.at(101)=='1'?true:false;
    m_bAlert = m_sClarityIIData.at(103)=='1'?true:false;
    if(m_sClarityIIData.size()>103) {
        m_dSQM = std::stod(m_sClarityIIData.substr(105,6));
        m_bHasSqm = true;
    } else
        m_bHasSqm = false;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] m_nTempUnit            : " << m_nTempUnit << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] m_nWinSpeedUnit        : " << m_nWinSpeedUnit << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] m_dSkyTemp             : " << m_dSkyTemp << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] m_dTemp                : " << m_dTemp << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] m_dSensorTemp          : " << m_dSensorTemp << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] m_dWindSpeed           : " << m_dWindSpeed << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] m_dPercentHumdity      : " << m_dPercentHumdity << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] m_dDewPointTemp        : " << m_dDewPointTemp << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] m_nheaterPower         : " << m_nheaterPower << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] m_nRainFlag            : " << m_nRainFlag << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] m_nWetFlag             : " << m_nWetFlag << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] m_nGoodDataSince       : " << m_nGoodDataSince << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] m_nCloudCondition      : " << m_nCloudCondition << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] m_nWindCondition       : " << m_nWindCondition << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] m_nRainCondition       : " << m_nRainCondition << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] m_nDaylightCondition   : " << m_nDaylightCondition << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] m_bNeedClose           : " << (m_bNeedClose?"Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] m_bAlert               : " << (m_bAlert?"Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] m_bHasSqm              : " << (m_bAlert?"Yes":"No") << std::endl;
    if(m_bHasSqm)
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] m_dSQM                 : " << m_dSQM << std::endl;
    m_sLogFile.flush();
#endif

    return nErr;
}

int CClarityIIPlus::readDataFile()
{
    int nErr = PLUGIN_OK;
    std::string sTmp;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [readDataFile] readDataFile." << std::endl;
    m_sLogFile.flush();
#endif

    if(m_Datafile.is_open())
        m_Datafile.close();

    m_Datafile.open(m_sDataFilePath, std::ifstream::in);

    if(m_Datafile.is_open()) {
        m_Datafile.sync_with_stdio(true);
        while (std::getline(m_Datafile, sTmp)) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [readDataFile] sTmp : " << sTmp << std::endl;
    m_sLogFile.flush();
#endif

            if(sTmp.find("//")==0) // we ingore comment lines
                continue;
            m_sClarityIIData.assign(sTmp);
            break;
        }
        m_Datafile.close();
    }
    else
        return -1;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [readDataFile] file content : " << m_sClarityIIData << std::endl;
    m_sLogFile.flush();
#endif
    return nErr;
}

void CClarityIIPlus::getClarityIIDataFileName(std::string &fName)
{
    fName.assign(m_sDataFilePath);
}

void CClarityIIPlus::setClarityIIDataFileName(std::string fName)
{
    std::string sTmp;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setClarityIIDataFileName] fName : " << fName << std::endl;
    m_sLogFile.flush();
#endif
    if(!fName.size())
        return;
    fName = trim(fName,"\n\r ");
    if(fName.at(0) == '~') {
#if defined(SB_WIN_BUILD)
        sTmp = getenv("HOMEDRIVE");
        sTmp += getenv("HOMEPATH");
#elif defined(SB_LINUX_BUILD)
        sTmp = getenv("HOME");
#elif defined(SB_MAC_BUILD)
        sTmp = getenv("HOME");
#endif
        sTmp += fName.substr(1,fName.size());
        m_sDataFilePath.assign(sTmp);
    }
    else
        m_sDataFilePath.assign(fName);
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setClarityIIDataFileName] m_sDataFilePath : " << m_sDataFilePath << std::endl;
    m_sLogFile.flush();
#endif
}

std::string& CClarityIIPlus::trim(std::string &str, const std::string& filter )
{
    return ltrim(rtrim(str, filter), filter);
}

std::string& CClarityIIPlus::ltrim(std::string& str, const std::string& filter)
{
    str.erase(0, str.find_first_not_of(filter));
    return str;
}

std::string& CClarityIIPlus::rtrim(std::string& str, const std::string& filter)
{
    str.erase(str.find_last_not_of(filter) + 1);
    return str;
}

#ifdef PLUGIN_DEBUG
void CClarityIIPlus::log(const std::string sLogLine)
{
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [log] " << sLogLine << std::endl;
    m_sLogFile.flush();

}

const std::string CClarityIIPlus::getTimeStamp()
{
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    std::strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}
#endif

