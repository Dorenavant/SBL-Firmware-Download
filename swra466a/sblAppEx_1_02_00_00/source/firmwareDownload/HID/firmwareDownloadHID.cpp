#include "sbllibUART.h"
#include "ComPortElement.h"

#include <vector>
#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <stdlib.h>

#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>


using namespace std;


double diff_ms(timeval t1, timeval t2)
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

    while (ulByteCount--)
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
    fprintf(stderr, "\r%d%% ", progress);
    fflush(stderr);
}

// Time variables for calculating execution time.
static timeval tBefore, tAfter;

// Start millisecond timer
static void getTime(void)
{
    gettimeofday(&tBefore, NULL);
}

// Print time since getTime()
static void printTimeDelta(void)
{
    gettimeofday(&tAfter, NULL);
    printf("(%.2fms)\n", diff_ms(tAfter, tBefore));
}

// Checks to see if the given string is hex
bool isHexString(string s)
{
    return s.length() > 2
            && s[0] == '0'
            && s[1] == 'x'
            && (s.find_first_not_of("0123456789abcdefABCDEF", 2) == string::npos)
            && !(s.length() % 2);
}


// Defines
#define DEVICE_CC2538               0x2538
#define DEVICE_CC26XX               0x2650
#define CC2538_FLASH_BASE           0x00200000
#define CC26XX_FLASH_BASE           0x00000000

