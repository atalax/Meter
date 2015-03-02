/* 
 * The MIT License (MIT)
 * 
 * Copyright (c) 2014 Josef Gajdusek
 * Based on xscreensaver's hacks/substrate.c
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

#include "SerialChooser.h"

#include <Button.h>
#include <GroupLayout.h>
#include <RadioButton.h>
#include <LayoutBuilder.h>
#include <SerialPort.h>
#include <String.h>
#include <Window.h>

SerialChooser::SerialChooser()
	:
	BWindow(BRect(0, 0, 0, 0), "Choose the serial port", B_MODAL_WINDOW,
			B_NOT_RESIZABLE)
{
	BGroupLayout* grp = new BGroupLayout(B_VERTICAL, 0);
	grp->SetInsets(B_USE_ITEM_INSETS);

	BButton* btn = new BButton("Ok", new BMessage(B_CONTROL_INVOKED));
	btn->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT,
				B_ALIGN_VERTICAL_UNSET));
	SetDefaultButton(btn);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_ITEM_SPACING)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(grp)
		.Add(btn);

	BSerialPort port;
	for (int i = 0; i < port.CountDevices(); i++) {
		char buf[256];
		port.GetDeviceName(i, buf, sizeof(buf));

		BRadioButton* radio = new BRadioButton(buf, NULL);
		fControls.AddItem(radio);
		grp->AddView(radio);
	}
}


SerialChooser::~SerialChooser()
{
}


void SerialChooser::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_CONTROL_INVOKED:
			for (int i = 0; i < fControls.CountItems(); i++)
				if (fControls.ItemAt(i)->Value() == B_CONTROL_ON) {
					fPortName = fControls.ItemAt(i)->Label();
					break;
				}
			delete_sem(fSem);
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


BString SerialChooser::Go()
{
	fSem = create_sem(0, "EditSem");

	BSize psize = GetLayout()->PreferredSize();
	ResizeTo(psize.Width(), psize.Height());
	Show();
	CenterOnScreen();

	acquire_sem(fSem);

	BString ret = fPortName;
	if (Lock())
		Quit();
	return ret;
}
