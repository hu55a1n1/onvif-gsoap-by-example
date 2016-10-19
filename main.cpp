/**
 * ./ipconvif -cIp '192.168.1.53' -cUsr 'admin' -cPwd 'admin' -fIp '192.168.1.51:21' -fUsr 'ftpuser' -fPwd 'Ftpftp123
 *
 */

#include <iostream>
#include "stdio.h"
#include "wsdd.nsmap"
#include "plugin/wsseapi.h"
#include "plugin/wsaapi.h"
#include <openssl/rsa.h>
#include "ErrorLog.h"
 
#include "include/soapDeviceBindingProxy.h"
#include "include/soapMediaBindingProxy.h"
#include "include/soapPTZBindingProxy.h"

#include "include/soapPullPointSubscriptionBindingProxy.h"
#include "include/soapRemoteDiscoveryBindingProxy.h" 

#include <stdarg.h>  // For va_start, etc.
#include <memory>    // For std::unique_ptr
#include <ctime>
#include <sstream>	// For stringstream
#include "Snapshot.hpp"
#include <algorithm>

#define MAX_HOSTNAME_LEN 128
#define MAX_LOGMSG_LEN 256 


char* getCmdOption(char ** begin, char ** end, const std::string & option)
{
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option)
{
    return std::find(begin, end, option) != end;
}

void PrintErr(struct soap* _psoap)
{
	fflush(stdout);
	processEventLog(__FILE__, __LINE__, stdout, "error:%d faultstring:%s faultcode:%s faultsubcode:%s faultdetail:%s", _psoap->error, 
	*soap_faultstring(_psoap), *soap_faultcode(_psoap),*soap_faultsubcode(_psoap), *soap_faultdetail(_psoap));
}

