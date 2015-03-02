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

#include "Display.h"

#include <Layout.h>
#include <LayoutBuilder.h>
#include <String.h>

#include "Meter.h"

Display::Display()
	:
	BWindow(BRect(0, 0, 0, 0), "Value", B_TITLED_WINDOW, B_NOT_RESIZABLE)
{
	fStringView = new BStringView("", "AC 0.000 Hz");
	fStringView->SetFontSize(100);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(fStringView);

	BSize psize = GetLayout()->PreferredSize();
	ResizeTo(psize.Width(), psize.Height());
	fStringView->SetText("None");
}


Display::~Display()
{
}


void Display::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_MEASUREMENT:
		{
			SetTitle(message->GetString("name", "Value"));
			fStringView->SetText(BString()
				.SetToFormat("%s %.5g %s",
					message->GetBool("dc", true) ? "DC" : "AC",
					message->GetInt32("value", 0.0) /
						pow(10, message->GetInt32("digits", 0) - message->GetInt32("shift", 0)),
					message->GetString("units", " ")));
			BSize psize = GetLayout()->PreferredSize();
			if (Size().Width() < psize.Width())
				ResizeTo(psize.Width(), Size().Height());
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}
