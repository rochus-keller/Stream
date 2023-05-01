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

#include "DataCell.h"
#include "Helper.h"
#include <Stream/Exceptions.h>
#include <QBuffer>
#include <QDataStream>
#include "DataReader.h"
#include <cassert>
#include <QtDebug>
using namespace Stream;

/////////////////////////////////////////////////////////////////////////////////////
// Stream Types

// VORSICHT: diese Konstanten nicht mehr ändern. Die Werte wurden in der Datenbank
// bereits gespeichert. Erweiterungen sind erlaubt, nicht aber Änderungen.
// VORSICHT: Das Most Significant Bit wird verwendet, um Komprimierung zu kodieren!
static const quint8 s_symNull = 0;
static const quint8 s_symTrue = 1;
static const quint8 s_symFalse = 2;
static const quint8 s_symInt32 = 3;
static const quint8 s_symDouble = 4;
static const quint8 s_symFloat = 5;
static const quint8 s_symDate = 10;
static const quint8 s_symTime = 11;
static const quint8 s_symDateTimeOld = 12; // stream time, date
static const quint8 s_symTag = 13;
static const quint8 s_symUInt8 = 14;
static const quint8 s_symInt64 = 15;
static const quint8 s_symTimeSlot = 16;
static const quint8 s_symDateTimeNew = 17; // stream date, time -> richtige Sortierung
static const quint8 s_symUInt16 = 18;
static const quint8 s_symAtom = 20;
//static const quint8 s_symOid32 = 21;
static const quint8 s_symUrl = 22;
static const quint8 s_symUuid = 23;
static const quint8 s_symOid64 = 24;
static const quint8 s_symId32 = 25;
static const quint8 s_symId64 = 26;
static const quint8 s_symSid = 27;
static const quint8 s_symRid = 28;
static const quint8 s_symUInt64 = 29;
static const quint8 s_symUInt32 = 30;
static const quint8 s_symLatin1 = 40;
static const quint8 s_symString = 41;
static const quint8 s_symHtml = 42;
static const quint8 s_symXml = 43;
static const quint8 s_symAscii = 44;
static const quint8 s_symLob = 60;
//static const quint8 s_symLbin = 61;
//static const quint8 s_symGetter = 62;
//static const quint8 s_symGetSetter = 63;
static const quint8 s_symImg = 64;
static const quint8 s_symPic = 65;
static const quint8 s_symBml = 66;
static const quint8 s_symFrameStart = 110;
static const quint8 s_symFrameName = 111;
static const quint8 s_symFrameEnd = 112;
static const quint8 s_symSlotName = 113;
static const quint8 s_symFrameNameStr = 114;
static const quint8 s_symSlotNameStr = 115;
static const quint8 s_symFrameNameTag = 116;
static const quint8 s_symSlotNameTag = 117;
static const quint8 s_symFrameNameIdx = 118;
static const quint8 s_symSlotNameIdx = 119;
static const quint8 s_symInvalid = 0x7f; // 127

const char* DataCell::bmlMimeType = "application/x-bml";
static const quint32 s_compressionThreshold = 127;

bool DataCell::symIsCompressed( quint8 sym )
{
	return ( sym & 0x80 ) != 0;
}

DataCell::DataType DataCell::symToType( quint8 sym )
{
	switch( sym & 0x7f ) // nur die ersten 7 Bit verwendet. Achtes Bit ist Kompressionsflag
	{
	case s_symNull:
		return TypeNull;
	case s_symTrue:
		return TypeTrue;
	case s_symFalse:
		return TypeFalse;
	case s_symInt32:
		return TypeInt32;
	case s_symUInt32:
		return TypeUInt32;
	case s_symInt64:
		return TypeInt64;
	case s_symUInt64:
		return TypeUInt64;
	case s_symDouble:
		return TypeDouble;
    case s_symFloat:
		return TypeFloat;
	case s_symDate:
		return TypeDate;
	case s_symTime:
		return TypeTime;
	case s_symDateTimeOld:
    case s_symDateTimeNew:
		return TypeDateTime;
	case s_symTimeSlot:
		return TypeTimeSlot;
	case s_symTag:
		return TypeTag;
	case s_symUrl:
		return TypeUrl;
	case s_symUInt8:
		return TypeUInt8;
	case s_symUInt16:
		return TypeUInt16;
	case s_symAtom:
		return TypeAtom;
	case s_symOid64:
		return TypeOid;
	case s_symRid:
		return TypeRid;
	case s_symSid:
		return TypeSid;
	case s_symId32:
		return TypeId32;
	case s_symId64:
		return TypeId64;
	case s_symLatin1:
		return TypeLatin1;
	case s_symAscii:
		return TypeAscii;
	case s_symString:
		return TypeString;
	case s_symHtml:
		return TypeHtml;
	case s_symXml:
		return TypeXml;
	case s_symLob:
		return TypeLob;
	case s_symBml:
		return TypeBml;
	case s_symFrameStart:
		return FrameStart;
	case s_symFrameName:
		return FrameName;
	case s_symFrameNameTag:
		return FrameNameTag;
	case s_symFrameNameStr:
		return FrameNameStr;
	case s_symFrameNameIdx:
		return FrameNameIdx;
	case s_symFrameEnd:
		return FrameEnd;
	case s_symSlotName:
		return SlotName;
	case s_symSlotNameStr:
		return SlotNameStr;
	case s_symSlotNameIdx:
		return SlotNameIdx;
	case s_symSlotNameTag:
		return SlotNameTag;
	case s_symImg:
		return TypeImg;
	case s_symPic:
		return TypePic;
	case s_symUuid:
		return TypeUuid;
	}
	return TypeInvalid;
}

