/*
* Copyright 2005-2017 Rochus Keller <mailto:me@rochus-keller.info>
*
* This file is part of the DoorScope Stream library.
*
* The following is the license that applies to this copy of the
* library. For a license to use the library under conditions
* other than those described here, please email to me@rochus-keller.info.
*
* GNU General Public License Usage
* This file may be used under the terms of the GNU General Public
* License (GPL) versions 2.0 or 3.0 as published by the Free Software
* Foundation and appearing in the file LICENSE.GPL included in
* the packaging of this file. Please review the following information
* to ensure GNU General Public Licensing requirements will be met:
* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
* http://www.gnu.org/copyleft/gpl.html.
*/

#include "TimeSlot.h"
using namespace Stream;

TimeSlot::TimeSlot( const QTime& start, quint16 duration )
{
	d_start = QTime( 0, 0 ).secsTo( start ) / 60;
	d_duration = duration;
}

TimeSlot::TimeSlot( const QTime& start, const QTime& end )
{
	setTime( start, end );
}

QTime TimeSlot::getStartTime() const
{
	return QTime( 0, 0 ).addSecs( d_start * 60 );
}

QTime TimeSlot::getEndTime(bool clipMidnight) const
{
	return getStartTime().addSecs( getDuration( clipMidnight ) * 60 );
}

quint16 TimeSlot::getDuration( bool clipMidnight ) const
{
	const int max = 24 * 60 - 1;
	if( clipMidnight && ( d_start + d_duration ) > max )
		return qMax( 0, max - d_start );
	else
		return d_duration;
}

void TimeSlot::setTime( QTime start, QTime end )
{
	if( end < start )
	{
		QTime t = end;
		end = start;
		start = t;
	}
	d_start = QTime( 0, 0 ).secsTo( start ) / 60;
	d_duration = start.secsTo( end ) / 60;
}
