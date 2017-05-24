#! /usr/bin/env python
import argparse
import subprocess
import time
import sys
import datetime

# Enum for SBL Functions
class SBLFunction:
	READ = 0
	WRITE = 1
	FIND = 2
	REWRITE_FLASH = 3

# Hardcoded beginning of BT Address (in chip these are the last 3 bytes of the BT address reversed)
CONST_BTA_HEADER = "b0b448"
# Hardcoded length of BT Address
CONST_BTA_SIZE = 12
# Hardcoded number of flash pages in CC2640 chip
CONST_NUM_FLASH_PAGES = 32

# Helper function for calling SBL
def subprocess_call(strOpen, functionSelect):
	if bListPorts: strOpen += " -l"
	cmd = strOpen.split()
	if functionSelect == SBLFunction.REWRITE_FLASH:
		subprocess.call(cmd)
		quit()

	firmwareOutput = subprocess.Popen(cmd, stdout=subprocess.PIPE)
	strReadStdout = firmwareOutput.stdout.read()

	# Check for timeout i.e. sending autobaud failed to get response (usually means jumpers aren't attached to Pi Hat)
	if strReadStdout.find("Timed") != -1:
		print "Error connecting to Pi Hat.  Ensure that you have jumpers on D6 to Pi Header and Reset to Pi Header."
		quit()

	# Check for unitialized GPIO i.e. daemon is not running
	if strReadStdout.find("GPIO") != -1:
		print "Error initializing GPIO.  Make sure pigpiod is running (use 'sudo pigpiod' to start the daemon)"
		quit()

	# SBL Read Function
	if functionSelect == SBLFunction.READ:
		if strReadStdout.find("Error") != -1:
			print "Error reading from firmware. Ensure you have jumpers connected to Pi Hat."
			quit()
		return strReadStdout.rstrip() # Remove newline

	# SBL Write Function
	elif functionSelect == SBLFunction.WRITE:
		if strReadStdout.find("Error") != -1:
			print "Error writing to device. Ensure you have jumpers connected to Pi Hat."
			quit()
		return ""

	# SBL Search Function
	elif functionSelect == SBLFunction.FIND:
		if strReadStdout.find("Error") != -1:
			print "Error finding bytes. Ensure you have jumpers connected to Pi Hat."
			quit()
		elif strReadStdout.find("Unable") != -1:
			print "Could not find BT Address location."
			quit()
		idx = strReadStdout.find("0x");
		if idx != -1: foundAddresses = strReadStdout[idx:]
		return foundAddresses.rstrip() # Remove newline

	else:
		print "Error: unsupported SBL function."
		quit()

def print_time_difference(before, after):
	diff = after - before
	print " ({0:.{1}f}ms)".format(diff.total_seconds() * 1000, 2)

# Reads device flash and returns it in a list
def copy_flash():
	lstFlashPages = []
	before = datetime.datetime.now()
	print "Reading flash from device..."

	# Setup progress bar
	sys.stdout.write("[%s]" % (" " * CONST_NUM_FLASH_PAGES))
	sys.stdout.flush()
	sys.stdout.write("\b" * (CONST_NUM_FLASH_PAGES+1)) # Return to start of line, after '['

	for i in range(CONST_NUM_FLASH_PAGES):
		time.sleep(0.1)
		sys.stdout.write("\r[{}{}] Page {}/32".format(("="*i), (" " * (CONST_NUM_FLASH_PAGES - (i+1))), i+1))
		sys.stdout.flush()
		lstFlashPages.append(subprocess_call("sudo ./firmwareDownloadUART -s -r{} -n4096".format(i*4096), SBLFunction.READ))
	print_time_difference(before, datetime.datetime.now())
	return lstFlashPages