quint8 DataCell::typeToSym( DataType t )
{
	switch( t )
	{
	case TypeNull:
		return s_symNull;
	case TypeTrue:
		return s_symTrue;
	case TypeFalse:
		return s_symFalse;
	case TypeInt32:
		return s_symInt32;
	case TypeUInt32:
		return s_symUInt32;
	case TypeInt64:
		return s_symInt64;
	case TypeUInt64:
		return s_symUInt64;
	case TypeDouble:
		return s_symDouble;
    case TypeFloat:
		return s_symFloat;
	case TypeDate:
		return s_symDate;
	case TypeTime:
		return s_symTime;
	case TypeDateTime:
		return s_symDateTimeNew;
	case TypeTimeSlot:
		return s_symTimeSlot;
	case TypeTag:
		return s_symTag;
	case TypeUInt8:
		return s_symUInt8;
	case TypeUInt16:
		return s_symUInt16;
	case TypeAtom:
		return s_symAtom;
	case TypeOid:
		return s_symOid64;
	case TypeRid:
		return s_symRid;
	case TypeSid:
		return s_symSid;
	case TypeId32:
		return s_symId32;
	case TypeId64:
		return s_symId64;
	case TypeLatin1:
		return s_symLatin1;
	case TypeAscii:
		return s_symAscii;
	case TypeString:
		return s_symString;
	case TypeHtml:
		return s_symHtml;
	case TypeXml:
		return s_symXml;
	case TypeLob:
		return s_symLob;
	case TypeBml:
		return s_symBml;
	case FrameStart:
		return s_symFrameStart;
	case FrameName:
		return s_symFrameName;
	case FrameNameStr:
		return s_symFrameNameStr;
	case FrameNameIdx:
		return s_symFrameNameIdx;
	case FrameNameTag:
		return s_symFrameNameTag;
	case FrameEnd:
		return s_symFrameEnd;
	case SlotNameStr:
		return s_symSlotNameStr;
	case SlotNameIdx:
		return s_symSlotNameIdx;
	case SlotNameTag:
		return s_symSlotNameTag;
	case SlotName:
		return s_symSlotName;
	case TypeUrl:
		return s_symUrl;
	case TypeImg:
		return s_symImg;
	case TypePic:
		return s_symPic;
	case TypeUuid:
		return s_symUuid;
	default:
		throw StreamException( StreamException::IncompleteImplementation, 
			"Not symbol defined for given value type" );
	}
	return TypeInvalid;
}

const int DataCell::UNISTR = -1;
const int DataCell::CSTRING = -2;
const int DataCell::BINARY = -3;
const int DataCell::MBYTE64 = -4;
const int DataCell::MBYTE32 = -5;
const int DataCell::typeByteCount[] =
{
	0,					// TypeNull,
	0,					// TypeTrue,
	0,					// TypeFalse,
	4,					// TypeAtom,  
	MBYTE64,			// TypeOid,	
	MBYTE64,			// TypeRid,
	MBYTE32,			// TypeSid,
	MBYTE32,			// TypeId32,
	MBYTE64,			// TypeId64,
	1,					// TypeUInt8
	2,					// TypeUInt16
	4,					// TypeInt32,
	4,					// TypeUInt32,
	8,					// TypeInt64,
	8,					// TypeUInt64,
	8,					// TypeDouble,
    4,					// TypeFloat,
	CSTRING,			// TypeLatin1,
	CSTRING,			// TypeAscii,	
	UNISTR,				// TypeString,	
	BINARY,				// TypeLob,	
	BINARY,				// TypeBml,	nur das Symbol
	4,					// TypeDate,
	4,					// TypeTime,
	8,					// TypeDateTime
	4,					// TypeTimeSlot
	CSTRING,			// TypeUrl,	
	BINARY,				// TypeImg
	BINARY,				// TypePic
	BINARY,				// TypeUuid
	UNISTR,				// TypeHtml	
	UNISTR,				// TypeXml	
	4,					// TypeTag
	0,					// MaxType
	0,					// FrameStart
	4,					// FrameName
	CSTRING,			// FrameNameStr
	MBYTE32,			// FrameNameIdx
	4,					// FrameNameTag
	0,					// FrameEnd
	4,					// SlotName
	CSTRING,			// SlotNameStr
	MBYTE32,			// SlotNameIdx
	4,					// SlotNameTag
	0,					// TypeInvalid
};
const char* DataCell::typePrettyName[] =
{
	"Null",				// TypeNull,
	"Boolean",			// TypeTrue,
	"Boolean",			// TypeFalse,
	"Atom",				// TypeAtom,  
	"OID",				// TypeOid
	"RID",				// TypeRid
	"SID",				// TypeSid
	"ID32",				// TypeId32
	"ID64",				// TypeId64
	"quint8",			// TypeUInt8
	"UInt16",			// TypeUInt16
	"Int32",			// TypeInt32,
	"UInt32",			// TypeUInt32,
	"Int64",			// TypeInt64,
	"UInt64",			// TypeUInt64,
	"Double",			// TypeDouble,
    "Float",			// TypeFloat,
	"Latin1",			// TypeLatin1,
	"Ascii",			// TypeAscii,	
	"Utf8",				// TypeString,	
	"BLOB",				// TypeLob,	
	"BML",				// TypeBml,	
	"Date",				// TypeDate,
	"Time",				// TypeTime,
	"DateTime",			// TypeDateTime
	"TimeSlot",			// TypeTimeSlot
	"URL",				// TypeUrl,	
	"Image",			// TypeImg
	"Picture",			// TypePic
	"UUID",				// TypeUuid
	"HTML",				// TypeHtml
	"XML",				// TypeXml
	"Tag",				// TypeTag
};

/////////////////////////////////////////////////////////////////////////////////////
// Time & Date

static const quint32 SECS_PER_DAY	= 86400;
static const quint32 MSECS_PER_DAY = 86400000;
static const quint32 SECS_PER_HOUR = 3600;
static const quint32 MSECS_PER_HOUR= 3600000;
static const quint32 SECS_PER_MIN	= 60;
static const quint32 MSECS_PER_MIN = 60000;

