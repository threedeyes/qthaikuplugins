/****************************************************************************
**
** Copyright (C) 2020 Gerasim Troeglazov,
** Contact: 3dEyes@gmail.com
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

#include "qhaikujoystickbackend_p.h"

#include <QtCore/QDebug>

#include <Joystick.h>

QT_BEGIN_NAMESPACE

static uint8 _hatMappind[] = { 0, 1, 3, 2, 6, 4, 12, 8, 9 };


QHaikuJoystickBackend::QHaikuJoystickBackend(QObject *parent)
    : QGamepadBackend(parent), lastButtonValues(0), lastHatValues(0)
{
    connect(&m_eventLoopTimer, SIGNAL(timeout()), this, SLOT(pumpHaikuEventLoop()));
}

QHaikuJoystickBackend::~QHaikuJoystickBackend()
{
}

void QHaikuJoystickBackend::pumpHaikuEventLoop()
{
	if (joystick->CountDevices() <= 0) {
		emit gamepadRemoved(index);
		m_eventLoopTimer.stop();
	}
	joystick->Update();
	uint32 buttonValues = joystick->ButtonValues();
	for (int buttonIdx = 0; buttonIdx < joystick->CountButtons(); buttonIdx++) {
		bool buttonValue = buttonValues & (1 << buttonIdx);
		bool lastButtonValue = lastButtonValues & (1 << buttonIdx);
		if (buttonValue != lastButtonValue) {
			if (buttonValue)
				emit gamepadButtonPressed(0, translateButton(buttonIdx), 1.0);
			else
				emit gamepadButtonReleased(0, translateButton(buttonIdx));
		}		
	}
	if (joystick->CountHats() > 0) {
		uint8 hats[joystick->CountHats()];
		joystick->GetHatValues(hats);
		uint8 hatValues = _hatMappind[hats[0]];
		for (int buttonIdx = 0; buttonIdx < 4; buttonIdx++) {
			bool buttonValue = hatValues & (1 << buttonIdx);
			bool lastButtonValue = lastHatValues & (1 << buttonIdx);
			if (buttonValue != lastButtonValue) {
				if (buttonValue) {
					emit gamepadButtonPressed(0, translateHatButton(buttonIdx), 1.0);
				} else {
					emit gamepadButtonReleased(0, translateHatButton(buttonIdx));
				}
			}
		}
		lastHatValues = hatValues;
	}
	
	if (joystick->CountAxes() > 0) {
		int16 axes[joystick->CountAxes()];
		joystick->GetAxisValues(axes);
		for (int axesIdx = 0; axesIdx < joystick->CountAxes(); axesIdx++) {
			double value = 0;
			if (axes[axesIdx] >= 0)
                value = axes[axesIdx] / 32767.0;
            else
                value = axes[axesIdx] / 32768.0;

			if (axesIdx == 0) {
				emit gamepadAxisMoved(0, QGamepadManager::AxisLeftX, value);
			}
			if (axesIdx == 1) {
				emit gamepadAxisMoved(0, QGamepadManager::AxisLeftY, value);
			}
			if (axesIdx == 3) {
				emit gamepadAxisMoved(0, QGamepadManager::AxisRightX, value);
			}
			if (axesIdx == 4) {
				emit gamepadAxisMoved(0, QGamepadManager::AxisRightY, value);
			}
		}
	}

	lastButtonValues = buttonValues;
}

bool QHaikuJoystickBackend::start()
{	
	joystick = new BJoystick();	

	if (joystick->CountDevices() <= 0)
		return false;

	char devName[B_OS_NAME_LENGTH];
	joystick->GetDeviceName(0, devName);
	if (joystick->Open(devName) == B_ERROR)
		return false;

	emit gamepadAdded(0);
	emit gamepadNamed(0, devName);
	
    m_eventLoopTimer.start(16);

    return true;
}

void QHaikuJoystickBackend::stop()
{
    m_eventLoopTimer.stop();
    delete joystick;
}


QGamepadManager::GamepadButton QHaikuJoystickBackend::translateButton(int button)
{
    switch (button) {
    case 0:
        return QGamepadManager::ButtonY;
    case 1:
        return QGamepadManager::ButtonB;
    case 2:
        return QGamepadManager::ButtonA;
    case 3:
        return QGamepadManager::ButtonX;
    case 4:
        return QGamepadManager::ButtonL1;
    case 5:
        return QGamepadManager::ButtonR1;
    case 6:
        return QGamepadManager::ButtonL2;
    case 7:
        return QGamepadManager::ButtonR2;
    case 8:
        return QGamepadManager::ButtonSelect;
    case 9:
        return QGamepadManager::ButtonStart;
    case 10:
        return QGamepadManager::ButtonL3;
    case 11:
        return QGamepadManager::ButtonR3;
    default:
        return QGamepadManager::ButtonInvalid;
    }
}

QGamepadManager::GamepadButton QHaikuJoystickBackend::translateHatButton(int button)
{
    switch (button) {
    case 0:
        return QGamepadManager::ButtonUp;
    case 1:
        return QGamepadManager::ButtonRight;
    case 2:
        return QGamepadManager::ButtonDown;
    case 3:
        return QGamepadManager::ButtonLeft;
    default:
        return QGamepadManager::ButtonInvalid;
    }
}

QT_END_NAMESPACE
