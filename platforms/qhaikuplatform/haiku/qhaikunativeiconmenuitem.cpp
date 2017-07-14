/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2015-2017 Gerasim Troeglazov,
** Contact: 3dEyes@gmail.com
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <View.h>
#include <Region.h>

#include "qhaikunativeiconmenuitem.h"

IconMenuItem::IconMenuItem(const char *text, BBitmap *icon, BMessage *msg) :
	BMenuItem(text, msg), iconBitmap(icon)
{
} 

IconMenuItem::~IconMenuItem() 
{
} 
 
void
IconMenuItem::GetContentSize(float *width, float *height)
{
	BMenuItem::GetContentSize(width, height);
	*width += 18;
	*height += 2;
}


void IconMenuItem::DrawContent(void) 
{
	Menu()->PushState();

	BFont font;
	Menu()->GetFont(&font);

	BPoint drawPoint(ContentLocation());
	drawPoint.x += 14;
	drawPoint.y += 1;
	Menu()->MovePenTo(drawPoint);
	BMenuItem::DrawContent();

	if(iconBitmap != NULL) {
		float h = iconBitmap->Bounds().Height();
		float w = iconBitmap->Bounds().Height();
		if (iconBitmap->Bounds().Height() > Frame().Height()) {
			float k = w / h;
	    	h = Frame().Height();
	    	w = h*k;
		}
		float y1 = Frame().top + ((Frame().Height()-h)/2);
		float y2 = Frame().top + ((Frame().Height()-h)/2)+h;
	
		//drawing_mode mode=Menu()->DrawingMode();
		Menu()->SetDrawingMode(B_OP_ALPHA);
		Menu()->DrawBitmap(iconBitmap, BRect(Frame().left + 6,y1, Frame().left+6+w,y2) );
		//Menu()->SetDrawingMode(mode);
	}
	Menu()->PopState();	  
} 