static QTime _toTime( quint32 i )
{
    i = i & 0x0FFFFFFF; // Schneide vordersten vier MSB ab
	return QTime( i / MSECS_PER_HOUR, (i % MSECS_PER_HOUR)/MSECS_PER_MIN,
		(i / 1000)%SECS_PER_MIN, i % 1000 );
}

static quint32 _fromTime( const QTime& t )
{
	return ( t.hour() * SECS_PER_HOUR + t.minute() * SECS_PER_MIN + t.second() )*
		1000 + t.msec();
    // Maximal resultierden Zahl: ( 23 * 3600 + 59 * 60 + 59 ) * 1000 + 999 = 86'399'999
    // oder Binär  00000101 00100110 01011011 11111111 oder Hex  05 26 5B FF
    // Man kann also MSB gut für Kodierung UTC-Bit verwenden
}

//////////////////////////////////////////////////////////////////////////////////////
// DataCell

const quint32 DataCell::null = 0; 

void DataCell::clear()
{
	if( typeByteCount[d_type] == UNISTR )
	{
		QString* s = (QString*) d_buf;
		s->~QString();
	}else if( typeByteCount[d_type] == BINARY || typeByteCount[d_type] == CSTRING )
	{
		QByteArray* ba = (QByteArray*) d_buf;
		ba->~QByteArray();
	}
	d_uint64 = 0;
	d_type = TypeInvalid;
}

DataCell& DataCell::setNull()
{
	clear();
	d_type = TypeNull;
	return *this;
}

void DataCell::assign( const DataCell& rhs )
{
	clear();
	d_type = rhs.d_type;
	if( typeByteCount[d_type] == UNISTR )
	{
		setStr( rhs.getStr() );
	}else if( typeByteCount[d_type] == BINARY || typeByteCount[d_type] == CSTRING )
	{
		setArr( rhs.getArr() );
	}else
		::memcpy( d_buf, rhs.d_buf, sizeof(double) );

}

bool DataCell::equals( const DataCell& rhs ) const
{
	if( d_type != rhs.d_type )
		return false;
	switch( typeByteCount[d_type] )
	{
	case UNISTR:
		return getStr() == rhs.getStr();
	case BINARY:
	case CSTRING:
		return getArr() == rhs.getArr();
	default:
		return ::memcmp( d_buf, rhs.d_buf, sizeof(double) ) == 0;
	}
	return false;
}

void DataCell::setStr( const QString& in )
{
	assert( sizeof(d_buf) >= sizeof(QString) );
    new( d_buf ) QString( in );
}

QString DataCell::getStr() const 
{ 
	if( typeByteCount[d_type] != UNISTR )
		return QString();
	return *(QString*) d_buf;
}

QByteArray DataCell::getArr() const 
{ 
	if( typeByteCount[d_type] != BINARY && typeByteCount[d_type] != CSTRING )
		return QByteArray();
	return *(QByteArray*) d_buf;
}

void DataCell::setArr( const QByteArray& in )
{
	assert( sizeof(d_buf) >= sizeof(QByteArray) );
    new( d_buf ) QByteArray( in );
}

DataCell& DataCell::setLatin1( const QByteArray& str, bool nullIfEmpty )
{
	clear();
	if( !nullIfEmpty || !str.isEmpty() )
	{
		d_type = TypeLatin1;
		setArr( str );
	}else
		setNull();
	return *this;
}

DataCell& DataCell::setAscii( const QByteArray& str, bool nullIfEmpty )
{
	clear();
	if( !checkAscii( str ) )
		throw StreamException( StreamException::WrongDataFormat, "expecting ascii" );
	if( !nullIfEmpty || !str.isEmpty() )
	{
		d_type = TypeAscii;
		setArr( str );
	}else
		setNull();
	return *this;
}

DataCell& DataCell::setString( const QString& str, bool nullIfEmpty )
{
	clear();
	if( !nullIfEmpty || !str.isEmpty() )
	{
		d_type = TypeString;
		setStr( str );
	}else
		setNull();
	return *this;
}

DataCell& DataCell::setHtml( const QString& ba, bool nullIfEmpty )
{
	clear();

	if( !nullIfEmpty || !ba.isEmpty() )
	{
		setStr( ba );
		d_type = TypeHtml;
	}else
		setNull();
	return *this;
}

DataCell& DataCell::setXml( const QString& str, bool nullIfEmpty )
{
	clear();
	if( !nullIfEmpty || !str.isEmpty() )
	{
		setStr( str );
		d_type = TypeXml;
	}else
		setNull();
	return *this;
}

DataCell& DataCell::setLob( const QByteArray& data, bool nullIfEmpty )
{
	clear();
    if( !nullIfEmpty || !data.isEmpty() )
	{
        d_type = TypeLob;
        setArr( data );
	}else
		setNull();
	return *this;
}

DataCell& DataCell::setUrl( const QByteArray& ascii )
{
	clear();
	if( !checkAscii( ascii ) )
		throw StreamException( StreamException::WrongDataFormat,
			"DataCell::setUrl: only accepting ascii" );
	QByteArray tmp = ascii.trimmed();
	if( tmp.isEmpty() )
		d_type = TypeNull;
	else
	{
		d_type = TypeUrl;
		setArr( tmp );
	}
	return *this;
}

DataCell& DataCell::setUrl( const QString& unencoded )
{
	QString tmp = unencoded.trimmed();
	if( !tmp.isEmpty() )
		setUrl( QUrl( tmp ) );
	else
		setNull();
	return *this;
}

DataCell& DataCell::setUrl( const QUrl& url )
{
	clear();
	if( !url.isEmpty() )
	{
		d_type = TypeUrl;
		QByteArray tmp = url.toEncoded();
		// QT-BUG: toEncoded scheint irgendwie seltsam zu funktionieren. Am Schluss wird %00 angehängt
		if( tmp.endsWith( "%00" ) )
			tmp.chop( 3 );
		if( !tmp.isEmpty() )
			setArr( tmp ); 
		else
			d_type = TypeNull;
	}else
		d_type = TypeNull;
	return *this;
}

