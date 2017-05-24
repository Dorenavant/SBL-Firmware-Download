/******************************************************************************
*  Filename:       sblAppEx.cpp
*  Revised:        $Date: 2014-10-09 14:34:40 +0200 (to, 09 okt 2014) $
*  Revision:       $Revision: 32696 $
*
*  Description:    Serial Bootloader Library application example.
*                  This example enumerates all COM devices and lets you
*                  select which port to connect to. The example assumes the
*                  connected device is a CC2538 or a CC2650 and programs a blinky
*                  onto the device. After programming the blinky, bootloader
*                  mode may be forced by holding the SELECT button on 06EB
*                  when resetting the chip.
*
*  Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/
*
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/
#include <sbllib.h>
#include <ComPortElement.h>

#include <vector>
#include <iostream>
#include <fstream>

//
// Including functions for millisecond precision timing.
// Taken from http://stackoverflow.com/questions/2494356/how-to-use-gettimeofday-or-something-equivalent-with-visual-studio-c-2008
//
#include <time.h>
#include <windows.h>
const __int64 DELTA_EPOCH_IN_MICROSECS= 11644473600000000;

using namespace std;

struct timeval2 {
    __int32    tv_sec;          /* seconds */
    __int32    tv_usec;         /* microseconds */
};

int gettimeofday(struct timeval2 *tv)
{
    FILETIME ft;
    __int64 tmpres = 0;
    TIME_ZONE_INFORMATION tz_winapi;
    int rez = 0;

    ZeroMemory(&ft,sizeof(ft));
    ZeroMemory(&tz_winapi,sizeof(tz_winapi));

    GetSystemTimeAsFileTime(&ft);

    tmpres = ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;

    /*converting file time to unix epoch*/
    tmpres /= 10;  /*convert into microseconds*/
    tmpres -= DELTA_EPOCH_IN_MICROSECS; 
    tv->tv_sec = (__int32)(tmpres*0.000001);
    tv->tv_usec =(tmpres%1000000);

    return 0;
}

double diff_ms(timeval2 t1, timeval2 t2)
{
    return ((((t1.tv_sec - t2.tv_sec) * 1000000) + (t1.tv_usec - t2.tv_usec)) + 500)/1000;
}


// Calculate crc32 checksum the way CC2538 and CC2650 does it.
int calcCrcLikeChip(const unsigned char *pData, unsigned long ulByteCount)
{
    unsigned long d, ind;
    unsigned long acc = 0xFFFFFFFF;
    const unsigned long ulCrcRand32Lut[] =
    {
        0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC, 
        0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C, 
        0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C, 
        0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C
    };

    while ( ulByteCount-- )
    {
        d = *pData++;
        ind = (acc & 0x0F) ^ (d & 0x0F);
        acc = (acc >> 4) ^ ulCrcRand32Lut[ind];
        ind = (acc & 0x0F) ^ (d >> 4);
        acc = (acc >> 4) ^ ulCrcRand32Lut[ind];
    }

    return (acc ^ 0xFFFFFFFF);
}

/// Application status function (used as SBL status callback)
void appStatus(char *pcText, bool bError)
{
    if(bError)
    {
        cerr << pcText;
    }
    else
    {
        cout << pcText;
    }
}


/// Application progress function (used as SBL progress callback)
static void appProgress(uint32_t progress)
{
    fprintf(stdout, "\r%d%% ", progress);
    fflush(stdout);
}

// Time variables for calculating execution time.
static timeval2 tBefore, tAfter;

// Start millisecond timer
static void getTime(void)
{
	gettimeofday(&tBefore);
}

// Print time since getTime()
static void printTimeDelta(void)
{
	gettimeofday(&tAfter);
    printf("(%.2fms)\n", diff_ms(tAfter, tBefore));
}

// Defines
#define DEVICE_CC2538				0x2538
#define DEVICE_CC26XX				0x2650
#define CC2538_FLASH_BASE			0x00200000
#define CC26XX_FLASH_BASE			0x00000000

