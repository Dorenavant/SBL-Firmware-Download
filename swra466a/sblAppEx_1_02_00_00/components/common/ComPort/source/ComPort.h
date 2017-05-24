#ifndef __COMPORT_H__
#define __COMPORT_H__
//*****************************************************************************
// Copyright (c) 2013 - 2015 Texas Instruments Incorporated
//
// All rights reserved not granted herein.
//
// Limited License.  
//
// Texas Instruments Incorporated grants a world-wide, royalty-free, 
// non-exclusive license under copyrights and patents it now or hereafter owns 
// or controls to make, have made, use, import, offer to sell and sell 
// ("Utilize") this software subject to the terms herein.  With respect to the 
// foregoing patent license, such license is granted  solely to the extent that 
// any such patent is necessary to Utilize the software alone.  The patent 
// license shall not apply to any combinations which include this software, 
// other than combinations with devices manufactured by or for TI 
// ("TI Devices").  No hardware patent is licensed hereunder.
//
// Redistributions must preserve existing copyright notices and reproduce this 
// license (including the above copyright notice and the disclaimer and (if 
// applicable) source code license limitations below) in the documentation 
// and/or other materials provided with the distribution
//
// Redistribution and use in binary form, without modification, are permitted 
// provided that the following conditions are met:
//
// * No reverse engineering, decompilation, or disassembly of this software is 
//   permitted with respect to any software provided in binary form.
//
// * any redistribution and use are licensed by TI for use only with TI Devices.
//
// * Nothing shall obligate TI to provide you with source code for the software 
//   licensed and provided to you in object code.
//
// If software source code is provided to you, modification and redistribution 
// of the source code are permitted provided that the following conditions 
// are met:
//
// * any redistribution and use of the source code, including any resulting 
//   derivative works, are licensed by TI for use only with TI Devices.
//
// * any redistribution and use of any object code compiled from the source 
//   code and any resulting derivative works, are licensed by TI for use only 
//   with TI Devices.
//
// Neither the name of Texas Instruments Incorporated nor the names of its 
// suppliers may be used to endorse or promote products derived from this 
// software without specific prior written permission.
//
// DISCLAIMER.
//
// THIS SOFTWARE IS PROVIDED BY TI AND TI'S LICENSORS "AS IS" AND ANY EXPRESS 
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN 
// NO EVENT SHALL TI AND TIâ€™S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
// OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//*****************************************************************************

#include <string>
#include "ComPortElement.h"
#if defined(_WIN32) || defined(_WIN64)
#include <wbemidl.h>
#endif

// DLL exports and import directives
#if defined(WIN32) || defined(WIN64)
    #if defined(COMPORTDLL_EXPORTS)
        #define COMPORTDLL_EXPORT __declspec(dllexport)
    #else
        #define COMPORTDLL_EXPORT __declspec(dllimport)
    #endif
#else
    #define COMPORTDLL_EXPORT
#endif

class COMPORTDLL_EXPORT ComPort
{
public:

    COMPORTDLL_EXPORT enum 
    {
        COMPORT_ERROR,
        COMPORT_SUCCESS
    };

    COMPORTDLL_EXPORT enum 
    { 
        UART_BUFFER_SIZE = 4096
    };

    ComPort();
    ~ComPort();

    int enumerate(ComPortElement* pComPortList, int& numElements);
    int open(std::string csPortNumber, int baudRate, int rdTimeoutMs = 100, int wrTimeoutMs = 200);
    int close();
    int readBytes(void *pData, int length);
    int writeBytes(void *pData, int length);
    int flushBuffers();

    int getBaudRate() { return m_baudRate; }
    bool isInitiated() { return m_bInitiated; }

protected:
	/// Handle to the COM-port device.
	void* m_hComDev;
    /// Variable to save error number returned by CreateFile()
    //DWORD m_error;
	/// The DCB structure defines the control setting for a serial communications device
	//DCB m_dcb;
    /// Variable to tell if COM port has been initiated.
    bool m_bInitiated;
    /// The maximum length returned by \c GetUartData
    int m_maxDataLength;
    /// The current baudrate.
    int m_baudRate;

private:
#if defined(_WIN32) || defined(_WIN64)
    int init(const std::string csPortName, unsigned int baudRate = 9600,  int maxDataLength = 255, int readTimeoutMs = 100, int writeTimeoutMs = 200);

    static int enumWmi(ComPortElement* pComPortList, int& numElements);
    static int enumSetupapi(ComPortElement* pComPortList, int& numElements);
    static std::string convertBSTRToStdString(BSTR bstr);
    static std::string convertWCSToMBS(const wchar_t* pstr, long wslen);
    static void convertWCSToChar(const wchar_t *pIn, char *pOut);

#elif defined LINUX
    int getPortNumber(std::string strDesc, std::string& strPort);
#endif

};

#endif // __COMPORT_H__