QUrl DataCell::getUrl() const
{
	switch( d_type )
	{
	case TypeAscii:
	case TypeLatin1:
	case TypeUrl:
		//qDebug() << QUrl::fromEncoded( getArr() ).toString(); // TEST
		// QT-BUG: der URL-Parser hat noch ein Problem mit Leerzeichen
		if( !getArr().isEmpty() )
			return QUrl::fromEncoded( getArr() );
	case TypeString:
		if( !getStr().isEmpty() )
            return QUrl::fromEncoded( getStr().toLatin1() ); // toAscii gibt's nicht mehr in Qt5
	}
	return QUrl();
}

DataCell& DataCell::setDate( const QDate& d )
{
	clear();
	d_type = TypeDate;
	d_int32 = d.toJulianDay();
	return *this;
}

QDate DataCell::getDate() const
{
    if( d_type == TypeDateTime )
        return getDateTime().date();
	else if( d_type != TypeDate )
		return QDate();
	return QDate::fromJulianDay( d_int32 );
}

DataCell& DataCell::setTime( const QTime& t )
{
	clear();
	d_type = TypeTime;
	d_int32 = _fromTime( t );
	return *this;
}

QTime DataCell::getTime() const
{
    if( d_type == TypeDateTime )
        return getDateTime().time();
	else if( d_type != TypeTime )
		return QTime();
	return _toTime( d_int32 );
}

DataCell& DataCell::setDateTime( QDateTime dt )
{
	clear();
	d_type = TypeDateTime;

    bool isUtc = false;
    switch( dt.timeSpec() )
    {
    case Qt::UTC:
        isUtc = true;
        break;
    case Qt::OffsetFromUTC:
        isUtc = true;
        dt = dt.toUTC();
        break;
    case Qt::LocalTime:
        // NOP
        break;
    default:
        qWarning() << "DataCell::setDateTime: unknown timeSpec";
        break;
    }

	d_pair[1] = dt.date().toJulianDay();
	d_pair[0] = _fromTime( dt.time() );
    if( isUtc )
        d_pair[0] = d_pair[0] | 0x80000000; // Setze MSB für UTC-Kennzeichnung
	return *this;
}

DataCell& DataCell::setTimeSlot( const TimeSlot& ts )
{
	clear();
	if( ts.isValid() )
	{
		d_type = TypeTimeSlot;
		d_pair[0] = ts.d_start;
		d_pair[1] = ts.d_duration;
	}else
		setNull();
	return *this;
}

QDateTime DataCell::getDateTime() const
{
    if( d_type == TypeTime )
        return QDateTime( QDate(), getTime() );
    else if( d_type == TypeDate )
        return QDateTime( getDate(), QTime() );
	else if( d_type != TypeDateTime )
		return QDateTime();

    Qt::TimeSpec ts = Qt::LocalTime;
    if( d_pair[0] & 0x80000000 )
        ts = Qt::UTC;
	return QDateTime( QDate::fromJulianDay( d_pair[1] ), _toTime( d_pair[0] ), ts );
}

TimeSlot DataCell::getTimeSlot() const
{
	if( d_type != TypeTimeSlot )
		return TimeSlot();
	return TimeSlot( quint16(d_pair[0]), quint16(d_pair[1]) );
}

#define _UUID_LEN 16

DataCell& DataCell::setUuid( const QUuid& u )
{
	clear();
	QByteArray buf;
	buf.resize( _UUID_LEN );
	quint32 i = Helper::write( buf.data(), u.data1 );
	i += Helper::write( buf.data() + i, u.data2 );
	i += Helper::write( buf.data() + i, u.data3 );
	Q_ASSERT( i == _UUID_LEN / 2 );
	::memcpy( buf.data() + i, u.data4, 8 );
	setArr( buf );
	d_type = TypeUuid;
	return *this;
}

QUuid DataCell::getUuid() const
{
	if( d_type != TypeUuid )
		return QUuid();

	QByteArray buf = getArr();
	Q_ASSERT( getArr().length() >= _UUID_LEN );
	QUuid u;
	quint32 i = Helper::read( buf.data(), u.data1 );
	i += Helper::read( buf.data() + i, u.data2 );
	i += Helper::read( buf.data() + i, u.data3 );
	::memcpy( u.data4, buf.data() + i, 8 );
	return u;
}
DataCell& DataCell::setImage( const QImage& img )
{
	clear();
#if defined( QT_GUI ) || defined(QT_GUI_LIB)
    QByteArray byteArray;
	QBuffer buffer(&byteArray);
	buffer.open(QIODevice::WriteOnly);
	img.save( &buffer, "PNG" ); // RISK
	buffer.close();
	setArr( byteArray );
	d_type = TypeImg;
#else
	qWarning( "calling DataCell::setImage without Qt Gui support" );
#endif
	return *this;
}

bool DataCell::getImage( QImage& img ) const
{
#if defined( QT_GUI ) || defined(QT_GUI_LIB)
	if( d_type == TypeImg )
	{
		return img.loadFromData( getArr() );
	}
#else
	qWarning( "calling DataCell::getImage without Qt Gui support" );
#endif
	return false;
}

DataCell& DataCell::setPicture( const QPicture& p )
{
	clear();
#if defined( QT_GUI ) || defined(QT_GUI_LIB)
    QByteArray byteArray;
	QBuffer buffer(&byteArray);
	buffer.open(QIODevice::WriteOnly);
	QPicture pic = p;
	pic.save( &buffer );
	buffer.close();
	setArr( byteArray);
	d_type = TypePic;
#else
	qWarning( "calling DataCell::setPicture without Qt Gui support" );
#endif
	return *this;
}

bool DataCell::getPicture( QPicture& pic ) const
{
#if defined( QT_GUI ) || defined(QT_GUI_LIB)
	if( d_type == TypePic )
	{
		QBuffer buf;
		buf.setData( getArr() );
		buf.open( QIODevice::ReadOnly );
		return pic.load( &buf );
	}
#else
	qWarning( "calling DataCell::getPicture without Qt Gui support" );
#endif
	return false;
}

