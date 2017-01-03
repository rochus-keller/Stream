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

#include "DataReader.h"
#include "Helper.h"
#include <Stream/Exceptions.h>
#include <QBuffer>
#include <QtDebug>
using namespace Stream;

DataReader::DataReader( const QIODevice* d, bool owner ):
	d_state( Idle ), d_level( 0 ), d_owner( owner ), d_lastToken( Pending ), d_peeking(false)
{
	d_in = const_cast<QIODevice*>( d );
}

DataReader::DataReader( const QByteArray& in ):
	d_state( Idle ),d_level( 0 ), d_owner( true ), d_lastToken( Pending ), d_peeking(false)
{
	QBuffer* buf = new QBuffer();
	buf->buffer() = in;
	d_in = buf;
}

DataReader::DataReader( const DataCell& bml ):
	d_state( Idle ),d_level( 0 ), d_owner( true ), d_lastToken( Pending ), d_peeking(false)
{
	// Erzeuge in jedem Fall QBuffer, auch wenn bml Null ist.
	QBuffer* buf = new QBuffer();
	d_in = buf;
	if( bml.isBml() )
		buf->buffer() = bml.getBml();
}

DataReader::~DataReader()
{
	if( d_in && d_owner )
		delete d_in;
}

void DataReader::setDevice( const QIODevice* in, bool owner )
{
	if( d_in && d_owner )
		delete d_in;
	d_in = const_cast<QIODevice*>( in );
	d_owner = owner;
	d_state = Idle;
	d_lastToken = Pending;
	d_peeking = false;
	d_level = 0;
}

bool DataReader::hasMoreData() const
{
	return d_in && d_in->bytesAvailable() > 0;
}

void DataReader::fetchNext()
{
	open();

	char buf[sizeof(double)];

	// Schaue, was als nächstes kommt
	// Wir benötigen mindestens den DataType
	switch( d_in->peek( buf, 1 ) )
	{
	case 0:
		d_lastToken = Pending;
		return;
    case -1: // d_in ist nicht lesbar
		throw StreamException( StreamException::DeviceAccess );
	}

	DataCell::DataType type = DataCell::symToType( buf[0] ); // throws

	if( d_state == Idle )
	{
		// Beginne von neuem
		if( type == DataCell::FrameStart )
		{
			// Fresse FrameStart, das mit peek oben vorsondiert wurde
			d_in->read( buf, 1 ); 
			// Schaue, was als nächstes kommt
			switch( d_in->peek( buf, 1 ) )
			{
			case 0:
				d_lastToken = Pending;
				return;
            case -1: // d_in ist nicht lesbar
				throw StreamException( StreamException::DeviceAccess );
			}
			type = DataCell::symToType( buf[0] );
			if( type == DataCell::FrameName || 
				type == DataCell::FrameNameTag ||
				type == DataCell::FrameNameStr ||
				type == DataCell::FrameNameIdx )
			{
				if( d_name.readCell( d_in ) == -1 )
				{
					// Wir haben ein Frame aber noch nicht den ganzen Namen.
					d_state = FrameNamePending;
					d_lastToken = Pending;
					return;
				}else
				{
					if( type == DataCell::FrameNameStr )
						d_names.append( d_name.getArr() );
					else if( type == DataCell::FrameNameIdx )
					{
                        if( int(d_name.getId32()) < d_names.size() )
							d_name.setLatin1( d_names[d_name.getId32()] );
					}
					// Wir haben ein Frame und den Namen
					d_level++;
					d_lastToken = BeginFrame;
					return;
				}
			}else
			{
				// Wir haben ein Frame entdeckt ohne Namen.
				d_name.setNull();
				d_level++;
				d_lastToken = BeginFrame;
				return;
			}
		}else if( type == DataCell::FrameEnd )
		{
			// Fresse FrameEnd, das mit peek vorsondiert wurde
			d_in->read( buf, 1 ); 
			d_level--;
			d_lastToken = EndFrame;
			return;
		}else if( type == DataCell::SlotName || 
				type == DataCell::SlotNameTag ||
				type == DataCell::SlotNameStr ||
				type == DataCell::SlotNameIdx )
		{
			if( d_name.readCell( d_in ) == -1 )
			{
				// Wir haben noch nicht den ganzen Slot-Name.
				d_lastToken = Pending;
				return;
			}
			if( type == DataCell::SlotNameStr )
				d_names.append( d_name.getArr() );
			else if( type == DataCell::SlotNameIdx )
			{
                if( int(d_name.getId32()) < d_names.size() )
					d_name.setLatin1( d_names[d_name.getId32()] );
			}
			// Slot-Name gelesen.
			d_peek = DataCell::peekCell( d_in );
			if( !d_peek.isValid() )
			{
				// Wir wissen noch nichts über den Slot
				d_state = SlotPeekPending;
				d_lastToken = Pending;
				return;
			}
			d_state = SlotValuePending;
			if( isValueReady() )
			{
				d_lastToken = Slot;
				return;
			}else
			{
				d_lastToken = Pending;
				return;
			}
		}else
		{
			// Wir haben einen Slot entdeckt ohne Namen
			d_name.setNull();
			d_peek = DataCell::peekCell( d_in );
			if( !d_peek.isValid() )
			{
				// Wir wissen noch nichts über den Slot
				d_lastToken = Pending;
				return;
			}
			d_state = SlotValuePending;
			if( isValueReady() )
			{
				d_lastToken = Slot;
				return;
			}else
			{
				d_lastToken = Pending;
				return;
			}
		}
	}else if( d_state == FrameNamePending )
	{
		Q_ASSERT( type == DataCell::FrameName || 
				type == DataCell::FrameNameTag ||
				type == DataCell::FrameNameStr ||
				type == DataCell::FrameNameIdx );
		if( d_name.readCell( d_in ) == -1 )
		{
			// Wir haben ein Frame aber noch nicht den ganzen Namen.
			d_lastToken = Pending;
			return;
		}else
		{
			// Wir haben ein Frame und den Namen
			d_level++;
			d_state = Idle;
			d_lastToken = BeginFrame;
			return;
		}
	}else if( d_state == SlotPeekPending )
	{
		d_peek = DataCell::peekCell( d_in );
		if( !d_peek.isValid() )
		{
			// Wir wissen noch nichts über den Slot
			d_lastToken = Pending;
			return;
		}
		d_state = SlotValuePending;
		if( isValueReady() )
		{
			d_lastToken = Slot;
			return;
		}else
		{
			d_lastToken = Pending;
			return;
		}
	}else if( d_state == SlotValuePending )
	{
		// Mindestens der Type und Counter des Slots sind schon da.

		if( isValueReady() )
		{
			d_lastToken = Slot;
			return;
		}else
		{
			d_lastToken = Pending;
			return;
		}
	}else
		Q_ASSERT( false );
	d_lastToken =  Pending;
}

