
#include "x2weatherstation.h"

X2WeatherStation::X2WeatherStation(const char* pszDisplayName,
												const int& nInstanceIndex,
												SerXInterface						* pSerXIn,
												TheSkyXFacadeForDriversInterface	* pTheSkyXIn,
												SleeperInterface					* pSleeperIn,
												BasicIniUtilInterface				* pIniUtilIn,
												LoggerInterface						* pLoggerIn,
												MutexInterface						* pIOMutexIn,
												TickCountInterface					* pTickCountIn)

{
	m_pSerX							= pSerXIn;
	m_pTheSkyXForMounts				= pTheSkyXIn;
	m_pSleeper						= pSleeperIn;
	m_pIniUtil						= pIniUtilIn;
	m_pLogger						= pLoggerIn;
	m_pIOMutex						= pIOMutexIn;
	m_pTickCount					= pTickCountIn;
    m_nPrivateISIndex               = nInstanceIndex;

    char szFname[2048];
    std::string sFname;
    
	m_bLinked = false;
    m_bUiEnabled = false;

    if (m_pIniUtil) {
        m_pIniUtil->readString(PARENT_KEY, CHILD_KEY_FNAME, "", szFname, 2048);
        sFname.assign(szFname);
        m_ClarityIIPlus.setClarityIIDataFileName(sFname);
        m_dSqmThreshold = m_pIniUtil->readDouble(PARENT_KEY, CHILD_KEY_SQM, 13);
    }
    m_DataTimer.Reset();
}

X2WeatherStation::~X2WeatherStation()
{
	//Delete objects used through composition
	if (GetSerX())
		delete GetSerX();
	if (GetTheSkyXFacadeForDrivers())
		delete GetTheSkyXFacadeForDrivers();
	if (GetSleeper())
		delete GetSleeper();
	if (GetSimpleIniUtil())
		delete GetSimpleIniUtil();
	if (GetLogger())
		delete GetLogger();
	if (GetMutex())
		delete GetMutex();
}

int	X2WeatherStation::queryAbstraction(const char* pszName, void** ppVal)
{
	*ppVal = NULL;

	if (!strcmp(pszName, LinkInterface_Name))
		*ppVal = (LinkInterface*)this;
	else if (!strcmp(pszName, WeatherStationDataInterface_Name))
        *ppVal = dynamic_cast<WeatherStationDataInterface*>(this);
    else if (!strcmp(pszName, ModalSettingsDialogInterface_Name))
        *ppVal = dynamic_cast<ModalSettingsDialogInterface*>(this);
    else if (!strcmp(pszName, X2GUIEventInterface_Name))
        *ppVal = dynamic_cast<X2GUIEventInterface*>(this);


	return SB_OK;
}

int X2WeatherStation::execModalSettingsDialog()
{
    int nErr = SB_OK;
    X2ModalUIUtil uiutil(this, GetTheSkyXFacadeForDrivers());
    X2GUIInterface*                    ui = uiutil.X2UI();
    X2GUIExchangeInterface*            dx = NULL;//Comes after ui is loaded
    bool bPressedOK = false;
    std::stringstream ssTmp;
    std::string sTmp;
    char szTmpBuf[4096];
    
    m_bUiEnabled = false;

    if (NULL == ui)
        return ERR_POINTER;
    if ((nErr = ui->loadUserInterface("ClarityIIPlus.ui", deviceType(), m_nPrivateISIndex))) {
        return nErr;
    }

    if (NULL == (dx = uiutil.X2DX())) {
        return ERR_POINTER;
    }
    X2MutexLocker ml(GetMutex());

    dx->setText("fileCheckStatus","");
    m_ClarityIIPlus.getClarityIIDataFileName(sTmp);
    dx->setText("filePath", sTmp.c_str());
    if(sTmp.size())
        m_ClarityIIPlus.getData();

    dx->setEnabled("sqmThreshold", m_ClarityIIPlus.isSqmAvailable());

    updateUI(dx);
    dx->setPropertyDouble("sqmThreshold", "value", m_dSqmThreshold);
    
    m_bUiEnabled = true;

    //Display the user interface
    if ((nErr = ui->exec(bPressedOK)))
        return nErr;

    //Retreive values from the user interface
    if (bPressedOK) {
        dx->propertyString("filePath", "text", szTmpBuf, 4095);
        sTmp.assign(szTmpBuf);
        m_ClarityIIPlus.setClarityIIDataFileName(sTmp);
        m_pIniUtil->writeString(PARENT_KEY, CHILD_KEY_FNAME, sTmp.c_str());
        dx->propertyDouble("sqmThreshold", "value", m_dSqmThreshold);
        m_pIniUtil->writeDouble(PARENT_KEY, CHILD_KEY_SQM, m_dSqmThreshold);
    }
    return nErr;
}