DataCell& DataCell::setBml( const QByteArray& in )
{
	clear();
	setArr( in );
	d_type = TypeBml;
	return *this;
}

int DataCell::getByteCount()
{
	if( typeByteCount[d_type] == UNISTR )
	{
		return getStr().toUtf8().size() + 1;
	}else if( typeByteCount[d_type] == BINARY || typeByteCount[d_type] == CSTRING )
	{
		return getArr().size() + 1;
	}else if( typeByteCount[d_type] == MBYTE64 )
		return 8;
	else if( typeByteCount[d_type] == MBYTE32 )
		return 4;
	else
		typeByteCount[d_type];
}

DataCell& DataCell::setTag( const NameTag& t )
{
	clear();
	d_uint32 = t.d_id;
	Q_ASSERT( sizeof(d_uint32) == NameTag::Size );
	d_type = TypeTag;
	return *this;
}

NameTag DataCell::getTag() const
{
	if( d_type != TypeTag )
		return NameTag::null;
	else
		return d_uint32;
}

static inline void _writeArray( QIODevice* out, DataCell::DataType t, QByteArray str, 
							   bool dataOnly, bool compressed, bool string )
{
	quint32 len = str.length();
	if( string )
	{
		// korrigiere hier, dass QByteArray::fromRawData bei length das Nullzeichen mitzählt.
		if( len > 0 && str[len-1] == char(0) )
			len = ::strlen( str ); 
		// verwende nicht truncate, da dann der Speicher umalloziiert wird
		if( str[len] != char(0) )
			qWarning( "DataCell:_writeArray: string without terminating null" );
		len += 1; 
	}
	if( len <= s_compressionThreshold )
		compressed = false;
	if( compressed )
	{
		// verwende hier nicht direkt QByteArray wegen obigem Problem mit Nullzeichen
		str = qCompress( reinterpret_cast<const uchar*>(str.data()), len, 7 ); // RISK. -1 entspricht 6
		len = str.length();
	}
	if( !dataOnly )
	{
		quint8 sym = DataCell::typeToSym( t );
		if( compressed ) 
			sym |= 0x80;
		Helper::write( out, sym );
		Helper::writeMultibyte32( out, len );
	}
	if( dataOnly && len == 0 )
		Helper::write( out, quint8( 0 ) ); // Damit sicher etwas geschrieben wird
	else
		out->write( str, len );
}

void DataCell::writeCell( QIODevice* out, bool dataOnly, bool compressed ) const
{
	// Falls dataOnly==true, werden die Daten ohne Typ und Counter geschrieben. Dieses
	// Format muss nicht mehr mit readCell gelesen werden, sondern dient z.B. zu Indizierungszwecken.

	Q_ASSERT( out != 0 );
	const DataType t = getType();
	switch( typeByteCount[t] )
	{
	case UNISTR:
		_writeArray( out, t, getStr().toUtf8(), dataOnly, compressed, true );
		break;
	case CSTRING:
		_writeArray( out, t, getArr(), dataOnly, compressed, true );
		break;
	case BINARY:
		_writeArray( out, t, getArr(), dataOnly, compressed, false );
		break;
	default:
		if( !dataOnly )
			Helper::write( out, typeToSym( t ) );
		switch( getType() )
		{
		case TypeNull:
			if( dataOnly )
				Helper::write( out, quint8(0) ); 
			break;
		case TypeTrue:
			if( dataOnly )
				Helper::write( out, quint8(1) );
			break;
		case TypeFalse:
			if( dataOnly )
				Helper::write( out, quint8(0) );
			break;
		case TypeAtom:	
			Helper::write( out, getAtom() );
			break;
		case TypeOid:	
			Helper::writeMultibyte64( out, getOid() );
			break;
		case TypeRid:	
			Helper::writeMultibyte64( out, getRid() );
			break;
		case TypeSid:	
			Helper::writeMultibyte32( out, getSid() );
			break;
		case TypeId32:	
			Helper::writeMultibyte32( out, getId32() );
			break;
		case TypeId64:	
			Helper::writeMultibyte64( out, getId64() );
			break;
		case TypeUInt8:	
			Helper::write( out, getUInt8() );
			break;
		case TypeUInt16:
			Helper::write( out, getUInt16() );
			break;
		case TypeInt32:
			Helper::write( out, getInt32() );
			break;
		case TypeUInt32:
			Helper::write( out, getUInt32() );
			break;
		case TypeInt64:
			Helper::write( out, getInt64() );
			break;
		case TypeUInt64:
			Helper::write( out, getUInt64() );
			break;
		case TypeDouble:
			Helper::write( out, getDouble() );
			break;
        case TypeFloat:
			Helper::write( out, getFloat() );
			break;
		case TypeDate:
			Helper::write( out, d_int32 );
			break;
		case TypeTime:
			Helper::write( out, d_int32 );
			break;
		case TypeDateTime:
            // s_symDateTimeNew:
            Helper::write( out, d_pair[1] ); // Datum
			Helper::write( out, d_pair[0] ); // Zeit
            // s_symDateTimeOld:
//          Helper::write( out, d_pair[0] ); // Zeit
//			Helper::write( out, d_pair[1] ); // Datum
			break;
		case TypeTimeSlot:
			Helper::write( out, quint16( d_pair[0] ) );
			Helper::write( out, quint16( d_pair[1] ) );
			break;
		case TypeTag:
			// Schreibt die Länge nicht
			out->write( (char*)d_buf, NameTag::Size );
			break;
		default:
			throw StreamException( StreamException::IncompleteImplementation,
				"writeCell: type not supported" );
		}
		break;
	}
}

QByteArray DataCell::writeCell(bool dataOnly, bool compressed) const
{
	QBuffer buf;
	buf.open( QIODevice::WriteOnly );
	writeCell( &buf, dataOnly, compressed );
	buf.close();
	return buf.buffer();
}