// Application main function
int main()
{
	//
	// START: Program Configuration
	//
	/* Device type. (Binary-coded decimal of the device
	     e.g. 0x2538 for CC2538 and 0x2650 for CC2650) */
	uint32_t deviceType = 0x2650;

	/* UART baud rate. Default: 230400 */
    uint32_t baudRate = 230400;

	//
	// END: Program configuration
	//

	SblDevice *pDevice = NULL;		// Pointer to SblDevice object
	int32_t devIdx;					// 
    ComPortElement pElements[10];	// An array for the COM ports that will be found by enumeration.
    int32_t nElem = 10;				// Sets the number of COM ports to list by SBL.
    int32_t devStatus = -1;			// Hold SBL status codes
	std::string fileName;			// File name to program
	uint32_t byteCount = 0;			// File size in bytes
    uint32_t fileCrc, devCrc;		// Variables to save CRC checksum
	uint32_t devFlashBase;	    	// Flash start address
	static std::vector<char> pvWrite(1);// Vector to application firmware in.
	static std::ifstream file;		// File stream


	//
	// Set variables based on deviceType
	//
	if(deviceType == DEVICE_CC2538)
	{
		fileName = "blinky_backdoor_select_btn2538.bin";
		devFlashBase = CC2538_FLASH_BASE;
	}
	else
	{
		fileName = "blinky_backdoor_select_btn2650.bin";
		devFlashBase = CC26XX_FLASH_BASE;
	}

    //
    // Set callback functions
    //
    SblDevice::setCallBackStatusFunction(&appStatus);
    SblDevice::setCallBackProgressFunction(&appProgress);


    //
    // Print out header
    //
    cout << "+-------------------------------------------------------------\n";
    cout << "| Serial Bootloader Library Example Application for CC" << (deviceType >> 12 & 0xf) << (deviceType >> 8 & 0xf) << (deviceType >> 4 & 0xf) << (deviceType & 0xf) << "\n";
    cout << "+-------------------------------------------------------------\n\n";

    //
    // Enumerate and list COM ports
    //
    cout << "+-------------------------------------------------------------\n";
    cout << "| COM ports:\n";
    cout << "+-------------------------------------------------------------\n";
    cout << "|Idx\t| Description\n";
    SblDevice::enumerate(pElements, nElem);
    for(int32_t i = 0; i < nElem; i++)
    {
        printf("|%2d\t| %s\n", i, pElements[i].description);
    }
    if(nElem == 0) 
    { 
        cout << "| No COM ports detected.\n"; 
        cout <<  "+-------------------------------------------------------------\n\n";
        goto exit;
    }
    cout <<  "+-------------------------------------------------------------\n\n";

    //
    // Wait for user to select COM port
    //
    cout << "Select COM port index: ";
    cin >> devIdx;
    if(devIdx < 0 || devIdx >= nElem)
    {
        cout << "Index out of bounds.\n";
        goto error;
    }

    //
    // Should SBL try to enable XOSC? (Not possible for CC26xx)
    //
	bool bEnableXosc = false;
	if(deviceType == 0x2538)
	{
		char answer[64];
		cout << "Enable device CC2538 XOSC? (Y/N): ";
		cin >> answer;
		bEnableXosc = (answer[0] == 'Y' || answer[0] == 'y') ? true : false;
	}

    //
    // Create SBL object
    //
    pDevice = SblDevice::Create(deviceType);
    if(pDevice == NULL) 
    { 
        cout << "No SBL device object.\n";
        goto error;
    }

    //
    // Connect to device
    //
    printf("\nConnecting (%s @ %d baud) ...\n", pElements[devIdx].portNumber, baudRate);
	getTime();
    if(pDevice->connect(pElements[devIdx].portNumber, baudRate, bEnableXosc) != SBL_SUCCESS) 
    { 
        goto error; 
    }
	printTimeDelta();
	
    //
    // Read file
    //
	file.open(fileName.c_str(), std::ios::binary);
    if(file.is_open())
    {
        //
        // Get file size:
        //
        file.seekg(0, std::ios::end);
        byteCount = (uint32_t)file.tellg();
        file.seekg(0, std::ios::beg);

        //
        // Read data
        //
        pvWrite.resize(byteCount);
        file.read((char*) &pvWrite[0], byteCount);
    }
    else   
    {
		cout << "Unable to open file " << fileName.c_str();
        goto error;
    }

    //
    // Calculate file CRC checksum
    //
    fileCrc = calcCrcLikeChip((unsigned char *)&pvWrite[0], byteCount);

	//
	// Erasing as much flash needed to program firmware.
	//
    cout << "Erasing flash ...\n";
    getTime();
    if(pDevice->eraseFlashRange(devFlashBase, byteCount) != SBL_SUCCESS)
    {
        goto error;
    }
    printTimeDelta();

	//
	// Writing file to device flash memory.
	//
    cout << "Writing flash ...\n";
    getTime();
    if(pDevice->writeFlashRange(devFlashBase, byteCount, &pvWrite[0]) != SBL_SUCCESS)
    {
        goto error;
    }
    printTimeDelta();

	//
	// Calculate CRC checksum of flashed content.
	//
    cout << "Calculating CRC on device ...\n";
    getTime();
    if(pDevice->calculateCrc32(devFlashBase, byteCount, &devCrc) != SBL_SUCCESS)
    {
        goto error;

    }
    printTimeDelta();

	//
	// Compare CRC checksums
	//
    cout << "Comparing CRC ...\n";
    if(fileCrc == devCrc) printf("OK\n");
    else printf("Mismatch!\n");

    cout << "Resetting device ...\n";
    if(pDevice->reset() != SBL_SUCCESS)
    {
        goto error;
    }
    cout << "OK\n";

    //
    // If we got here, all succeeded. Jump to exit.
    //
    goto exit;

error:
    cout << "\n\nAn error occurred: " << pDevice->getLastStatus();
exit:
	devStatus = 0;
	if(pDevice != NULL) {
		devStatus = pDevice->getLastStatus();
	}
	
	return devStatus;
}