void X2WeatherStation::uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent)
{
    int nErr = SB_OK;
    std::stringstream ssTmp;
    std::string sTmp;
    char szTmpBuf[4096];
    
    // the test for m_bUiEnabled is done because even if the UI is not displayed we get events on the comboBox changes when we fill it.
    if(!m_bUiEnabled)
        return;


    if (!strcmp(pszEvent, "on_timer")) {
        m_ClarityIIPlus.getData();
        updateUI(uiex);
    }
    else if (!strcmp(pszEvent, "on_pushButton_clicked")) {
        uiex->propertyString("filePath", "text", szTmpBuf, 4095);
        sTmp.assign(szTmpBuf);
        m_ClarityIIPlus.setClarityIIDataFileName(sTmp);
        nErr = m_ClarityIIPlus.getData();
        if (nErr != PLUGIN_OK) {
            ssTmp << "Read file error : " << nErr;
            uiex->messageBox("Error reading file", ssTmp.str().c_str());
            uiex->setText("fileCheckStatus","<html><head/><body><p><span style=\" color:#FF0000;\">File can't be read or contains bad data</span></p></body></html>");
            return;
        }
        uiex->setEnabled("sqmThreshold", m_ClarityIIPlus.isSqmAvailable());
        updateUI(uiex);
        m_ClarityIIPlus.getClarityIIDataFileName(sTmp);
        uiex->setText("filePath", sTmp.c_str());
        uiex->setText("fileCheckStatus","<html><head/><body><p><span style=\" color:#00FF00;\">File check successful</span></p></body></html>");
    }
}

void X2WeatherStation::updateUI(X2GUIExchangeInterface* dx)
{
    std::stringstream ssTmp;
    double dTmp;
    int nTmp;

    dTmp = m_ClarityIIPlus.getSkyTemp();
    ssTmp << std::fixed << std::setprecision(2) << dTmp << "ºC";
    dx->setText("skyTemp", ssTmp.str().c_str());
    std::stringstream().swap(ssTmp);

    dTmp = m_ClarityIIPlus.getAmbientTemp();
    ssTmp << std::fixed << std::setprecision(2) << dTmp << "ºC";
    dx->setText("ambientTemp", ssTmp.str().c_str());
    std::stringstream().swap(ssTmp);

    dTmp = m_ClarityIIPlus.getSensorTemp();
    ssTmp << std::fixed << std::setprecision(2) << dTmp << "ºC";
    dx->setText("sensorTemp", ssTmp.str().c_str());
    std::stringstream().swap(ssTmp);

    dTmp = m_ClarityIIPlus.getWindSpeed();
    nTmp = m_ClarityIIPlus.getWindSpeedUnit();
    std::stringstream().swap(ssTmp);
    ssTmp << std::fixed << std::setprecision(2) << dTmp << (nTmp==KPH?" Km/h":" MPH");
    dx->setText("windSpeed", ssTmp.str().c_str());
    std::stringstream().swap(ssTmp);

    dTmp = m_ClarityIIPlus.getHumidity();
    ssTmp << std::fixed << std::setprecision(2) << dTmp << "%";
    dx->setText("humidity", ssTmp.str().c_str());
    std::stringstream().swap(ssTmp);

    dTmp = m_ClarityIIPlus.getDewPointTemp();
    ssTmp << std::fixed << std::setprecision(2) << dTmp << "ºC";
    dx->setText("dewPoint", ssTmp.str().c_str());
    std::stringstream().swap(ssTmp);

    if(m_ClarityIIPlus.isSqmAvailable()) {
        dTmp = m_ClarityIIPlus.getSQM();
        ssTmp << std::fixed << std::setprecision(2) << dTmp << " mpsas";
        dx->setText("SQM", ssTmp.str().c_str());
    }
    else {
        dx->setText("SQM", "N/A");

    }
}

void X2WeatherStation::driverInfoDetailedInfo(BasicStringInterface& str) const
{
    str = "ClarityII Plus X2 plugin for Boltwood and AAG-CloudWatcher enhanced Clarity II file by Rodolphe Pineau";
}

double X2WeatherStation::driverInfoVersion(void) const
{
	return PLUGIN_VERSION;
}

void X2WeatherStation::deviceInfoNameShort(BasicStringInterface& str) const
{
    str = "ClarityII Plus";
}

void X2WeatherStation::deviceInfoNameLong(BasicStringInterface& str) const
{
    deviceInfoNameShort(str);
}

void X2WeatherStation::deviceInfoDetailedDescription(BasicStringInterface& str) const
{
    deviceInfoNameShort(str);
}

void X2WeatherStation::deviceInfoFirmwareVersion(BasicStringInterface& str)
{
    if(m_bLinked) {
        str = "N/A";
        std::string sFirmware;
        X2MutexLocker ml(GetMutex());
        m_ClarityIIPlus.getFirmware(sFirmware);
        str = sFirmware.c_str();
    }
    else
        str = "N/A";

}

void X2WeatherStation::deviceInfoModel(BasicStringInterface& str)
{
    deviceInfoNameShort(str);
}

