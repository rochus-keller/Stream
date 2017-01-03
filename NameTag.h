#ifndef __stream_nametag__
#define __stream_nametag__

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

#include <QString>

namespace Stream
{
	struct NameTag
	{
		enum { Size = sizeof(quint32) };
		union
		{
			char d_tag[ Size ];
			quint32 d_id;
		};

		void setNull();
		bool isNull() const { return d_id == 0; }
		QString toString() const;
		QByteArray toByteArray() const;
		NameTag& operator=( const NameTag& rhs ) 
		{
			d_id = rhs.d_id;
			return *this;
		}
		NameTag& operator=( const char* ); // von String
		bool operator==( const char* rhs ) const { return equals( rhs ); }
		bool operator!=( const char* rhs ) const { return !equals( rhs ); }
		bool operator==( const NameTag& rhs ) const { return d_id == rhs.d_id; }
		bool operator!=( const NameTag& rhs ) const { return d_id != rhs.d_id; }
		bool operator<( const NameTag& rhs ) const { return d_id < rhs.d_id; }
		bool equals( const char* ) const;

		NameTag();
		NameTag( const char* ); // von String
		NameTag( quint32 ); // von Id

		static const NameTag null;
	};

}

#endif // __stream_nametag__