#ifdef __unused__
static qint64 _read( QIODevice* in, bool peek, char * data, qint64 maxSize )
{
	if( peek )
		return in->peek( data, maxSize );
	else
		return in->read( data, maxSize );
}
#endif

DataCell::Peek DataCell::peekCell(QIODevice* in)
{
	Q_ASSERT( in != 0 );
	char buf[1 + Helper::multiByte64MaxLen];
	if( in->bytesAvailable() < 1 )
		return Peek();

	in->peek( buf, 1 );
	Peek res;
	res.d_type = symToType( buf[0] ); // throws

	if( res.d_type >= TypeInvalid )
		throw StreamException( StreamException::InvalidProtocol, "invalid type" );

	int len = typeByteCount[ res.d_type ];
	switch( len )
	{
	case UNISTR:
	case CSTRING:
	case BINARY:
		{
			// NOTE: auch String wird im Stream mit Anzahl gesendet
			if( in->bytesAvailable() < 1 + 1 ) // Type + Multibyte mind. Länge 1
				return Peek();
			const int count = in->peek( buf, 1 + Helper::multiByte32MaxLen );
			const int n = Helper::peekMultibyte32( buf + 1, count - 1 );
			if( n < 0 )
				return Peek(); // Es fehlen noch Bytes
			Helper::readMultibyte32( buf + 1, res.d_len, n );
			res.d_off = n;
		}
		break;
	case MBYTE64:
		{
			if( in->bytesAvailable() < 1 + 1 ) // Type + Multibyte mind. Länge 1
				return Peek();
			const int count = in->peek( buf, 1 + Helper::multiByte64MaxLen );
			const int n = Helper::peekMultibyte64( buf + 1, count - 1 );
			if( n < 0 )
				return Peek(); // Es fehlen noch Bytes
			res.d_len = n;
		}
		break;
	case MBYTE32:
		{
			if( in->bytesAvailable() < 1 + 1 ) // Type + Multibyte mind. Länge 1
				return Peek();
			const int count = in->peek( buf, 1 + Helper::multiByte32MaxLen );
			const int n = Helper::peekMultibyte32( buf + 1, count - 1 );
			if( n < 0 )
				return Peek(); // Es fehlen noch Bytes
			res.d_len = n;
		}
		break;
	default:
		res.d_len = len;
	}
	return res;
}

#if 0
#include <thirdparty/zlib.h>
static QByteArray myUncompress(const uchar* data, int nbytes)
{
	// Direkte Kopie aus Qt-4.3.5. Diese Routine ist ab Qt 4.4 fehlerhaft
    if (!data) {
        qWarning("qUncompress: Data is null");
        return QByteArray();
    }
    if (nbytes <= 4) {
        if (nbytes < 4 || (data[0]!=0 || data[1]!=0 || data[2]!=0 || data[3]!=0))
            qWarning("qUncompress: Input data is corrupted");
        return QByteArray();
    }
    ulong expectedSize = (data[0] << 24) | (data[1] << 16) |
                       (data[2] <<  8) | (data[3]);
	/* NOTE: Qt setzt in qCompress die Originallänge als 32Bit-Zahl vor den Stream in folgender Weise, die plattformunabhängig ist:
	        bazip.resize(len + 4);
            bazip[0] = (nbytes & 0xff000000) >> 24;
            bazip[1] = (nbytes & 0x00ff0000) >> 16;
            bazip[2] = (nbytes & 0x0000ff00) >> 8;
            bazip[3] = (nbytes & 0x000000ff);
	*/
    ulong len = qMax(expectedSize, 1ul);
    QByteArray baunzip;
    int res;
    do {
        baunzip.resize(len);
        res = ::uncompress((uchar*)baunzip.data(), &len,
                            (uchar*)data+4, nbytes-4);

        switch (res) {
        case Z_OK:
            if ((int)len != baunzip.size())
                baunzip.resize(len);
            break;
        case Z_MEM_ERROR:
            qWarning("qUncompress: Z_MEM_ERROR: Not enough memory");
            break;
        case Z_BUF_ERROR:
            len *= 2;
            break;
        case Z_DATA_ERROR:
            qWarning("qUncompress: Z_DATA_ERROR: Input data is corrupted");
            break;
        }
    } while (res == Z_BUF_ERROR);

    if (res != Z_OK)
        baunzip = QByteArray();

    return baunzip;
}
#endif