int main(int argc, char* argv[])
{

	char szHostName[MAX_HOSTNAME_LEN] = { 0 };

	// Proxy declarations
	DeviceBindingProxy proxyDevice;
	RemoteDiscoveryBindingProxy proxyDiscovery; 
	MediaBindingProxy proxyMedia;

	if(!(  cmdOptionExists(argv, argv+argc, "-cIp")
			&& cmdOptionExists(argv, argv+argc, "-cUsr")
			&& cmdOptionExists(argv, argv+argc, "-cPwd")
#ifdef WITH_FTP_UPLOAD
            && cmdOptionExists(argv, argv+argc, "-fIp")
			&& cmdOptionExists(argv, argv+argc, "-fUsr")
			&& cmdOptionExists(argv, argv+argc, "-fPwd")
#endif
            ))
    {
    	std::cout  <<  "usage: ./ipconvif -cIp [<camera-ip>:<port>] -cUsr <cam-id> -cPwd <cam-pwd>\n";

#ifdef WITH_FTP_UPLOAD
    	std::cout  <<  "                  -fIp [<ftp-server-ip>:<port>] -fUsr <ftp-id> -fPwd <ftp-pwd>\n";
#endif

    	return -1;
    }

    char *camIp = getCmdOption(argv, argv+argc, "-cIp");
    char *camUsr = getCmdOption(argv, argv+argc, "-cUsr");
    char *camPwd = getCmdOption(argv, argv+argc, "-cPwd");

#ifdef WITH_FTP_UPLOAD
    char *ftpIp = getCmdOption(argv, argv+argc, "-fIp");
    char *ftpUsr = getCmdOption(argv, argv+argc, "-fUsr");
    char *ftpPwd = getCmdOption(argv, argv+argc, "-fPwd");
#endif

    if (!(camIp && camUsr && camPwd
#ifdef WITH_FTP_UPLOAD
    	   && ftpIp && ftpUsr && ftpPwd
#endif
        ))
    {
    	processEventLog(__FILE__, __LINE__, stdout, "Error: Invalid args (All args are required!)");
    	return -1;
    }

    strcat(szHostName, "http://");
	strcat(szHostName, camIp);
	strcat(szHostName, "/onvif/device_service");

	proxyDevice.soap_endpoint = szHostName;

	// Register plugins
	soap_register_plugin(proxyDevice.soap, soap_wsse);
	soap_register_plugin(proxyDiscovery.soap, soap_wsse);
	soap_register_plugin(proxyMedia.soap, soap_wsse);

	struct soap *soap = soap_new();
	// For DeviceBindingProxy
	if (SOAP_OK != soap_wsse_add_UsernameTokenDigest(proxyDevice.soap, NULL, camUsr, camPwd))
	{
		return -1;
	}

	if (SOAP_OK != soap_wsse_add_Timestamp(proxyDevice.soap, "Time", 10)) 
	{
		return -1;
	}
    
    // Get Device info
    _tds__GetDeviceInformation *tds__GetDeviceInformation = soap_new__tds__GetDeviceInformation(soap, -1);
    _tds__GetDeviceInformationResponse *tds__GetDeviceInformationResponse = soap_new__tds__GetDeviceInformationResponse(soap, -1);
    
    if (SOAP_OK == proxyDevice.GetDeviceInformation(tds__GetDeviceInformation, tds__GetDeviceInformationResponse))
    {
        processEventLog(__FILE__, __LINE__, stdout, "-------------------DeviceInformation-------------------");
        processEventLog(__FILE__, __LINE__, stdout, "Manufacturer:%sModel:%s\r\nFirmwareVersion:%s\r\nSerialNumber:%s\r\nHardwareId:%s", tds__GetDeviceInformationResponse->Manufacturer.c_str(),
                        tds__GetDeviceInformationResponse->Model.c_str(), tds__GetDeviceInformationResponse->FirmwareVersion.c_str(),
                        tds__GetDeviceInformationResponse->SerialNumber.c_str(), tds__GetDeviceInformationResponse->HardwareId.c_str());
    }
    else
    {
        PrintErr(proxyDevice.soap);
        return -1;
    }
    
    // DeviceBindingProxy ends
    soap_destroy(soap); 
    soap_end(soap); 

    // Get Device capabilities
	_tds__GetCapabilities *tds__GetCapabilities = soap_new__tds__GetCapabilities(soap, -1);
	tds__GetCapabilities->Category.push_back(tt__CapabilityCategory__All);
	_tds__GetCapabilitiesResponse *tds__GetCapabilitiesResponse = soap_new__tds__GetCapabilitiesResponse(soap, -1);

	if (SOAP_OK == proxyDevice.GetCapabilities(tds__GetCapabilities, tds__GetCapabilitiesResponse))
	{

		if (tds__GetCapabilitiesResponse->Capabilities->Media != NULL)
		{
			processEventLog(__FILE__, __LINE__, stdout, "-------------------Media-------------------");
			processEventLog(__FILE__, __LINE__, stdout, "XAddr:%s", tds__GetCapabilitiesResponse->Capabilities->Media->XAddr.c_str());

			processEventLog(__FILE__, __LINE__, stdout, "-------------------streaming-------------------");
			processEventLog(__FILE__, __LINE__, stdout, "RTPMulticast:%s", (tds__GetCapabilitiesResponse->Capabilities->Media->StreamingCapabilities->RTPMulticast) ? "Y" : "N");
			processEventLog(__FILE__, __LINE__, stdout, "RTP_TCP:%s", (tds__GetCapabilitiesResponse->Capabilities->Media->StreamingCapabilities->RTP_USCORETCP) ? "Y" : "N");
			processEventLog(__FILE__, __LINE__, stdout, "RTP_RTSP_TCP:%s", (tds__GetCapabilitiesResponse->Capabilities->Media->StreamingCapabilities->RTP_USCORERTSP_USCORETCP) ? "Y" : "N");

			proxyMedia.soap_endpoint = tds__GetCapabilitiesResponse->Capabilities->Media->XAddr.c_str();

			// TODO: Add check to see if device is capable of 'GetSnapshotUri', if not skip this device!
		}
	}

	// For MediaBindingProxy
    if (SOAP_OK != soap_wsse_add_UsernameTokenDigest(proxyMedia.soap, NULL, camUsr, camPwd))
	{
		return -1;
	}

	if (SOAP_OK != soap_wsse_add_Timestamp(proxyMedia.soap, "Time", 10)) 
	{
		return -1;
	}

	// Get Device Profiles
	_trt__GetProfiles *trt__GetProfiles = soap_new__trt__GetProfiles(soap, -1);
	_trt__GetProfilesResponse *trt__GetProfilesResponse = soap_new__trt__GetProfilesResponse(soap, -1);


	if (SOAP_OK == proxyMedia.GetProfiles(trt__GetProfiles, trt__GetProfilesResponse))
	{
        _trt__GetSnapshotUriResponse *trt__GetSnapshotUriResponse = soap_new__trt__GetSnapshotUriResponse(soap, -1);
        _trt__GetSnapshotUri *trt__GetSnapshotUri = soap_new__trt__GetSnapshotUri(soap, -1);
        
        // processEventLog(__FILE__, __LINE__, stdout, "-------------------ONE SNAPSHOT PER PROFILE-------------------");
        
        // Loop for every profile
        for (int i = 0; i < trt__GetProfilesResponse->Profiles.size(); i++)
        {
            trt__GetSnapshotUri->ProfileToken = trt__GetProfilesResponse->Profiles[i]->token;
            
            if (SOAP_OK != soap_wsse_add_UsernameTokenDigest(proxyMedia.soap, NULL, camUsr, camPwd))
            {
                return -1;
            }
            
            // Get Snapshot URI for profile
            if (SOAP_OK == proxyMedia.GetSnapshotUri(trt__GetSnapshotUri, trt__GetSnapshotUriResponse))
            {
                processEventLog(__FILE__, __LINE__, stdout, "Profile Name- %s", trt__GetProfilesResponse->Profiles[i]->Name.c_str());
                processEventLog(__FILE__, __LINE__, stdout, "SNAPSHOT URI- %s", trt__GetSnapshotUriResponse->MediaUri->Uri.c_str());
                
                // Create a 'Snapshot' instance
                Snapshot *snapshot = new Snapshot("./downloads", "1.jpg");
                
                // Download snapshot file locally
                CURLcode result = snapshot->download(trt__GetSnapshotUriResponse->MediaUri->Uri);

#ifdef WITH_FTP_UPLOAD
                if(CURLE_OK == result) {
                    std::string upload = snapshot->getUploadUri(gateName, ftpIp, ftpUsr, ftpPwd);
                    processEventLog(__FILE__, __LINE__, stdout, "Uploading using: %s", upload.c_str());
                    system(upload.c_str());
                    sleep(5);
                }
#endif

                // Delete current 'Snapshot' instance
                // delete snapshot;

                // Break out of the media profiles loop (now that we have one snapshot uploaded),
                // so we only take one Snapshot per camera.
                break;
            }
            else
            {
                PrintErr(proxyMedia.soap);
            }
        }
	}
	else
	{
		PrintErr(proxyMedia.soap);
	}

	// MediaBindingProxy ends
	soap_destroy(soap); 
	soap_end(soap); 

	return 0;
}

