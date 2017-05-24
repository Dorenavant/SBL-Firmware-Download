#ifndef __COMPORTTHREAD_H__
#define __COMPORTTHREAD_H__
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
// ("TI Devices"ù).  No hardware patent is licensed hereunder.
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
// NO EVENT SHALL TI AND TI‚ÄôS LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
// OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//*****************************************************************************

#include "windows.h"

/// Start Of Packet
#define CPT_SOP 0xFE

/// Only used for debuggin. UART data simulated.
//#define DEBUG

/** \brief Read COM port Thread
 *
 * This class is will provide methods to read data from the COM port
 * in its own thread.
 * Calling to the run() function will start the thread and stop() will end it.
 *
 */
class ComPortThread
{

public:
	ComPortThread();
    virtual ~ComPortThread();
    bool Init(std::string csPortName, unsigned int baudRate = 9600,  int maxDataLength = 255);
    void Stop();
    bool initiated() { return bInitiated; }
    int WriteCOMMBytes(void * pData, int length);
    int getPacket(QByteArray **pPacket);

    enum{HEADER_SIZE = 4, UART_BUFFER_SIZE = 4096};

signals:
    void packetReady(void);


protected:
	// Methods
	void run();
    bool Close();
    bool GetUartData(int *pLength, BYTE *pData);
    bool PopUartBuffer(int *pLength, BYTE *pData);
    bool SearchForSop(void);
    bool CrcValid(unsigned char *pData, int length);
    void TrimUartBuffer();
    int ReadCOMMBytes(void *pData, int length);

	/// Local data buffer (FIFO, trimmed upon uart access)
    typedef struct {
        unsigned char pData[2 * UART_BUFFER_SIZE];    ///< Data buffer
        int inPos;                                    ///< Head position
        int outPos;                                   ///< Tail position
        int bufferedCount;                            ///< The number of bytes currently in the buffer
    } UART_BUFFER;

	/// Local data buffer
    UART_BUFFER uartBuffer;

	/// Handle to the COM-port device.
	HANDLE m_hComDev;
    /// Variable to save error number returned by CreateFile()
    DWORD m_error;
	/// The DCB structure defines the control setting for a serial communications device
	DCB m_dcb; 
    /// The maximum length returned by \c GetUartData
    int m_maxDataLength;
    /// Variable to tell if COM port has been initiated.
    bool bInitiated;
    /// Variable used to stop the loop in the run() function.
    volatile bool stopped;
    /// packet FIFO queue where all the packets will be stored.
    QQueue<QByteArray *> packets;
    /// Mutex to protect the packet Queue to make it thread safe.
    QMutex mutex;


};
#endif // __COMPORTTHREAD_H__