long DataCell::readCell( QIODevice* in )
{
	Q_ASSERT( in != 0 );
	const Peek cell = peekCell( in );
	if( in->bytesAvailable() < cell.getCellLength() )
		return -1;
	char typeSym[1];
	in->read( typeSym, 1 );
	DataType type = symToType( typeSym[0] ); // throws
	const bool compressed = symIsCompressed( typeSym[0] );
	if( type < TypeNull || type >= TypeInvalid )
		throw StreamException( StreamException::InvalidProtocol, "readCell: invalid type" );
	if( type == FrameName || type == SlotName )
		type = TypeAtom;
	else if( type == FrameNameStr || type == SlotNameStr )
		type = TypeAscii; 
	else if( type == FrameNameIdx || type == SlotNameIdx )
		type = TypeId32; 
	else if( type == FrameNameTag || type == SlotNameTag )
		type = TypeTag;

	clear(); // lösche this
	d_type = type;

	const int len = typeByteCount[ type ];
	switch( len )
	{
	case UNISTR:
	case CSTRING:
	case BINARY:
		{
			quint32 count = 0;
			Helper::readMultibyte32( in, count );
			QByteArray str = in->read( count );
			if( compressed )
#if 0
				str = myUncompress( reinterpret_cast<const uchar*>(str.constData()), str.size() );
#else
                str = qUncompress(str);
#endif

			if( len == UNISTR )
			{
				setStr( QString::fromUtf8( str ) );
			}else
			{
				assert( len == BINARY || len == CSTRING );
				if( len == CSTRING )
				{
					// Korrigiere hier, dass der gespeicherte String bereits ein Nullzeichen enthält.
					if( count > 0 && str[count-1] == char(0) )
					{
						// Prüfe, ob der String ev. das Opfer von überzähligen Nullzeichen ist, was vor dieser
						// Fehlerbehebung bei mehrmaligem read/write passieren konnte.
						if( count > 1 && str[count-2] == char(0) )
						{
							count = ::strlen( str ) + 1; 
							// Das passiert mit früheren DB-Dateien ziemlich häufig.
							// qWarning( "DataCell::readCell string with more than one terminal null" );
						}
						// Da str hier noch nicht shared ist, macht truncate keine Allokationsänderung; also günstig.
						str.truncate( count-1 );
					}
				}
				setArr( str );
			}
		}	
		break;
	case MBYTE64:
		Helper::readMultibyte64( in, d_uint64 );
		break;
	case MBYTE32:
		Helper::readMultibyte32( in, d_uint32 );
		break;
	default:
		switch( d_type )
		{
		case TypeNull:
		case TypeTrue:
		case TypeFalse:
			break;
		case TypeAtom:	
			Helper::read( in, d_uint32 );
			break;
		case TypeUInt8:	
			Helper::read( in, d_uint8 );
			break;
		case TypeUInt16:
			Helper::read( in, d_uint16 );
			break;
		case TypeInt32:
			Helper::read( in, d_int32 );
			break;
		case TypeUInt32:
			Helper::read( in, d_uint32 );
			break;
		case TypeInt64:
			Helper::read( in, d_int64 );
			break;
		case TypeUInt64:
			Helper::read( in, d_uint64 );
			break;
		case TypeDouble:
			Helper::read( in, d_double );
			break;
        case TypeFloat:
			Helper::read( in, d_float );
			break;
		case TypeDate:
			Helper::read( in, d_int32 );
			break;
		case TypeTime:
			Helper::read( in, d_int32 );
			break;
		case TypeDateTime:
            if( typeSym[0] == s_symDateTimeOld )
            {
                Helper::read( in, d_pair[0] );
                Helper::read( in, d_pair[1] );
            }else
            {
                Helper::read( in, d_pair[1] );
                Helper::read( in, d_pair[0] );
            }
			break;
		case TypeTimeSlot:
			{
				quint16 v;
				Helper::read( in, v );
				d_pair[0] = v;
				Helper::read( in, v );
				d_pair[1] = v;
			}
			break;
		case TypeTag:
			in->read( (char*)d_buf, NameTag::Size );
			break;
		default:
			throw StreamException( StreamException::IncompleteImplementation,
				"readCell: type not supported" );
		}
	}

	return cell.getCellLength();
}

bool DataCell::readCell( const QByteArray& in )
{
	QBuffer buf;
	buf.buffer() = in;
	buf.open( QIODevice::ReadOnly );
	return readCell( &buf ) >= 0;
}

QString DataCell::toString(bool strip_markup) const
{
	switch( d_type )
	{
	case TypeLatin1:
	case TypeAscii:
		return QString::fromLatin1( getArr() );
	case TypeHtml:
    case TypeXml:
        if( strip_markup )
            return stripMarkup( getStr(), false );
            // RISK: 4.3.14 eingeführt, vorher wie TypeXml, am 2.5.14 ersetzt durch 2
        else
            return getStr();
    case TypeString:
		return getStr();
	case TypeBml:
		{
			DataReader r( getBml() );
			return r.extractString();
		}
	case TypeUInt8:	
		return QString::number( getUInt8() );
	case TypeUInt16:
		return QString::number( getUInt16() );
	case TypeInt32:
		return QString::number( getInt32() );
	case TypeUInt32:
		return QString::number( getUInt32() );
	case TypeInt64:
		return QString::number( getInt64() );
	case TypeUInt64:
		return QString::number( getUInt64() );
	case TypeId32:
		return QString::number( getId32() );
	case TypeId64:
		return QString::number( getId64() );
	case TypeDouble:
		return QString::number( getDouble() );
    case TypeFloat:
		return QString::number( getFloat() );
	case TypeDate:
		return getDate().toString(Qt::ISODate);
	case TypeTime:
		return getTime().toString(Qt::ISODate);
	case TypeDateTime:
		return getDateTime().toString(Qt::ISODate);
	case TypeTrue:
		return "true";
	case TypeFalse:
		return "false";
	case TypeUuid:
		return getUuid().toString();
	case TypeUrl:
        return QString::fromLatin1( getArr() ); // fromAscii gibt's nicht mehr in Qt5
	case TypeTag:
		return getTag().toString();
	case TypeNull:
		return QString();
	default:
		qWarning( "DataCell::toString cannot convert type '%s' to QString", typePrettyName[getType()] );
	}
    return QString();
}

quint64 DataCell::toId64() const
{
    switch( d_type )
	{
    case TypeNull:
	case TypeInvalid:
        return 0;
	case TypeAtom:
		return getAtom();
	case TypeOid:
		return getOid();
	case TypeRid:
		return getRid();
	case TypeSid:
		return getSid();
	case TypeId32:
		return getId32();
	case TypeId64:
		return getId64();
	case TypeUInt64:
		return getUInt64();
	default:
		qWarning( "DataCell::toId64 missing type %s", typePrettyName[d_type] );
		throw StreamException( StreamException::IncompleteImplementation,
			"toUint64: type not supported" );
	}
}

