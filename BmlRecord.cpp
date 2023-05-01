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

#include "BmlRecord.h"
#include "DataReader.h"
#include <QtDebug>
using namespace Stream;

BmlRecord::BmlRecord( const QByteArray& v )
{
	readFrom( v );
}

BmlRecord::BmlRecord( const DataCell& v )
{
	readFrom( v );
}

void BmlRecord::clear()
{
	d_array.clear();
	d_atoms.clear();
	d_tags.clear();
	d_strings.clear();
}

void BmlRecord::readFrom( const QByteArray& bml )
{
	DataReader r( bml );
	DataReader::Token t = r.nextToken();
	while( t == DataReader::Slot )
	{
		if( r.getName().isNull() )
			d_array.append( r.readValue() );
		else if( r.getName().getType() == DataCell::TypeAtom )
			d_atoms[ r.getName().getAtom() ] = r.readValue();
		else if( r.getName().getType() == DataCell::TypeTag )
			d_tags[ r.getName().getTag() ] = r.readValue();
		else if( r.getName().getType() == DataCell::TypeAscii )
			d_strings[ r.getName().getArr() ] = r.readValue();
		t = r.nextToken();
	}
}

void BmlRecord::readFrom( const DataCell& bml )
{
	clear();
	if( bml.isBml() )
		readFrom( bml.getArr() );
}

void BmlRecord::dump()
{
	qDebug( "*** BmlRecord start" );
	for( int i = 0; i < d_array.size(); i++ )
		qDebug() << i << " = " << d_array[i].toPrettyString();
	for( QMap<quint32,DataCell>::const_iterator j = d_atoms.begin(); j != d_atoms.end(); ++j )
		qDebug() << QString("atom(0x%1)").arg( j.key(), 0, 16 ) << " = " << j.value().toPrettyString();
	for( QMap<NameTag,DataCell>::const_iterator k = d_tags.begin(); k != d_tags.end(); ++k )
		qDebug() << "tag(" << k.key().toString() << ") = " << k.value().toPrettyString();
	for( QMap<QByteArray,DataCell>::const_iterator l = d_strings.begin(); l != d_strings.end(); ++l )
		qDebug() << l.key() << " = " << l.value().toPrettyString();
	qDebug( "*** BmlRecord end" );
}
