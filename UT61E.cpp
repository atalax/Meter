/* 
 * The MIT License (MIT)
 * 
 * Copyright (c) 2015 Josef Gajdusek
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "UT61E.h"

#include <stdlib.h>

#include "Meter.h"

#define P_RANGE		0
#define P_D4		1
#define P_D3		2
#define P_D2		3
#define P_D1		4
#define P_D0		5
#define P_FN		6
#define P_STAT		7
#define P_O1		8
#define P_O2		9
#define P_O3		10
#define P_O4		11
#define P_CR		12
#define P_LF		13

#define FN_VOLTAGE	0x3b
#define FN_AUTO_UA	0x3d
#define FN_AUTO_MA	0x3f
#define FN_A		0x30
#define FN_MAN_A	0x39
#define FN_OHM		0x33
#define FN_CONT		0x35
#define FN_DIODE	0x31
#define FN_FREQ		0x32
#define FN_CAP		0x36
#define FN_TEMP		0x34
#define FN_ADP		0x3e

#define DIG_MASK	0x0f

#define CR			0x0d
#define LF			0x0a


UT61E::UT61E(const char* port)
	:
	Device("UNI-T UT61E"),
	fPortName(port)
{
	srand(time(NULL));
}


UT61E::~UT61E()
{
}

#include <iostream>
#include <iomanip>
#include <stdio.h>
using namespace std;

void UT61E::PollThread()
{
	fPort.Open(fPortName);
	// The baud rate is actually 19230, but that is not supported by most USB<->Serial converters
	fPort.SetDataRate((data_rate) 19200);
	fPort.SetParityMode(B_ODD_PARITY);
	fPort.SetDataBits(B_DATA_BITS_7);
	fPort.SetStopBits(B_STOP_BIT_1);

	uint8_t buff[14];
	int ptr = 0;
	while (true) {
		ptr += fPort.Read(&buff[ptr], sizeof(buff) - ptr);
		if (ptr == 14) {
			ptr = 0;
			printf("Packet: ");
			for (int i = 0; i < 14; i++)
				printf("%x,", buff[i] & 0xff);
			printf("\n");
			fflush(stdout);

			if (buff[P_CR] != CR ||
					buff[P_LF] != LF) {
				fprintf(stderr, "Invalid packet received, trying to recover...!\n");
				while (buff[0] != LF)
					fPort.Read(buff, 1);
				fPort.ClearInput();
				continue;
			}

			BMessage* msg = new BMessage(MSG_MEASUREMENT);

			int val = 0;
			for (int i = P_D4; i <= P_D0; i++)
				val = (val * 10) + (DIG_MASK & buff[i]);

			int shift = 0;
			BString units = "?";
			// Parse the units and ranges
			uint8_t range = buff[P_RANGE] & 0x0f; // The first nibble is constant
			switch (buff[P_FN]) {
			case FN_VOLTAGE:
				switch (range) {
				case 0x0:
				case 0x1:
				case 0x2:
				case 0x3:
					units = "V";
					shift = (buff[P_RANGE] & 0x0f) + 1;
					break;
				case 0x4:
					units = "mV";
					shift = 3;
					break;
				default:
					fprintf(stderr, "Received invalid RANGE byte: %02x\n", buff[P_RANGE]);
					continue;
				}
				break;
			case FN_AUTO_UA:
			case FN_AUTO_MA:
				// TODO
				break;
			case FN_A:
				units = "A";
				shift = 2;
				break;
			case FN_MAN_A:
				units = "A";
				shift = range + 1;
				break;
			case FN_OHM:
				switch (range) {
				case 0x0:
					shift = 3;
					units = "Ω";
					break;
				case 0x1:
				case 0x2:
				case 0x3:
					units = "kΩ";
					shift = range;
					break;
				case 0x4:
				case 0x5:
				case 0x6:
					units = "MΩ";
					shift = range - 3;
					break;
				}
				break;
			case FN_CONT:
				// TODO
				break;
			case FN_DIODE:
				// TODO
				break;
			case FN_FREQ:
				switch (range) {
				case 0x0:
				case 0x1:
					units = "Hz";
					shift = range + 2;
					break;
				case 0x3:
				case 0x4:
				case 0x5:
					units = "kHz";
					shift = range - 1;
					break;
				case 0x6:
				case 0x7:
				case 0x8:
					units = "MHz";
					shift = range -3;
					break;
				}
				break;
			case FN_CAP:
				switch(range) {
					case 0x0:
					case 0x1:
						units = "nF";
						shift = range + 2;
						break;
					case 0x2:
					case 0x3:
					case 0x4:
						units = "μF";
						shift = range - 1;
						break;
					case 0x5:
					case 0x6:
					case 0x7:
						units = "mF";
						shift = range - 4;
						break;
				};
				break;
			case FN_TEMP:
				// TODO UT61E does not have temperature setting
				break;
			case FN_ADP:
				// TODO I have no idea what this is...
				break;
			default:
				break;
			}

			printf("Val: %d, Dec: %d\n", val, shift);

			msg->SetBool("dc", true);
			msg->SetBool("auto", true);
			msg->SetString("name", Name());
			msg->SetInt32("shift", shift);
			msg->SetInt32("value", val);
			msg->SetString("units", units);
			msg->SetInt32("digits", 5);
			Invoke(msg);
		}
	}
}