QString DataCell::toPrettyString() const
{
	switch( d_type )
	{
	case TypeNull:
		return "null";
	case TypeAtom:	
		return QString( "atom(0x%1)" ).arg( getAtom(), 0, 16 );
	case TypeOid:	
		return QString( "oid(%1)" ).arg( getOid() );
	case TypeRid:	
		return QString( "rid(%1)" ).arg( getRid() );
	case TypeSid:	
		return QString( "sid(%1)" ).arg( getSid() );
	case TypeId32:	
		return QString( "id32(%1)" ).arg( getId32() );
	case TypeId64:	
		return QString( "id64(%1)" ).arg( getId64() );
	case TypeTimeSlot:
		{
			TimeSlot t = getTimeSlot();
			if( t.d_duration )
				return t.getStartTime().toString() + QLatin1Char('-') + t.getEndTime().toString();
			else
				return t.getStartTime().toString();
		}
	case TypeTag:
		return QString( "tag(%1)" ).arg( getTag().toString() );
	case TypeImg:
		return "<image>";
	case TypePic:
		return "<picture>";
	case TypeLob:
		return "<blob>";
	case TypeBml:
		return "<bml>";
	case TypeInvalid:
		return "<invalid>";
	case TypeUrl:
		return getUrl().toString();
	default:
		return toString( true );
	}
}

bool DataCell::checkAscii( const char* str )
{
	while( str && *str )
	{
		if( *str & 0x80 ) // Zeichen > 127
			return false;
		str++;
	}
    return true;
}

static void _markup2text( QString& result, const QString& markup )
{
    QString m = markup.toLower();
    if( m == QLatin1String( "<br>" ) || m == QLatin1String( "</p>" ) ||
            m == QLatin1String( "</tr>" ) )
        result.append( QLatin1String("\n") );
    else if( m.startsWith( QLatin1String( "&#" ) ) )
    {
        QString number = m.mid( 2, m.size() - 3 );
        if( !number.isEmpty() && number[0] == QChar('x') )
            result.append( QChar( number.toInt( 0, 16 ) ) );
        else
            result.append( QChar( number.toInt() ) );
    }else if( m.startsWith( QChar('&') ) )
    {
        QChar res = Helper::resolveEntity( m.mid( 1, m.size() - 2 ) );
        if( !res.isNull() )
            result.append( res );
    }
}

QString DataCell::stripMarkup(const QString & html, bool interpreteMarkup)
{
    QString result;
    result.reserve( html.size() * 0.5 ); // RISK
    enum State { Idle, WithinMarkup, WithinEntity };
    State state = Idle;
    QString markup;
    foreach( QChar c, html )
    {
        switch( state )
        {
        case Idle:
            if( c == QChar('<' ) )
            {
                state = WithinMarkup;
                markup.clear();
                markup += c;
            }else if( c == QChar('&') )
            {
                state = WithinEntity;
                markup.clear();
                markup += c;
            }else
                result += c;
            break;
        case WithinMarkup:
            if( c == QChar('>') )
                state = Idle;
            markup += c;
            if( state == Idle && interpreteMarkup )
                _markup2text( result, markup );
            break;
        case WithinEntity:
            if( c == QChar(';') )
                state = Idle;
            markup += c;
            if( state == Idle && interpreteMarkup )
                _markup2text( result, markup );
            break;
        }
    }
    return result.trimmed();
}

QVariant DataCell::toVariant() const
{
	switch( getType() )
	{
	case TypeNull:
		return QVariant();
	case TypeTrue:
		return true;
	case TypeFalse:
		return false;
	case TypeUInt8:
		return getUInt8();
	case TypeUInt16:
		return getUInt16();
	case TypeInt32:
		return getInt32();
	case TypeUInt32:
		return getUInt32();
	case TypeInt64:
		return getInt64();
	case TypeUInt64:
		return getUInt64();
	case TypeAtom:
		return getAtom();
	case TypeOid:
		return getOid();
	case TypeRid:
		return getRid();
	case TypeSid:
		return getSid();
	case TypeId32:
		return getId32();
	case TypeId64:
		return getId64();
	case TypeDouble:
		return getDouble();
    case TypeFloat:
		return getFloat();
	case TypeLatin1:
	case TypeAscii:
		return QString::fromLatin1( getArr() );
	case TypeHtml:
	case TypeString:
	case TypeXml:	
		return getStr();
	case TypeDate:
		return getDate();
	case TypeTime:
		return getTime();
	case TypeDateTime:
		return getDateTime();
	case TypeUrl:
		return getUrl();
	case TypeImg:
		{
#if defined( QT_GUI ) || defined(QT_GUI_LIB)
			QImage img;
			if( !getImage(img) )
				return QVariant();
			return img;
#else
	qWarning( "calling DataCell::toVariant(TypeImg) without Qt Gui support" );
#endif
		}
		break;
	case TypeLob:
		return getArr();
	case TypeUuid:
		return QVariant::fromValue( getUuid() );
	case TypeTag:
		return getTag().toString();
	case TypeBml:
		return getArr();
	default:
		qWarning( "DataCell::toVariant unknown type %s", typePrettyName[getType()] );
	}
	return QVariant();
}

void DataCell::fromVariant( const QVariant& v )
{
	clear();
	switch( v.type() )
	{
	case QVariant::Invalid:
		break;
	case QVariant::Bool:
		setBool( v.toBool() );
		break;
	case QVariant::ByteArray:
		setLob( v.toByteArray() ); // RISK
		break;
	case QVariant::Char:
		setString( v.toChar() );
		break;
	case QVariant::Date:
		setDate( v.toDate() );
		break;
	case QVariant::DateTime:
		setDateTime( v.toDateTime() );
		break;
	case QVariant::Double:
		setDouble( v.toDouble() );
		break;
	// QVariant::Image, QVariant::Pixmap
	case QVariant::Int:
		setInt32( v.toInt() );
		break;
	case QVariant::LongLong:
		setInt64( v.toLongLong() );
		break;
	case QVariant::String:
		setString( v.toString() );
		break;
	case QVariant::Time:
		setTime( v.toTime() );
		break;
	case QVariant::UInt:
		setUInt32( v.toUInt() );
		break;
	case QVariant::ULongLong:
		setUInt64( v.toULongLong() );
		break;
	case QVariant::Url:
		setUrl( v.toUrl() );
		break;
    default:
        break;
	}
}