// Application main function
int main(int argc, char* argv[])
{
    //
    // START: Program Configuration
    //
    /* Device type. (Binary-coded decimal of the device
         e.g. 0x2538 for CC2538 and 0x2650 for CC2650) */
    uint32_t deviceType = DEVICE_CC26XX;

    /* UART baud rate. Default: 460800 */
    uint32_t baudRate = 460800;

    //
    // END: Program configuration
    //

    SblDevice *pDevice = NULL;     // Pointer to SblDevice object
    int32_t devIdx = 0;            // 
    ComPortElement* pElements;     // An array for the COM ports that will be found by enumeration.
    int32_t nElem = 10;            // Sets the number of COM ports to list by SBL.
    int32_t devStatus = -1;        // Hold SBL status codes
    uint32_t byteCount = 0;        // File size in bytes
    uint32_t fileCrc, devCrc;      // Variables to save CRC checksum
    uint32_t devFlashBase;         // Flash start address
    uint32_t readLength = 0;       // How many bytes to read
    static std::vector<char> pvWrite(1);// Vector to application firmware in.
    static std::ifstream file;     // File stream
    std::string filePath;          // File path to program
    bool bEnableXosc = false;      // Should SBL try to enable XOSC? (Not possible for CC26xx)
    bool idxSelected = false;      // Was index inputted in command line
    bool filePathInputted = false; // Was a file path specified
    bool readSelected = false;     // Whether we're in read mode
    bool writeSelected = false;    // Whether we're in write mode
    bool findSelected = false;     // Whether we're in find mode
    bool silentModeSelected = false;  // Whether or not to output raw data to stdout
    bool listPorts = false;        // Whether or not to list ports to user
    std::string addressInput;      // Inputted address to read from
    std::string writeInput;        // Bytes to write to the device
    std::string searchInput;       // Inputted bytes to search for

    devFlashBase = CC26XX_FLASH_BASE;

    //
    // Enumerate COM ports
    //
    pDevice = SblDevice::Create(deviceType);
    pDevice->enumerate(pElements, nElem);

    if(nElem == 0) 
    { 
        cout << "No COM ports detected.\n"; 
        //cout <<  "+-------------------------------------------------------------\n\n";
        goto exit;
    }

    opterr = 1;
    int c;
    while ((c = getopt(argc, argv, "p::l::h::r::n::w::f::s::")) != -1)
    {
        switch (c)
        {
            case 'p':
                if (!optarg)
                {
                    cout << "Option -p requires an argument!" << endl;
                    goto exit;
                }
                devIdx = strtol(optarg, NULL, 0);
                idxSelected = true;
                break;
            case 'l':
                listPorts = true;
                break;
            case 'r':
                if (!optarg)
                {
                    cout << "Option -r requires an argument!" << endl;
                    goto exit;
                }
                readSelected = true;
                addressInput = optarg;
                break;
            case 'n':
                if (!optarg)
                {
                    cout << "Option -n requires an argument!" << endl;
                    goto exit;
                }
                readLength = strtol(optarg, NULL, 0);
                break;
            case 'w':
                if (!optarg)
                {
                    cout << "Option -w requires an argument!" << endl;
                    goto exit;
                }
                writeSelected = true;
                addressInput = optarg;
                break;
            case 'f':
                if (!optarg)
                {
                    cout << "Option -f requires an argument!" << endl;
                    goto exit;
                }
                findSelected = true;
                searchInput = optarg;
                break;
            case 's':
                silentModeSelected = true;
                break;
            case '?':
                if (optopt == 'p' || optopt == 'r' || optopt == 'n' || optopt == 'w' || optopt == 'f')
                    cout << "Option -" << optopt << " requires an argument" << endl;
                goto exit;
            default:
                cout << "This app will download firmware in the form of a bin file to the chip on the CC2650\n"
                     << "You can choose to write a file to the whole chip, read from a certain address, or write to a certain address\n"
                     << "Note: the number of bytes entered must be a multiple of four"
                     << "Usage:\n"
                     << "\tsudo ./firmwareDownloadUART [options] <bytes to write> OR <file path>\n"
                     << "Options:\n"
                     << "\t-h\tShow this screen\n"
                     << "\t-p\tSelect port number [default: 0]\n"
                     << "\t-l\tEnumerate connected devices\n"
                     << "\t-r\tRead from given address\n\t\t\t(Enter address as int or hex with '0x' in front)\n"
                     << "\t-w\tWrite to given address\n\t\t\t(Enter address as int or hex with '0x' in front)\n"
                     << "\t-n\tNumber of bytes to read [1 - 4096]\n"
                     << "\t-f\tSearch for string of bytes\n"
                     << "\t-s\tSilent mode: will only print error messages"
                     << endl;
                goto exit;
        }
    }

    if (!silentModeSelected)
    {
        //
        // Print out header
        //
        cout << "+--------------------------------------------------------------------+\n";
        cout << "| Serial Bootloader Library Firmware Download Application for CC" 
            << (deviceType >> 12 & 0xf) << (deviceType >> 8 & 0xf) << (deviceType >> 4 & 0xf) << (deviceType & 0xf) << " |\n";
        cout << "+--------------------------------------------------------------------+\n";
    }

    if (listPorts)
    {
        if (silentModeSelected) cout << "+--------------------------------------------------------------------+\n";
        printf("%-069s|\n", "| COM ports:");
        cout << "+--------------------------------------------------------------------+\n";
        printf("%-066s|\n", "|Idx\t| Description");
        for(int32_t i = 0; i < nElem; i++)
        {
            printf("|%2d\t| %s\n", i, pElements[i].description);
        }
        cout << "+--------------------------------------------------------------------+\n";
    }

    if (!((readSelected || readLength) && silentModeSelected))
    {
        //
        // Set callback functions
        //
        SblDevice::setCallBackProgressFunction(&appProgress);
        SblDevice::setCallBackStatusFunction(&appStatus);
    }


    if (optind < argc)
    {
        if (writeSelected)
        {
            writeInput = argv[optind];
        }
        else
        {
            filePath.assign(argv[optind]);
            filePathInputted = true;
        }
    }

    if (!idxSelected)
    {
        if (nElem == 1)
        {
            devIdx = 0;
        }
        else
        {
            //
            // Wait for user to select COM port
            //
            cout << "Select COM port index: ";
            cin >> devIdx;
    
        }
    }

    if(devIdx < 0 || devIdx >= nElem)
    {
        cout << "Port index out of bounds." << endl;
        goto error;
    }

    //
    // Create SBL object
    //
    if(!pDevice) 
    { 
        cout << "No SBL device object.\n";
        goto error;
    }

    if (readSelected && !readLength)
    {
        cout << "Please enter the number of bytes to read: ";
        cin >> readLength;
    }

    if (readLength && !readSelected)
    {
        cout << "Please enter a memory address to read from: ";
        cin >> addressInput;
    }

    if (writeSelected && writeInput.empty())
    {
        cout << "Please enter the bytes to write to this address (put '0x' in front for hex): ";
        cin >> writeInput;
    }

    if (readSelected && (readLength > 4096 || readLength < 1))
    {
        cout << "Read Length must be between 1 and 4096" << endl;
        goto error;
    }

    if (!readSelected && !writeSelected && !findSelected)
    {
        if (!filePathInputted)
        {
            //
            //  User input file path
            //
            cout << "Please enter the file path: ";
            cin >> filePath;
        }

        //
        // Read file
        //
        file.open(filePath.c_str(), std::ios::binary);
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
            cout << "Unable to open file path: " << filePath.c_str();
            goto error;
        }
    }

    //
    // Connect to device
    //
    if (!silentModeSelected)
    {
        printf("\nConnecting (%s @ %d baud) ...\n", pElements[devIdx].portNumber, baudRate);
        getTime();
    }
    if (pDevice->connect(pElements[devIdx].portNumber, baudRate, bEnableXosc) != SBL_SUCCESS) 
    {
        goto error;
    }
    if (!silentModeSelected) printTimeDelta();

    if (readSelected || writeSelected)
    {
        uint32_t realAddress = (isHexString(addressInput)) ? strtol(addressInput.c_str(), NULL, 16) : atoi(addressInput.c_str());

        if (readSelected)
        {
            if (!silentModeSelected) cout << "Reading data ..." << endl;    
            char* pcData = new char[readLength];
            
            if (!silentModeSelected) getTime();
            if (pDevice->readMemory8(realAddress, readLength, pcData) != SBL_SUCCESS)
            {
                cout << "Error reading from firmware." << endl;
                goto error;
            }
            if (!silentModeSelected) printTimeDelta();

            if (!silentModeSelected) printf("\n\n%-08s\tData", "Address");
            for (int i = 0; i < readLength; i++)
            {
                if (!(i % 16))
                {
                    if (!silentModeSelected) printf("\n0x%08x\t", realAddress + i);
                }
                printf("%02x", pcData[i]);
            }
            cout << endl;
            fflush(stdout);
        }

        if (writeSelected)
        {
            // Check to see if hex input and convert if so
            string bytesToWrite;
            if (isHexString(writeInput))
            {
                for (int i = 2; i < writeInput.length(); i += 2)
                {
                    string subStr = writeInput;
                    subStr = subStr.substr(i, 2);
                    char c = (char) (int)strtol(subStr.c_str(), NULL, 16);
                    bytesToWrite.push_back(c);
                }
            }
            else
            {
                bytesToWrite = writeInput;
            }

            if (!silentModeSelected)
            {
                cout << "Writing data ..." << endl;
                getTime();
            }

            if (pDevice->writeFlashRangeAutoErase(realAddress, bytesToWrite.length(), bytesToWrite.c_str()) != SBL_SUCCESS)
            {
                cout << "Error writing to device." << endl;
                goto error;
            }
            if (!silentModeSelected) printTimeDelta();
        }

        if (!silentModeSelected) cout << "\n\nResetting device ..." << endl;
        if(pDevice->reset() != SBL_SUCCESS)
            cout << "Error resetting device.  Please press the reset button on the PI HAT." << endl;
        else if (!silentModeSelected) cout << "OK" << endl;
        goto exit;
    }
    else if (findSelected)
    {
        // Check to see if hex input and convert if so
        string bytesToSearch;
        if (isHexString(searchInput))
        {
            for (int i = 2; i < searchInput.length(); i += 2)
            {
                string subStr = searchInput;
                subStr = subStr.substr(i, 2);
                char c = (char) (int)strtol(subStr.c_str(), NULL, 16);
                bytesToSearch.push_back(c);
            }
        }
        else
        {
            bytesToSearch = searchInput;
        }

        if (!silentModeSelected) 
        {
            cout << "Searching for bytes ..." << endl;
            getTime();
        }

        vector<uint32_t> pvAddresses;
        if (pDevice->findBytes(bytesToSearch.length(), bytesToSearch.c_str(), pvAddresses) != SBL_SUCCESS)
        {
            cout << "Error finding bytes." << endl;
            goto error;
        }
        if (!silentModeSelected) printTimeDelta();

        if (pvAddresses.empty())
        {
            cout << "\nUnable to find bytes '" << ((isHexString(searchInput)) ? searchInput.substr(2) : searchInput) << "'.\n";
        }
        else
        {
            if (!silentModeSelected) 
                cout << "\nBytes '" << ((isHexString(searchInput)) ? searchInput.substr(2) : searchInput) << "' were found at: " << endl;
            for (vector<uint32_t>::const_iterator i = pvAddresses.begin(); i != pvAddresses.end(); i++)
            {
                printf("0x%08x\n", *i);
            }
            fflush(stdout);
        }
        
        if (!silentModeSelected) cout << "\n\nResetting device ..." << endl;
        if(pDevice->reset() != SBL_SUCCESS)
            cout << "Error resetting device.  Please press the reset button on the PI HAT." << endl;
        else if (!silentModeSelected) cout << "OK" << endl;
        goto exit;
    }

    //
    // Calculate file CRC checksum
    //
    fileCrc = calcCrcLikeChip((unsigned char *)&pvWrite[0], byteCount);

    //
    // Erasing as much flash needed to program firmware.
    //
    if (!silentModeSelected)
    {
        cout << "Erasing flash ...\n";
        getTime();
    }
    if (pDevice->eraseFlashRange(devFlashBase, byteCount) != SBL_SUCCESS)
    {
        goto error;
    }
    if (!silentModeSelected) printTimeDelta();

    //
    // Writing file to device flash memory.
    //
    if (!silentModeSelected)
    {
        cout << "Writing flash ...\n";
        getTime();
    } 
    if (pDevice->writeFlashRange(devFlashBase, byteCount, &pvWrite[0]) != SBL_SUCCESS)
    {
        goto error;
    }
    if (!silentModeSelected) printTimeDelta();

    //
    // Calculate CRC checksum of flashed content.
    //
    if (!silentModeSelected)
    {
        cout << "Calculating CRC on device ...\n";
        getTime();
    }
    if (pDevice->calculateCrc32(devFlashBase, byteCount, &devCrc) != SBL_SUCCESS)
    {
        goto error;

    }
    printTimeDelta();

    //
    // Compare CRC checksums
    //
    if (!silentModeSelected) cout << "Comparing CRC ...\n";
    if (fileCrc == devCrc)
    {
        if (!silentModeSelected) printf("OK\n");
    }
    else printf("CRC Mismatch!\n");

    if (!silentModeSelected) cout << "Resetting device ...\n";
    if(pDevice->reset() != SBL_SUCCESS) goto error;
    if (!silentModeSelected) cout << "OK\n";

    //
    // If we got here, all succeeded. Jump to exit.
    //
    goto exit;

error:
    cout << "\n\nError: ";
    if (pDevice) cout << pDevice->getLastStatus() << endl;
    else cout << SBL_ERROR << endl;
exit:
    devStatus = 0;
    if(pDevice) {
        devStatus = pDevice->getLastStatus();
        delete pDevice;
    }
    return devStatus;
}
