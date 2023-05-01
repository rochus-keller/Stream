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

#include "DataWriter.h"
#include "Helper.h"
#include <Stream/Exceptions.h>
#include <QBuffer>
using namespace Stream;

DataWriter::DataWriter( QIODevice* d, bool owner ):
	d_out( d ), d_level(0), d_cells(0), d_nulls(0), d_owner( owner )
{
	if( d_out == 0 )
	{
		d_out = new QBuffer();
		d_owner = true;
	}
}

DataWriter::DataWriter():
	d_level(0), d_cells(0), d_nulls(0)
{
	d_out = new QBuffer();
	d_owner = true;
}

DataWriter::DataWriter(const DataWriter& rhs):
	d_level(0), d_cells(0), d_nulls(0)
{
    Q_UNUSED(rhs);
	d_out = new QBuffer();
	d_owner = true;
}

DataWriter::~DataWriter()
{
	if( d_out && d_owner )
	{
		delete d_out;
	}
}

void DataWriter::setDevice( QIODevice* out, bool owner )
{
	if( d_out && d_owner )
	{
		delete d_out;
	}
	d_out = out;
	d_owner = owner;
	if( d_out == 0 )
	{
		d_out = new QBuffer();
		d_owner = true;
	}
	d_level = 0;
	d_cells = 0;
	d_nulls = 0;
}

void DataWriter::begin()
{
	if( d_level == 0 )
		d_cells++;
	d_level++;
}

void DataWriter::startFrame( DataCell::Atom name )
{
	open();
	Helper::write( d_out, DataCell::typeToSym( DataCell::FrameStart ) );
	begin();
	if( name != DataCell::null )
	{
		Helper::write( d_out, DataCell::typeToSym( DataCell::FrameName ) );
		Helper::write( d_out, name );
	}
}

void DataWriter::startFrame( NameTag name )
{
	open();
	Helper::write( d_out, DataCell::typeToSym( DataCell::FrameStart ) );
	begin();
	if( !name.isNull() )
	{
		Helper::write( d_out, DataCell::typeToSym( DataCell::FrameNameTag ) );
		d_out->write( name.d_tag, NameTag::Size );
	}
}

void DataWriter::startFrame( const char* ascii )
{
	open();
	/* RISK
	if( !DataCell::checkAscii( ascii ) )
		throw StreamException( StreamException::WrongDataFormat,
			"startFrame: expecting ascii name" );
			*/
	Helper::write( d_out, DataCell::typeToSym( DataCell::FrameStart ) );
	begin();
	QByteArray name = ascii;
	QMap<QByteArray,quint32>::const_iterator i = d_names.find( name );
	if( i == d_names.end() )
	{
		// Name existiert noch nicht. Sende ihn explizit
		d_names[name] = d_names.size();
		Helper::write( d_out, DataCell::typeToSym( DataCell::FrameNameStr ) );
		// Schreibt zuerst die Länge
		const quint32 len = name.size() + 1;
		Helper::writeMultibyte32( d_out, len );
		d_out->write( name, len );
	}else
	{
		Helper::write( d_out, DataCell::typeToSym( DataCell::FrameNameIdx ) );
		Helper::writeMultibyte32( d_out, i.value() );
	}
}

void DataWriter::endFrame()
{
	open();
	if( d_level == 0 )
		return;
	d_level--;
	Helper::write( d_out, DataCell::typeToSym( DataCell::FrameEnd ) );
}

void DataWriter::writeSlot( const DataCell& v, DataCell::Atom name, bool compress )
{
	open();
	if( !v.isValid() )
		return;
	//Künftig auch Named-Slots auf Toplevel zulässig
	//if( d_level == 0 && name != DataCell::null )
	//	throw Exception( "writeSlot: named slots not allowed on top level" );
	if( name != DataCell::null )
	{
		Helper::write( d_out, DataCell::typeToSym( DataCell::SlotName ) );
		Helper::write( d_out, name );
	}
	v.writeCell( d_out, false, compress );
	if( d_level == 0 )
	{
		d_cells++;
		if( v.isNull() )
			d_nulls++;
	}
}

void DataWriter::writeSlot( const DataCell& v, NameTag name, bool compress )
{
	open();
	if( !v.isValid() )
		return;
	if( !name.isNull() )
	{
		Helper::write( d_out, DataCell::typeToSym( DataCell::SlotNameTag ) );
		d_out->write( name.d_tag, NameTag::Size );
	}
	v.writeCell( d_out, false, compress );
	if( d_level == 0 )
	{
		d_cells++;
		if( v.isNull() )
			d_nulls++;
	}
}

void DataWriter::writeSlot( const DataCell& v, const char* ascii, bool compress )
{
	open();
	if( !v.isValid() )
		return;
	/* RISK
	if( !DataCell::checkAscii( ascii ) )
		throw StreamException( StreamException::WrongDataFormat,
			"startFrame: expecting ascii name" );
			*/

	QByteArray name = ascii;
	QMap<QByteArray,quint32>::const_iterator i = d_names.find( name );
	if( i == d_names.end() )
	{
		// Name existiert noch nicht. Sende ihn explizit
		d_names[name] = d_names.size();
		Helper::write( d_out, DataCell::typeToSym( DataCell::SlotNameStr ) );
		// Schreibt zuerst die Länge
		const quint32 len = name.size() + 1;
		Helper::writeMultibyte32( d_out, len );
		d_out->write( name, len );
	}else
	{
		// Name wurde bereits verwendet. Hier daher Index
		Helper::write( d_out, DataCell::typeToSym( DataCell::SlotNameIdx ) );
		Helper::writeMultibyte32( d_out, i.value() );
	}
	v.writeCell( d_out, false, compress );

	if( d_level == 0 )
	{
		d_cells++;
		if( v.isNull() )
			d_nulls++;
	}
}

void DataWriter::open()
{
	if( d_out == 0 )
		throw StreamException( StreamException::InvalidDevice );
	if( !d_out->isOpen() )
	{
		if( !d_out->open( QIODevice::WriteOnly ) )
			throw StreamException( StreamException::DeviceAccess,
				"cannot open device for writing" );
	}
}

QByteArray DataWriter::getStream() const
{
	QBuffer* buf = dynamic_cast<QBuffer*>( d_out );
	if( buf )
	{
		buf->close();
		return buf->buffer();
	}else
		return QByteArray();
}
