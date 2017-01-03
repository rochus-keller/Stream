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

#include "NameTag.h"
#include <string.h>
using namespace Stream;

const NameTag NameTag::null;

NameTag::NameTag()
{
	setNull();
}

void NameTag::setNull()
{
	d_id = 0;
}

NameTag& NameTag::operator=( const char* str )
{
	const int len = ::strlen( str );
	d_id = 0;
	for( int i = 0; i < len && i < Size; i++ )
		d_tag[ i ] = str[ i ];
	return *this;
}

bool NameTag::equals( const char* str ) const
{
	return *this == NameTag( str );
}

NameTag::NameTag( const char* str )
{
	setNull();
	*this = str;
}

NameTag::NameTag( quint32 id )
{
	setNull();
	d_id = id;
}

QByteArray NameTag::toByteArray() const
{
	char buf[ Size + 1 ];
	::memcpy( buf, d_tag, Size );
	buf[ Size ] = 0;
	return buf;
}

QString NameTag::toString() const
{
	char buf[ Size + 1 ];
	::memcpy( buf, d_tag, Size );
	buf[ Size ] = 0;
	return QString::fromLatin1( buf );
}
