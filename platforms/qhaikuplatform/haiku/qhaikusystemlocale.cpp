/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Copyright (C) 2015-2018 Gerasim Troeglazov,
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

#include <FormattingConventions.h>
#include <Locale.h>
#include <LocaleRoster.h>
#include <Message.h>
#include <String.h>

#include "qhaikusystemlocale.h"
#include "qdatetime.h"
#include "qstringlist.h"
#include "qvariant.h"

QT_BEGIN_NAMESPACE

QHaikuSystemLocale::QHaikuSystemLocale() : m_locale(QLocale::English)
{
}

void QHaikuSystemLocale::getLocaleFromHaiku() const
{
    QWriteLocker locker(&m_lock);

	BMessage preferredLanguages;
	BLocaleRoster::Default()->GetPreferredLanguages(&preferredLanguages);
	QString languageCode = "en";
	const char* firstPreferredLanguage;
	if (preferredLanguages.FindString("language", &firstPreferredLanguage) == B_OK)
		languageCode = QString(firstPreferredLanguage);

    QString countryCode = "US";    
    BFormattingConventions conventions;
	BLocale::Default()->GetFormattingConventions(&conventions);
	if (conventions.CountryCode() != NULL)
		countryCode = QString(conventions.CountryCode());

    m_locale = QLocale(languageCode + QLatin1Char('_') + countryCode);
}

QVariant QHaikuSystemLocale::query(QueryType type, QVariant in) const
{
    if (type == LocaleChanged) {
        getLocaleFromHaiku();
        return QVariant();
    }

    QReadLocker locker(&m_lock);

    switch (type) {
    case DecimalPoint:
        return m_locale.decimalPoint();
    case GroupSeparator:
        return m_locale.groupSeparator();
    case ZeroDigit:
        return m_locale.zeroDigit();
    case NegativeSign:
        return m_locale.negativeSign();
    case DateFormatLong:
        return m_locale.dateFormat(QLocale::LongFormat);
    case DateFormatShort:
        return m_locale.dateFormat(QLocale::ShortFormat);
    case TimeFormatLong:
        return m_locale.timeFormat(QLocale::LongFormat);
    case TimeFormatShort:
        return m_locale.timeFormat(QLocale::ShortFormat);
    case DayNameLong:
        return m_locale.dayName(in.toInt(), QLocale::LongFormat);
    case DayNameShort:
        return m_locale.dayName(in.toInt(), QLocale::ShortFormat);
    case MonthNameLong:
        return m_locale.monthName(in.toInt(), QLocale::LongFormat);
    case MonthNameShort:
        return m_locale.monthName(in.toInt(), QLocale::ShortFormat);
    case StandaloneMonthNameLong:
        return m_locale.standaloneMonthName(in.toInt(), QLocale::LongFormat);
    case StandaloneMonthNameShort:
        return m_locale.standaloneMonthName(in.toInt(), QLocale::ShortFormat);
    case DateToStringLong:
        return m_locale.toString(in.toDate(), QLocale::LongFormat);
    case DateToStringShort:
        return m_locale.toString(in.toDate(), QLocale::ShortFormat);
    case TimeToStringLong:
        return m_locale.toString(in.toTime(), QLocale::LongFormat);
    case TimeToStringShort:
        return m_locale.toString(in.toTime(), QLocale::ShortFormat);
    case DateTimeFormatLong:
        return m_locale.dateTimeFormat(QLocale::LongFormat);
    case DateTimeFormatShort:
        return m_locale.dateTimeFormat(QLocale::ShortFormat);
    case DateTimeToStringLong:
        return m_locale.toString(in.toDateTime(), QLocale::LongFormat);
    case DateTimeToStringShort:
        return m_locale.toString(in.toDateTime(), QLocale::ShortFormat);
    case PositiveSign:
        return m_locale.positiveSign();
    case AMText:
        return m_locale.amText();
    case PMText:
        return m_locale.pmText();
    case FirstDayOfWeek:
        return m_locale.firstDayOfWeek();
    case CurrencySymbol:
        return m_locale .currencySymbol(QLocale::CurrencySymbolFormat(in.toUInt()));
    case CurrencyToString: {
        switch (in.type()) {
        case QVariant::Int:
            return m_locale .toCurrencyString(in.toInt());
        case QVariant::UInt:
            return m_locale .toCurrencyString(in.toUInt());
        case QVariant::Double:
            return m_locale .toCurrencyString(in.toDouble());
        case QVariant::LongLong:
            return m_locale .toCurrencyString(in.toLongLong());
        case QVariant::ULongLong:
            return m_locale .toCurrencyString(in.toULongLong());
        default:
            break;
        }
        return QString();
    }
    case StringToStandardQuotation:
        return m_locale.quoteString(in.value<QStringRef>());
    case StringToAlternateQuotation:
        return m_locale.quoteString(in.value<QStringRef>(), QLocale::AlternateQuotation);
    case ListToSeparatedString:
        return m_locale.createSeparatedList(in.value<QStringList>());
    case LocaleChanged:
        Q_ASSERT_X(false, Q_FUNC_INFO, "This can't happen.");
    default:
        break;
    }
    return QVariant();
}

QLocale QHaikuSystemLocale::fallbackUiLocale() const
{
    QReadLocker locker(&m_lock);
    return m_locale;
}

QT_END_NAMESPACE