DataReader::Token DataReader::nextToken( bool peek )
{
	if( peek )
	{
		if( d_peeking )
		{
			// Wir wollen den peek Wert, und dieser ist schon da wegen vorgängigem peek.
			return DataReader::Token(d_lastToken);
		}else
		{
			// Wir wollen den peek Wert, und dieser ist noch nicht da.
			d_peeking = true;
			fetchNext();
			return DataReader::Token(d_lastToken);
		}
	}else // !peek
	{
		if( d_peeking )
		{
			// Wir wollen den richtigen Wert, und dieser ist schon da wegen vorgängigem peek.
			d_peeking = false;
			return DataReader::Token(d_lastToken);
		}else
		{
			// Wir wollen den richtigen Wert, und dieser ist noch nicht da.
			fetchNext();
			return DataReader::Token(d_lastToken);
		}
	}
}

bool DataReader::readValue( DataCell& value ) const
{
	value = d_value;
	return true;
}

const DataCell& DataReader::readValue() const
{
	return d_value;
}

bool DataReader::isValueReady() const
{
	open();
	if( d_state == SlotValuePending && d_value.readCell( d_in ) >= 0 )
	{
		d_state = Idle;
		return true;
	}else
		return false;
}

void DataReader::open() const
{
	if( d_in == 0 )
		throw StreamException( StreamException::InvalidDevice );
	if( !d_in->isOpen() )
	{
		if( !d_in->open( QIODevice::ReadOnly ) )
			throw StreamException( StreamException::DeviceAccess,
				"cannot open device for reading" );
	}
}

void DataReader::dump(const QByteArray& title)
{
	if( !title.isEmpty() )
		qDebug() << "***Start BML: " << title;
	else
		qDebug() << "***Start BML";
	Token t = nextToken();
	DataCell v;
	while( isUseful(t) )
	{
		switch( t )
		{
		case BeginFrame:
			qDebug() << "BeginFrame: " << getName().toPrettyString();
			break;
		case EndFrame:
			qDebug() << "EndFrame";
			break;
		case Slot:
			readValue( v );
			qDebug() << "Slot: " << getName().toPrettyString() << " = " << v.toPrettyString();
			break;
        default:
            break;
		}
		t = nextToken();
	}
	qDebug() << "***End BML";
	d_in->reset();
}

QString DataReader::extractString(bool unicodeOnly, bool separateBySpace)
{
	QString str;
	Token t = nextToken();
	DataCell v;
	while( isUseful(t) )
	{
		switch( t )
		{
		case BeginFrame:
			break;
		case EndFrame:
			break;
		case Slot:
			readValue( v );
			switch( v.getType() )
			{
			case DataCell::TypeString:
				if( separateBySpace && !str.isEmpty() && !str[str.size()-1].isSpace() )
					str += " ";
				str += v.getStr();
				break;
			case DataCell::TypeLatin1:
			case DataCell::TypeAscii:
				if( !unicodeOnly )
				{
					if( separateBySpace && !str.isEmpty() && !str[str.size()-1].isSpace() )
						str += " ";
					str += QString::fromLatin1( v.getArr() );
				}
				break;
            default:
                break;
			}
			break;
        default:
            break;
		}
		t = nextToken();
	}
    return str;
}

bool DataReader::skipToEndFrame()
{
    const int startLevel = d_level;
    Token t = nextToken();
    while( isUseful( t ) )
    {
        if( t == EndFrame )
        {
            if( d_level < startLevel )
                return true;
        }
        t = nextToken();
    }
    return false;
}