def backup_flash():
	# Get file name for backup
	strFileName = raw_input("\nPlease enter a file name for device flash backup: ")
	if (strFileName[-4:] == ".bin"):
		strFileName = strFileName[:-4]
	currentTime = datetime.datetime.now()
	strFileName += currentTime.strftime("--%Y-%m-%dT%Hh%Mm%Ss") + ".bin"
	fWriteBin = open(strFileName, "wb")

	# Write device flash to bin file
	print "\nCopying device flash to bin file...\n"
	lstFlashPages = copy_flash()
	
	# Sanity check
	if (len(lstFlashPages) != CONST_NUM_FLASH_PAGES):
		print "Error reading full device flash"
		quit()

	print "Writing flash to bin file..."
	before = datetime.datetime.now()

	# Setup progress bar
	sys.stdout.write("[%s]" % (" " * CONST_NUM_FLASH_PAGES))
	sys.stdout.flush()
	sys.stdout.write("\b" * (CONST_NUM_FLASH_PAGES+1)) # Return to start of line, after '['

	# Write to bin with progress bar
	for i in range(CONST_NUM_FLASH_PAGES):
		time.sleep(0.1)
		lstFlashPages[i] = lstFlashPages[i].decode("hex")
		bytesToWrite = bytearray(lstFlashPages[i])
		fWriteBin.write(bytesToWrite)
		sys.stdout.write("\r[{}{}] Page {}/32".format(("="*i), (" " * (CONST_NUM_FLASH_PAGES - (i+1))), i+1))
		sys.stdout.flush()
	print_time_difference(before, datetime.datetime.now())

def get_addr_location():
	strAddrOfBTA = subprocess_call("sudo ./firmwareDownloadUART -s -f0x48b4b0", SBLFunction.FIND)
	temp = int(strAddrOfBTA, 16) - 3 # Unique BT address will be 3 bytes before BT Address header
	strAddrOfBTA = hex(temp)
	return strAddrOfBTA
	
def parse_cmd_args():
	# Command line argument parsing
	parser = argparse.ArgumentParser(prog="python firmwareDownloadUART.py",
		formatter_class=argparse.RawTextHelpFormatter,
		description="Python program to change Bluetooth Address of Pi Hat using the SBL library",
		epilog="Remember to take off the jumpers and press the reset button on the Pi Hat when you are done using this program")
	parser.add_argument("-l", "--list", action="store_true", help="Enumerate connected ports")
	parser.add_argument("-p", "--port", metavar="PORT_NUM", default=0, help="Port to connect to (Default: 0)")
	parser.add_argument("-f", "--find", metavar="SEARCH_STR", help="Search for string (for hex use '0x') in Pi Hat firmware")
	parser.add_argument("-r", "--read", nargs=2, metavar=("ADDR","NUM_BYTES"),
		help="Read from address (for hex address use '0x')\nTakes address to read from and number of bytes to read")
	parser.add_argument("-w", "--write", nargs=2, metavar=("ADDR", "WRITE_STR"),
		help="Write to address (for hex address use '0x')\nTakes address to write to and string to write")
	parser.add_argument("-a", "--address", action="store_true", help="Read Bluetooth Address currently stored in Pi Hat")
	parser.add_argument("-n", "--new-address", metavar="BLUETOOTH_ADDR", help="Set new Bluetooth Adress in Pi Hat in hex")
	parser.add_argument("file", nargs="?", metavar="FILE",
		help="File path for bin input of Pi Hat flash\nWARNING: WILL REWRITE FULL FLASH")
	parser.add_argument("-b", "--no-backup", action="store_true", help="Skip backup of device flash to file before writing")
	return parser.parse_args()


