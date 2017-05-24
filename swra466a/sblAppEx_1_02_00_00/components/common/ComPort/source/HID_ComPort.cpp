#include "ComPort.h"
#include <cp2110.h>
class HID_ComPort : public ComPort {
public:
	HID_ComPort::HID_ComPort() {
        m_hComDev = NULL;
        m_pComPortList = NULL;
        //m_error = 0;
        m_bInitiated = false;
        m_maxDataLength = 0;
        m_baudRate = 0;
	}
	HID_ComPort::HID_ComPort() {
        if (m_hComDev) { CP2110_release(m_hComDev); }
        if (m_pComPortList) { delete[] m_pComPortList; }
        //m_error = 0;
        m_bInitiated = false;
        m_maxDataLength = -1;
        m_baudRate = -1;
	}

	// Should return 0 if failure and 1 if success
    int enumerate(ComPortElement*& pComPortList, int& numElements) {
        hid_device_info *devs = CP2110_enumerate();
        if (m_pComPortList) {
            delete[] m_pComPortList;
        }
        m_pComPortList = new ComPortElement[numElements];
    	int i = 0;
        while (devs) {
            m_pComPortList[i].portNumber = i+1; 
            m_pComPortList[i].description = devs->serial_number;
            if (m_pComPortList[i].internal) {
                hid_free_enumeration(m_pComPortList[i].internal);
            }
            m_pComPortList[i].internal = devs;
            devs = devs->next;
            i++;
        }
        pComPortList = m_pComPortList;
        numElements = i+1;
    	return 1;
    }

    int open(std::string csPortNumber, int baudRate, int rdTimeoutMs = 100, int wrTimeoutMs = 200) {    
        if (m_hComDev = CP2110_init(m_pComPortList[csPortNumber].internal->serial_number) &&
                CP2110_setUARTConfig(m_hComDev, baudRate, PARITY_NONE,FLOW_CONTROL_DISABLED,
                 DATA_BITS_8, STOP_BITS_SHORT) >= 0) {
            m_bInitiated = true;
            m_baudRate = baudRate;
            return 1;
        }
        return 0;
    }

    int close() {
        if (m_hComDev) {
            CP2110_release(m_hComDev);
            isInitiated = false;
        }
        return 1;
    }

    int readBytes(void *pData, int length) {
        return CP2110_read(m_hComDev, pData, length);
    }

    int writeBytes(void *pData, int length) {
        return CP2110_write(m_hComDev, pData, length);
    }

    int flushBuffers() {
        return CP2110_purgeFIFO(m_hComDev, FIFO_BOTH);
    }

    int getBaudRate() { return m_baudRate; }

    bool isInitiated() { return m_bInitiated; }

private:
    int getPortNumber(std::string strDesc, std::string& strPort) {

    }
};