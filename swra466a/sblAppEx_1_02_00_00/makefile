HEAD	= ../..
CFLAGS += -I/usr/include/libusb-1.0/
CFLAGS += -I$(HEAD)/Dependencies/include/
CFLAGS += -I$(HEAD)/Dependencies/include/HID/
CFLAGS += -I$(HEAD)/Dependencies/include/UART/
CFLAGS += -Iinclude/HID/
CFLAGS += -Iinclude/UART/
CFLAGS += -Iinclude/
CFLAGS += -Wno-write-strings

LDLIBS += -lusb-1.0 -lpthread -lpigpiod_if2 -lrt

PWD=$(shell pwd)

HIDINCLDIR  := $(wildcard $(HEAD)/Dependencies/include/HID/*.h) include/HID/*.h
UARTINCLDIR := $(wildcard $(HEAD)/Dependencies/include/UART/*.h) include/UART/*.h

HIDCORESRCDIR  := source/firmwareDownload/HID
UARTCORESRCDIR := source/firmwareDownload/UART

HIDSBLSRCDIR := source/serial_bootloader_library/HID
UARTSBLSRCDIR := source/serial_bootloader_library/UART

HIDLIBSRCDIR  := $(HEAD)/Dependencies/source/HID
UARTLIBSRCDIR := $(HEAD)/Dependencies/source/UART

HIDCORECPP := $(wildcard $(HIDCORESRCDIR)/*.cpp) $(wildcard $(HIDSBLSRCDIR)/*.cpp)
UARTCORECPP := $(wildcard $(UARTCORESRCDIR)/*.cpp) $(wildcard $(UARTSBLSRCDIR)/*.cpp)

HIDLIBCPP := $(wildcard $(HIDLIBSRCDIR)/*.cpp)
HIDLIBC   := $(wildcard $(HIDLIBSRCDIR)/*.c) $(wildcard $(HIDLIBSRCDIR)/backup/*.c)
#$(HEAD)/../cp2110-c/src/cp2110.c $(HEAD)/../hidapi/libusb/hid.c

UARTLIBCPP := $(wildcard $(UARTLIBSRCDIR)/*.cpp) 
UARTLIBC   := $(wildcard $(UARTLIBSRCDIR)/*.c) 

HIDOBJDIR  := .hidobj
UARTOBJDIR := .uartobj

HIDCOREOBJ  := $(HIDCORECPP:$(HIDOBJDIR)/%.cpp=.o) $(HIDCOREC:$(HIDOBJDIR)/%.c=.o)
UARTCOREOBJ := $(UARTCORECPP:$(UARTOBJDIR)/%.cpp=.o) $(UARTCOREC:$(UARTOBJDIR)/%.c=.o)

HIDLIBOBJ  := $(HIDLIBCPP:$(HIDOBJDIR)/%.cpp=.o) $(HIDLIBC:$(HIDOBJDIR)/%.c=.o)
UARTLIBOBJ := $(UARTLIBCPP:$(UARTOBJDIR)/%.cpp=.o) $(UARTLIBC:$(UARTOBJDIR)/%.c=.o)

HIDDEPS := $(HIDCOREOBJ:.o=.d) $(HIDLIBOBJ:.o=.d)

all: bin/firmwareDownloadHID bin/firmwareDownloadUART
hidonly: bin/firmwareDownloadHID
uartonly: bin/firmwareDownloadUART
	
bin/firmwareDownloadHID: $(HIDCOREOBJ) $(HIDLIBOBJ) $(HIDINCLDIR)
	@echo "Compiling HID …"
	@$(CXX) $(CFLAGS) $(LDFLAGS) $(LDLIBS) $(HIDCOREOBJ) $(HIDLIBOBJ) include/HID/sbllib.h -o $@    
	@echo Complete

bin/firmwareDownloadUART: $(UARTCOREOBJ) $(UARTLIBOBJ) $(UARTINCLDIR)
	@echo "Compiling UART …"
	@$(CXX) $(CFLAGS) $(LDFLAGS) $(LDLIBS) $(UARTCOREOBJ) $(UARTLIBOBJ) include/UART/sbllibUART.h -o $@
	@echo Complete

install:
	@echo "Nothing to install here!"
	