int	X2WeatherStation::establishLink(void)
{
    int nErr = SB_OK;

    X2MutexLocker ml(GetMutex());
    nErr = m_ClarityIIPlus.Connect();
    if(nErr)
        m_bLinked = false;
    else
        m_bLinked = true;

	return nErr;
}
int	X2WeatherStation::terminateLink(void)
{
    m_ClarityIIPlus.Disconnect();

	m_bLinked = false;
	return SB_OK;
}


bool X2WeatherStation::isLinked(void) const
{
	return m_bLinked;
}


int X2WeatherStation::weatherStationData(double& dSkyTemp,
                                         double& dAmbTemp,
                                         double& dSenT,
                                         double& dWind,
                                         int& nPercentHumdity,
                                         double& dDewPointTemp,
                                         int& nRainHeaterPercentPower,
                                         int& nRainFlag,
                                         int& nWetFlag,
                                         int& nSecondsSinceGoodData,
                                         double& dVBNow,
                                         double& dBarometricPressure,
                                         WeatherStationDataInterface::x2CloudCond& cloudCondition,
                                         WeatherStationDataInterface::x2WindCond& windCondition,
                                         WeatherStationDataInterface::x2RainCond& rainCondition,
                                         WeatherStationDataInterface::x2DayCond& daylightCondition,
                                         int& nRoofCloseThisCycle //The weather station hardware determined close or not (boltwood hardware says cloudy is not close)
)
{
    int nErr = SB_OK;
    int dWindCond;
    int nTempUnit;
    double dSqm;
    
    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());
    if(m_DataTimer.GetElapsedSeconds() > 5 ) {
        m_ClarityIIPlus.getData();
        m_DataTimer.Reset();
    }
    
    nSecondsSinceGoodData = m_ClarityIIPlus.getTimeSinceGoodData();
    dSkyTemp = m_ClarityIIPlus.getSkyTemp();
    dAmbTemp = m_ClarityIIPlus.getAmbientTemp();
    nTempUnit = m_ClarityIIPlus.getTempUnit();
    if(nTempUnit == FRH) {
        // convert to C
        dSkyTemp = (dSkyTemp-32) / 1.8;
        dAmbTemp = (dAmbTemp-32) / 1.8;
    }
    else if(nTempUnit == KEL) {
        dSkyTemp = dSkyTemp-273.15;
        dAmbTemp = dAmbTemp-273.15;
    }
    
    dSenT = m_ClarityIIPlus.getSensorTemp();
    dWind = m_ClarityIIPlus.getWindSpeed();
	nPercentHumdity = int(m_ClarityIIPlus.getHumidity());
	dDewPointTemp = m_ClarityIIPlus.getDewPointTemp();
    nRainHeaterPercentPower = m_ClarityIIPlus.getHeaterPower();
	nRainFlag = m_ClarityIIPlus.getRainFlag();
	nWetFlag = m_ClarityIIPlus.getWetlag();

    dWindCond = m_ClarityIIPlus.getWindCondition();

    cloudCondition = (WeatherStationDataInterface::x2CloudCond)m_ClarityIIPlus.getCloudCondition();
    windCondition = (WeatherStationDataInterface::x2WindCond)m_ClarityIIPlus.getWindCondition();
    rainCondition = (WeatherStationDataInterface::x2RainCond)m_ClarityIIPlus.getRainCondition();
    if(m_ClarityIIPlus.isSqmAvailable()) {
        dSqm = m_ClarityIIPlus.getSQM();
        if(dSqm >= m_dSqmThreshold) {
            daylightCondition = WeatherStationDataInterface::x2DayCond::dayDark;
        }
        else {
            daylightCondition = WeatherStationDataInterface::x2DayCond::dayLight;
        }
    }
    else
        daylightCondition = (WeatherStationDataInterface::x2DayCond)m_ClarityIIPlus.getLightCondition();

    nRoofCloseThisCycle = m_ClarityIIPlus.getNeedClose();
    
    
	return nErr;
}

WeatherStationDataInterface::x2WindSpeedUnit X2WeatherStation::windSpeedUnit()
{
    WeatherStationDataInterface::x2WindSpeedUnit nUnit = WeatherStationDataInterface::x2WindSpeedUnit::windSpeedKph;
    int ClarityIIPlusUnit;
    std::stringstream tmp;

    ClarityIIPlusUnit = m_ClarityIIPlus.getWindSpeedUnit();

    switch(ClarityIIPlusUnit) {
        case KPH:
            nUnit = WeatherStationDataInterface::x2WindSpeedUnit::windSpeedKph;
            break;
        case MPS:
            nUnit = WeatherStationDataInterface::x2WindSpeedUnit::windSpeedMps;
            break;
        case MPH:
            nUnit = WeatherStationDataInterface::x2WindSpeedUnit::windSpeedMph;
            break;
    }

    return nUnit ;
}
