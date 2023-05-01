#ifndef __Stream_TimeSlot__
#define __Stream_TimeSlot__

/*
* Copyright 2005-2017 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the DoorScope Stream library.
*
* The following is the license that applies to this copy of the
* library. For a license to use the library under conditions
* other than those described here, please email to me@rochus-keller.ch.
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

#include <QTime>

namespace Stream
{
	struct TimeSlot // By Value
	{
	public:
		qint16 d_start; // Anzahl Minuten seit Mitternacht, -1 invalid
		quint16 d_duration; // Anzahl Minuten ab Start, max. 1440

		TimeSlot(quint16 s, quint16 d = 0):d_start(s),d_duration(d){}
		TimeSlot( const QTime& start, quint16 duration = 0 ); // duration in Minuten
		TimeSlot( const QTime& start, const QTime& end );
		TimeSlot():d_start(-1),d_duration(0){}

		QTime getStartTime() const;
		QTime getEndTime(bool clipMidnight = true) const; 
		void setTime( QTime start, QTime end );
		quint16 getDuration( bool clipMidnight = true ) const;
		bool isValid() const { return d_start >= 0; }

		// Sortiere aufsteigend nach d_start und absteigend nach d_duration
		bool operator<( const TimeSlot& rhs ) const { return d_start < rhs.d_start || 
			( !( rhs.d_start < d_start ) && d_duration > rhs.d_duration ); }
	};
}

#endif