if __name__ == "__main__":
	# Get command line
	args = parse_cmd_args()
	global bListPorts
	bListPorts = args.list # Whether or not to enumerate ComPorts
	readCurrentAddr = args.address # Whether or not to read the current address
	strBTA = args.new_address # New address to write to chip
	strBytesToFind = args.find # Bytes to search for
	lstReadData = args.read # Read address and length
	lstWriteData = args.write # Write address and string
	fWriteFlash = args.file # Bin file to overwrite device flash
	bNoBackup = args.no_backup # Whether or not to skip device flash backup

	# Read current address
	if readCurrentAddr:
		# Find address in flash
		print "Finding BT Address..."
		strAddrOfBTA = get_addr_location()

		# Read BT Address from location
		print "Reading BT Address..."
		strOldBTA = subprocess_call("sudo ./firmwareDownloadUART -s -r{} -n6".format(strAddrOfBTA), SBLFunction.READ)
		print "Current BT Address: 0x{}".format("".join(reversed([strOldBTA[i:i+2] for i in range(0, len(strOldBTA), 2)]))) # Reverse for output
		quit()

	# Write new BT Address to chip
	if strBTA:
		try:
			"".join(strBTA.split())
			int(strBTA, 16)
		except ValueError:
			print "Please enter a valid hex address."
			quit()
		if (strBTA[:2] == "0x"): strBTA = strBTA[2:]

		# Check against hardcoded BT address header and size
		if (strBTA[:6] != CONST_BTA_HEADER) or (len(strBTA) != CONST_BTA_SIZE): 
			print "Invalid. Please enter a valid Bluetooth Address."
			quit()

		# Format address for writing
		strRealBTA = "".join(reversed([strBTA[i:i+2] for i in range(0, len(strBTA), 2)]))
		strRealBTA = "0x" + strRealBTA

		# Backup flash before writing address
		if not bNoBackup: backup_flash()

		# Find address in flash
		print "Finding BT Address..."
		strAddrOfBTA = get_addr_location()
		
		# Read BT Address from location
		print "Reading BT Address..."
		strOldBTA = subprocess_call("sudo ./firmwareDownloadUART -s -r{} -n6".format(strAddrOfBTA), SBLFunction.READ)
		print "Current BT Address: 0x{}".format("".join(reversed([strOldBTA[i:i+2] for i in range(0, len(strOldBTA), 2)]))) # Reverse for output
	
		# Write new BT Address to chip
		print "\nWriting new BT Address..."
		subprocess_call("sudo ./firmwareDownloadUART -w{} {}".format(strAddrOfBTA, strRealBTA), SBLFunction.WRITE)
		print "Wrote new BT Address 0x{} to {}.\n\n".format(strBTA, strAddrOfBTA)
		print "Program complete.\nPlease remove the jumpers and press the reset button on the Pi Hat."
		quit()

	if strBytesToFind:
		print "\nSearching flash for {}...".format(strBytesToFind)
		strFound = subprocess_call("sudo ./firmwareDownloadUART -s -f{}".format(strBytesToFind), SBLFunction.FIND)
		print "\nBytes {} were found at:\n{}".format(strBytesToFind, strFound)
		quit()

	if lstReadData:
		strReadAddr = lstReadData[0]
		nReadLength = lstReadData[1]
		print "\nReading {} bytes from {}...".format(strReadAddr, nReadLength)
		strData = subprocess_call(
			"sudo ./firmwareDownloadUART -r{} -n{}".format(strReadAddr, nReadLength),
			SBLFunction.READ)
		idxStart = strData.find("Address");
		idxEnd = strData.find("Resetting");
		strData = strData[idxStart:idxEnd - 2]
		print "\n\n" + strData
		quit()

	if lstWriteData:
		if not bNoBackup: backup_flash()
		strWriteAddr = lstWriteData[0]
		strBytesToWrite = lstWriteData[1]
		print "\nWriting {} to {}...".format(strBytesToWrite, strWriteAddr)
		subprocess_call("sudo ./firmwareDownloadUART -s -w{} {}".format(strWriteAddr, strBytesToWrite), SBLFunction.WRITE)
		print "Success."
		quit()

	if fWriteFlash:
		if not bNoBackup: backup_flash()
		subprocess_call("sudo ./firmwareDownloadUART {}".format(fWriteFlash), SBLFunction.REWRITE_FLASH)
