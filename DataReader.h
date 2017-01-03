#ifndef __stream_datareader__
#define __stream_datareader__

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

#include <Stream/DataCell.h>
#include <QList>

namespace Stream
{
	class DataReader
	{
	public:
		enum Token { Pending, BeginFrame, EndFrame, Slot };

		Token nextToken(bool peek = false);
		bool hasMoreData() const;
		bool isValueReady() const;
		bool readValue( DataCell& value ) const; // true..fertig gelesen
		const DataCell& readValue() const;
		const DataCell& getValue() const { return d_value; }
		const DataCell& getName() const { return d_name; }
		qint16 getLevel() const { return d_level; }
		void setDevice( const QIODevice*, bool owner = false );
		bool hasDevice() const { return d_in != 0; }
		static bool isUseful( Token t ) { return t >= BeginFrame; }
		void dump(const QByteArray& title = QByteArray() );
		QString extractString(bool unicodeOnly = true, bool separateBySpace = true );
		Token getCurrentToken() const { return Token(d_lastToken); }
        bool skipToEndFrame(); // Bis und mit EndFrame

		DataReader( const QIODevice* = 0, bool owner = false );
		DataReader( const QByteArray& ); // Variante mit owned QBuffer
		DataReader( const DataCell& bml );
		~DataReader();
	private:
        DataReader( const DataReader& ) {}
		DataReader& operator=( const DataReader& ) { return *this; }
		void open() const;
		void fetchNext();
		QIODevice* d_in;
		DataCell d_name;
		mutable DataCell d_value;
		enum State { Idle, FrameNamePending, SlotPeekPending, SlotValuePending };
		mutable quint32 d_state : 2;
		quint32 d_lastToken : 2;
		quint32 d_peeking : 1;
		quint32 d_owner : 1;
		qint32 d_level : 16;
		quint32 dummy : 10;
		DataCell::Peek d_peek;
		QList<QByteArray> d_names;

		// DONT_CREATE_ON_HEAP;
	};
}

#endif
