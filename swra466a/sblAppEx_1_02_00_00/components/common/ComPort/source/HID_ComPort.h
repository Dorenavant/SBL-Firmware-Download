#ifndef __HID_COMPORT_H__
#define __HID_COMPORT_H__
#include "ComPort.h"
class COMPORTDLL_EXPORT HID_ComPort : public ComPort {
private:
	ComPortElement* m_pComPortList;
};
#endif //__HID_COMPORT_H__