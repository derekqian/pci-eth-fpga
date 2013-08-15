// About USB:
// USB works in burst mode. In USB terminology, we use "bulk" 
// packets. USB bulk packets are 64 bytes long maximum. 
// The PC can send more at once (up to 65535), but the 
// driver/hardware will slice them in 64 bytes chunks. 
// If you send 100 bytes, the Spartan-II will receive them, 
// but in 2 steps, a first burst of 64 bytes, followed very 
// quickly (but still a few µs later) by a burst of 36 bytes.

// This example code sends data to the LCD module using very short 
// packets (one ot two bytes long) but the functions shown can 

// handle up to 65535 bytes at once.

#include <windows.h>
#include <assert.h>
#include <iostream>
#include <conio.h>

HANDLE* DragonDeviceHandle;

/////////////////
// Open and close the USB driver
void USB_Open()
{
	DragonDeviceHandle = CreateFile("\\\\.\\DRAGON_USB-0", 
		GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	assert(DragonDeviceHandle!=INVALID_HANDLE_VALUE);
}

void USB_Close()
{
	CloseHandle(DragonDeviceHandle);
}

/////////////////
// Low-level USB functions to send and receive bulk packets
// Do not use directly but use USB_Write and USB_Read instead
void USB_BulkWrite(ULONG pipe, void* buffer, WORD buffersize)
{
	int nBytes;
	DeviceIoControl(DragonDeviceHandle, 0x222051, &pipe, sizeof(pipe), buffer, buffersize, &nBytes, NULL);
	assert(nBytes=buffersize);	// make sure everything was sent
}

void USB_BulkRead(ULONG pipe, void* buffer, WORD buffersize)
{
	int nBytes;
	DeviceIoControl(DragonDeviceHandle, 0x22204E, &pipe, sizeof(pipe), buffer, buffersize, &nBytes, NULL);
	assert(nBytes=buffersize);	// make sure everything was read
}

/////////////////
// User functions to send and receive from Dragon
// Can send or receive from 1 to 65535 bytes at once

// When sending more than 64 bytes, packets will reach Dragon in bursts of 64 bytes
void USB_Write(void* buffer, WORD buffersize)
{
	USB_BulkWrite(2, buffer, buffersize);
}

// When reading more than 64 bytes, data will be read in bursts of 64 bytes
void USB_Read(void* buffer, WORD buffersize)
{
	if(buffersize==0) return;
	USB_BulkWrite(6, &buffersize, 2);
	USB_BulkRead(3, buffer, buffersize);
}

/////////////////
// Helper functions
char USB_ReadChar() {
	char c = -1;
	USB_Read(&c, 1);
	return c;
}

void USB_WriteChar(char c)
{
	USB_Write(&c, 1);
}

void USB_WriteWord(WORD w)
{
	USB_Write(&w, 2);
}

/////////////////
int main()
{
	unsigned char c = 0x55;

	USB_Open();

	//USB_WriteWord(0x0100);
	//USB_WriteChar('D');
	//USB_WriteChar(c);
	//printf("W: 0x%02x\r\n", c);

	while(1) {
		if(kbhit()) break;
		c = USB_Read();
		printf("R: 0x%02x\r\n", c);
		Sleep(200);
	}

	USB_Close();
}
