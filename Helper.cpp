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

#include "Helper.h"
#include <Stream/Exceptions.h>
#include <QBuffer>
#include <QtDebug>
#include <QSysInfo>
using namespace Stream;

static const bool s_big = QSysInfo::ByteOrder == QSysInfo::BigEndian;

void Helper::adjustSex(char* ptr, quint32 len )
{
	if( s_big )	// Die Maschine hat schon BigEndian, wir drehen nicht.
		return;
	const quint32 max = len / 2;
	for( quint32 i = 0; i < max; i++ )
	{
		char tmp = ptr[ i ];
		ptr[ i ] = ptr[ len - 1 - i ];
		ptr[ len - 1 - i ] = tmp;
	}
}

quint32 Helper::writeMultibyte29( char* out, quint32 i )
{
	// Wie in amf3_spec_121207.pdf

	if( i <= 0x0000007F )	// 0xxxxxxx
		// 0x0000007F = 1111111
	{
		out[0] = i;
		return 1;
	}else if( i <= 0x00003FFF ) // 1xxxxxxx 0xxxxxxx
		// 0x00003FFF = 1111111 1111111
	{
		out[0] = (i >> 7) & 0x7F | 0x80;
		out[1] = i & 0x7F;
		return 2;
	}else if( i <= 0x001FFFFF ) // 1xxxxxxx 1xxxxxxx 0xxxxxxx
		// 0x001FFFFF = 1111111 1111111 1111111
	{
		out[0] = (i >> 14) & 0x7F | 0x80;
		out[1] = (i >> 7) & 0x7F | 0x80;
		out[2] = i & 0x7F;
		return 3;
	}else if( i <= 0x1FFFFFFF ) // 1xxxxxxx 1xxxxxxx 1xxxxxxx xxxxxxxx
		// Original war 0x3FFFFFFF, was vermutlich falsch ist
		//            0x1FFFFFFF =     1111111 1111111 1111111 11111111
		//            0x3FFFFFFF =   1 1111111 1111111 1111111 11111111
		//            0x20000000 =   1 0000000 0000000 0000000 00000000
	{
		out[0] = (i >> 22) & 0x7F | 0x80;
		out[1] = (i >> 15) & 0x7F | 0x80;
		out[2] = (i >> 8) & 0x7F | 0x80;
		out[3] = i & 0xFF;
		return 4;
	}else
		throw StreamException( StreamException::WrongDataFormat,
			"only 29 significant bits remain for encoding an integer" );
}
quint32 Helper::writeMultibyte29( QIODevice* out, quint32 i )
{
	char buf[multiByte29MaxLen];
	const quint32 res = writeMultibyte29( buf, i );
	out->write( buf, res );
	return res;
}

quint32 Helper::writeMultibyte32( char* out, quint32 i )
{
	// Wie Helper::writeMultibyte, analog erweitert

	if( i <= 0x0000007F )	// 0xxxxxxx
		// 0x0000007F = 1111111
	{
		out[0] = i;
		return 1;
	}else if( i <= 0x00003FFF ) // 1xxxxxxx 0xxxxxxx
		// 0x00003FFF = 1111111 1111111
	{
		out[0] = (i >> 7) & 0x7F | 0x80;
		out[1] = i & 0x7F;
		return 2;
	}else if( i <= 0x001FFFFF ) // 1xxxxxxx 1xxxxxxx 0xxxxxxx
		// 0x001FFFFF = 1111111 1111111 1111111
	{
		out[0] = (i >> 14) & 0x7F | 0x80;
		out[1] = (i >> 7) & 0x7F | 0x80;
		out[2] = i & 0x7F;
		return 3;
	}else if( i <= 0xFFFFFFF ) // 1xxxxxxx 1xxxxxxx 1xxxxxxx 0xxxxxxx
	{
		out[0] = (i >> 21) & 0x7F | 0x80;
		out[1] = (i >> 14) & 0x7F | 0x80;
		out[2] = (i >> 7) & 0x7F | 0x80;
		out[3] = i & 0x7F;
		return 4;
	}else // 1xxxxxxx 1xxxxxxx 1xxxxxxx 1xxxxxxx xxxxxxxx
	{
		out[0] = (i >> 29) & 0x7F | 0x80;
		out[1] = (i >> 22) & 0x7F | 0x80;
		out[2] = (i >> 15) & 0x7F | 0x80;
		out[3] = (i >> 8) & 0x7F | 0x80;
		out[4] = i & 0xFF;
		return 5;
	}
}
quint32 Helper::writeMultibyte32( QIODevice* out, quint32 i )
{
	char buf[multiByte32MaxLen];
	const quint32 res = writeMultibyte32( buf, i );
	out->write( buf, res );
	return res;
}


int Helper::readMultibyte29( QIODevice* in, quint32& out )
{
	char buf[multiByte29MaxLen];
	const int count = in->peek( buf, multiByte29MaxLen );
	if( count < 0 )
		throw StreamException( StreamException::DeviceAccess,
			"cannot read device" );

	const int n = peekMultibyte29( buf, count );
	if( n < 0 )
		return -1;
	in->read( buf, n ); // Konsumiere die Bytes.
	return readMultibyte29( buf, out, n );
}

int Helper::peekMultibyte29( const char* buf, int count )
{
	int n = 0;
	for( int i = 0; i < count && i < ( multiByte29MaxLen - 1 ); i++ )
	{
		if( buf[i] & 0x80 )
			n++; // Byte mit 1xxxxxxx entdeckt
		else
			break; // Erstes Byte mit 0xxxxxxx entdeckt
	}
	if( n < ( multiByte29MaxLen - 1 ) )
	{
		// n==0, 1 oder 2
		if( count < n || ( buf[ n ] & 0x80 ) != 0 )
			return -1; // Es fehlen noch Bytes
	}
	n++;
	if( count < n )
		return -1; // Es fehlen noch Bytes
	return n;
}

int Helper::readMultibyte29( const char* in, quint32& out, int n )
{
	Q_ASSERT( n > 0 || n < multiByte29MaxLen );
    out = 0;
	for( int j = 0; j < n; j++ )
	{
		// 1xxxxxxx 1xxxxxxx 1xxxxxxx xxxxxxxx
		// 1000000 0000000 10000000 (0x200080) nok 
		// 111 1111111 1111111 1111111 10000000 (0xFFFFFF80)
		// 1000000 0000000 01111111 (0x20007F) ok
		// 2^32 - 1 = 0xFFFFFFFF = 111 1111111 1111111 1111111 11111111
		//            0x1FFFFFFF =     1111111 1111111 1111111 11111111
		//            0x20000000 =   1 0000000 0000000 0000000 00000000

		if( j < ( multiByte29MaxLen - 1 ) )
		{
			out <<= 7;
			out |= ( quint8(in[j]) & 0x7F );
		}else
		{
			out <<= 8;
			out |= quint8( in[j] );
		}
	}
	return n;
}

void Helper::test()
{
	char buf[8];
    for( quint32 i = 0; i < 0x1FFFFFFF; i += 10 )
	{
        writeMultibyte29( buf, i );

		const int n = peekMultibyte29( buf );
		Q_ASSERT( n > 0 );
		quint32 out;
		if( readMultibyte29( buf, out, n ) < 0 )
			Q_ASSERT( false );

		if( i != out )
		{
			qDebug( "failed %x %x", i, out );
			Q_ASSERT( false );
		}
	}
	qDebug( "done" );
}

void Helper::test2()
{
    for( quint32 i = 0; i < 0x1FFFFFFF; i += 100 )
	{
		QBuffer buf;
		buf.open( QIODevice::WriteOnly );
		writeMultibyte29( &buf, i );
		buf.close();
		buf.open( QIODevice::ReadOnly );
		quint32 out;
		readMultibyte29( &buf, out );
		if( i != out )
		{
			qDebug( "failed %x %x", i, out );
			Q_ASSERT( false );
		}
	}
	qDebug( "done" );
}

#include "DataWriter.h"
#include "DataReader.h"
#include <QFile>
#include "NameTag.h"
#include "DataCell.h"

void Helper::test3()
{
	QFile buf("test.txt");
	buf.open( QIODevice::WriteOnly );

	DataWriter out( &buf );

	out.startFrame( NameTag("ABC") );
	out.writeSlot( DataCell().setLatin1( "Dies ist ein Test" ), NameTag( "TST" ) );
	out.startFrame();
	out.writeSlot( DataCell().setDateTime( QDateTime::currentDateTime() ) );
	out.endFrame();
	out.endFrame();

	buf.close();
	buf.open( QIODevice::ReadOnly );

	DataReader in( &buf );

	while( !buf.atEnd() )
	{
		switch( in.nextToken() )
		{
		case DataReader::Pending:
			qDebug() << "Pending";
			break;
		case DataReader::BeginFrame:
			if( !in.getName().isNull() )
				qDebug() << QString( "Frame [%1]" ).arg( in.getName().toPrettyString() );
			else
				qDebug() << "Frame";
			break;
		case DataReader::EndFrame:
			qDebug() << "End Frame";
			break;
		case DataReader::Slot:
			{
				DataCell val;
				in.readValue( val );
				if( !in.getName().isNull() )
					qDebug() << QString( "Slot [%1] = %2" ).
						arg( in.getName().toPrettyString() ).arg( val.toPrettyString() );
				else
					qDebug() << QString( "Slot = %1" ).arg( val.toPrettyString() );
			}
			break;
		}
	}
	qDebug( "done" );
}

quint32 Helper::writeMultibyte64( char* out, quint64 i )
{
	// Wie in amf3_spec_121207.pdf

	if( i <= 0x0000007F )	// 0xxxxxxx
		// 0x0000007F = 1111111
	{
		out[0] = i;
		return 1;
	}else if( i <= 0x00003FFF ) // 1xxxxxxx 0xxxxxxx
		// 0x00003FFF = 1111111 1111111
	{
		out[0] = (i >> 7) & 0x7F | 0x80;
		out[1] = i & 0x7F;
		return 2;
	}else if( i <= 0x001FFFFF ) // 1xxxxxxx 1xxxxxxx 0xxxxxxx
		// 0x001FFFFF = 1111111 1111111 1111111
	{
		out[0] = (i >> 14) & 0x7F | 0x80;
		out[1] = (i >> 7) & 0x7F | 0x80;
		out[2] = i & 0x7F;
		return 3;
	}else if( i <= 0xFFFFFFF ) // 1xxxxxxx 1xxxxxxx 1xxxxxxx 0xxxxxxx
	{
		out[0] = (i >> 21) & 0x7F | 0x80;
		out[1] = (i >> 14) & 0x7F | 0x80;
		out[2] = (i >> 7) & 0x7F | 0x80;
		out[3] = i & 0x7F;
		return 4;
	}else if( i <= 0x7FFFFFFFFL ) // 1xxxxxxx 1xxxxxxx 1xxxxxxx 1xxxxxxx 0xxxxxxx
	{
		out[0] = (i >> 28) & 0x7F | 0x80;
		out[1] = (i >> 21) & 0x7F | 0x80;
		out[2] = (i >> 14) & 0x7F | 0x80;
		out[3] = (i >> 7) & 0x7F | 0x80;
		out[4] = i & 0x7F;
		return 5;
	}else if( i <= 0x3FFFFFFFFFFL ) // 1xxxxxxx 1xxxxxxx 1xxxxxxx 1xxxxxxx 1xxxxxxx 0xxxxxxx
	{
		out[0] = (i >> 35) & 0x7F | 0x80;
		out[1] = (i >> 28) & 0x7F | 0x80;
		out[2] = (i >> 21) & 0x7F | 0x80;
		out[3] = (i >> 14) & 0x7F | 0x80;
		out[4] = (i >> 7) & 0x7F | 0x80;
		out[5] = i & 0x7F;
		return 6;

	}else if( i <= 0x1FFFFFFFFFFFFL ) // 1xxxxxxx 1xxxxxxx 1xxxxxxx 1xxxxxxx 1xxxxxxx 1xxxxxxx 0xxxxxxx
	{
		out[0] = (i >> 42) & 0x7F | 0x80;
		out[1] = (i >> 35) & 0x7F | 0x80;
		out[2] = (i >> 28) & 0x7F | 0x80;
		out[3] = (i >> 21) & 0x7F | 0x80;
		out[4] = (i >> 14) & 0x7F | 0x80;
		out[5] = (i >> 7) & 0x7F | 0x80;
		out[6] = i & 0x7F;
		return 7;
	}else if( i <= 0x1FFFFFFFFFFFFFFL ) // Für Mac GCC 4.0.1 muss hier LLU stehen
		// 1xxxxxxx 1xxxxxxx 1xxxxxxx 1xxxxxxx 1xxxxxxx 1xxxxxxx 1xxxxxxx 0xxxxxxx
	{
		out[0] = (i >> 49) & 0x7F | 0x80;
		out[1] = (i >> 42) & 0x7F | 0x80;
		out[2] = (i >> 35) & 0x7F | 0x80;
		out[3] = (i >> 28) & 0x7F | 0x80;
		out[4] = (i >> 21) & 0x7F | 0x80;
		out[5] = (i >> 14) & 0x7F | 0x80;
		out[6] = (i >> 7) & 0x7F | 0x80;
		out[7] = i & 0x7F;
		return 8;
	}else 
		// 1xxxxxxx 1xxxxxxx 1xxxxxxx 1xxxxxxx 1xxxxxxx 1xxxxxxx 1xxxxxxx 1xxxxxxx xxxxxxxx
	{
		out[0] = (i >> 57) & 0x7F | 0x80;
		out[1] = (i >> 50) & 0x7F | 0x80;
		out[2] = (i >> 43) & 0x7F | 0x80;
		out[3] = (i >> 36) & 0x7F | 0x80;
		out[4] = (i >> 29) & 0x7F | 0x80;
		out[5] = (i >> 22) & 0x7F | 0x80;
		out[6] = (i >> 15) & 0x7F | 0x80;
		out[7] = (i >> 8) & 0x7F | 0x80;
		out[8] = i & 0xFF;
		return 9;
	}
}
quint32 Helper::writeMultibyte64( QIODevice* out, quint64 i )
{
	char buf[multiByte64MaxLen];
	const quint32 res = writeMultibyte64( buf, i );
	out->write( buf, res );
	return res;
}

int Helper::readMultibyte64( QIODevice* in, quint64& out )
{
	char buf[multiByte64MaxLen];
	const int count = in->peek( buf, multiByte64MaxLen );
	if( count < 0 )
		throw StreamException( StreamException::DeviceAccess,
			"cannot read device" );

	const int n = peekMultibyte64( buf, count );
	if( n < 0 )
		return -1;
	in->read( buf, n ); // Konsumiere die Bytes.
	return readMultibyte64( buf, out, n );
}

int Helper::peekMultibyte64( const char* buf, int count )
{
	int n = 0;
	for( int i = 0; i < count && i < ( multiByte64MaxLen - 1 ); i++ )
	{
		if( buf[i] & 0x80 )
			n++; // Byte mit 1xxxxxxx entdeckt
		else
			break; // Erstes Byte mit 0xxxxxxx entdeckt
	}
	if( n < ( multiByte64MaxLen - 1 ) )
	{
		if( count < n || ( buf[ n ] & 0x80 ) != 0 )
			return -1; // Es fehlen noch Bytes
	}
	n++;
	if( count < n )
		return -1; // Es fehlen noch Bytes
	return n;
}

int Helper::readMultibyte64( const char* in, quint64& out, int n )
{
	Q_ASSERT( n > 0 || n < multiByte64MaxLen );
    out = 0;
	for( int j = 0; j < n; j++ )
	{
		if( j < ( multiByte64MaxLen - 1 ) )
		{
			out <<= 7;
			out |= ( quint8(in[j]) & 0x7F );
		}else
		{
			out <<= 8;
			out |= quint8( in[j] );
		}
	}
	return n;
}

int Helper::readMultibyte32( QIODevice* in, quint32& out )
{
	char buf[multiByte32MaxLen];
	const int count = in->peek( buf, multiByte32MaxLen );
	if( count < 0 )
		throw StreamException( StreamException::DeviceAccess,
			"cannot read device" );

	const int n = peekMultibyte32( buf, count );
	if( n < 0 )
		return -1;
	in->read( buf, n ); // Konsumiere die Bytes.
	return readMultibyte32( buf, out, n );
}

int Helper::peekMultibyte32( const char* buf, int count )
{
	int n = 0;
	for( int i = 0; i < count && i < ( multiByte32MaxLen - 1 ); i++ )
	{
		if( buf[i] & 0x80 )
			n++; // Byte mit 1xxxxxxx entdeckt
		else
			break; // Erstes Byte mit 0xxxxxxx entdeckt
	}
	if( n < ( multiByte32MaxLen - 1 ) )
	{
		// n==0, 1 oder 2
		if( count < n || ( buf[ n ] & 0x80 ) != 0 )
			return -1; // Es fehlen noch Bytes
	}
	n++;
	if( count < n )
		return -1; // Es fehlen noch Bytes
	return n;
}

int Helper::readMultibyte32( const char* in, quint32& out, int n )
{
	Q_ASSERT( n > 0 || n < multiByte32MaxLen );
    out = 0;
	for( int j = 0; j < n; j++ )
	{
		if( j < ( multiByte32MaxLen - 1 ) )
		{
			out <<= 7;
			out |= ( quint8(in[j]) & 0x7F );
		}else
		{
			out <<= 8;
			out |= quint8( in[j] );
		}
	}
	return n;
}

// Folgender Code aus Qt4.4 qtexthtmlparser.cpp

static bool operator<(const QString &entityStr, const Helper::HtmlEntity &entity)
{
    return entityStr < QLatin1String(entity.name);
}

static bool operator<(const Helper::HtmlEntity &entity, const QString &entityStr)
{
    return QLatin1String(entity.name) < entityStr;
}

QChar Helper::resolveEntity(const QString &entity)
{
    const HtmlEntity *start = &s_entities[0];
    const HtmlEntity *end = &s_entities[MAX_ENTITY];
    const HtmlEntity *e = qBinaryFind(start, end, entity);
    if (e == end)
        return QChar();
    return e->code;
}
const Helper::HtmlEntity Helper::s_entities[MAX_ENTITY] = {
    { "aelig", 0x00c6 },
    { "amp", 38 },
    { "aacute", 0x00c1 },
    { "acirc", 0x00c2 },
    { "agrave", 0x00c0 },
    { "alpha", 0x0391 },
    { "aring", 0x00c5 },
    { "atilde", 0x00c3 },
    { "auml", 0x00c4 },
    { "beta", 0x0392 },
    { "ccedil", 0x00c7 },
    { "chi", 0x03a7 },
    { "dagger", 0x2021 },
    { "delta", 0x0394 },
    { "eth", 0x00d0 },
    { "eacute", 0x00c9 },
    { "ecirc", 0x00ca },
    { "egrave", 0x00c8 },
    { "epsilon", 0x0395 },
    { "eta", 0x0397 },
    { "euml", 0x00cb },
    { "gt", 62 },
    { "gamma", 0x0393 },
    { "iacute", 0x00cd },
    { "icirc", 0x00ce },
    { "igrave", 0x00cc },
    { "iota", 0x0399 },
    { "iuml", 0x00cf },
    { "kappa", 0x039a },
    { "lt", 60 },
    { "lambda", 0x039b },
    { "mu", 0x039c },
    { "ntilde", 0x00d1 },
    { "nu", 0x039d },
    { "oelig", 0x0152 },
    { "oacute", 0x00d3 },
    { "ocirc", 0x00d4 },
    { "ograve", 0x00d2 },
    { "omega", 0x03a9 },
    { "omicron", 0x039f },
    { "oslash", 0x00d8 },
    { "otilde", 0x00d5 },
    { "ouml", 0x00d6 },
    { "phi", 0x03a6 },
    { "pi", 0x03a0 },
    { "prime", 0x2033 },
    { "psi", 0x03a8 },
    { "quot", 34 },
    { "rho", 0x03a1 },
    { "scaron", 0x0160 },
    { "sigma", 0x03a3 },
    { "thorn", 0x00de },
    { "tau", 0x03a4 },
    { "theta", 0x0398 },
    { "uacute", 0x00da },
    { "ucirc", 0x00db },
    { "ugrave", 0x00d9 },
    { "upsilon", 0x03a5 },
    { "uuml", 0x00dc },
    { "xi", 0x039e },
    { "yacute", 0x00dd },
    { "yuml", 0x0178 },
    { "zeta", 0x0396 },
    { "aacute", 0x00e1 },
    { "acirc", 0x00e2 },
    { "acute", 0x00b4 },
    { "aelig", 0x00e6 },
    { "agrave", 0x00e0 },
    { "alefsym", 0x2135 },
    { "alpha", 0x03b1 },
    { "amp", 38 },
    { "and", 0x22a5 },
    { "ang", 0x2220 },
    { "apos", 0x0027 },
    { "aring", 0x00e5 },
    { "asymp", 0x2248 },
    { "atilde", 0x00e3 },
    { "auml", 0x00e4 },
    { "bdquo", 0x201e },
    { "beta", 0x03b2 },
    { "brvbar", 0x00a6 },
    { "bull", 0x2022 },
    { "cap", 0x2229 },
    { "ccedil", 0x00e7 },
    { "cedil", 0x00b8 },
    { "cent", 0x00a2 },
    { "chi", 0x03c7 },
    { "circ", 0x02c6 },
    { "clubs", 0x2663 },
    { "cong", 0x2245 },
    { "copy", 0x00a9 },
    { "crarr", 0x21b5 },
    { "cup", 0x222a },
    { "curren", 0x00a4 },
    { "darr", 0x21d3 },
    { "dagger", 0x2020 },
    { "darr", 0x2193 },
    { "deg", 0x00b0 },
    { "delta", 0x03b4 },
    { "diams", 0x2666 },
    { "divide", 0x00f7 },
    { "eacute", 0x00e9 },
    { "ecirc", 0x00ea },
    { "egrave", 0x00e8 },
    { "empty", 0x2205 },
    { "emsp", 0x2003 },
    { "ensp", 0x2002 },
    { "epsilon", 0x03b5 },
    { "equiv", 0x2261 },
    { "eta", 0x03b7 },
    { "eth", 0x00f0 },
    { "euml", 0x00eb },
    { "euro", 0x20ac },
    { "exist", 0x2203 },
    { "fnof", 0x0192 },
    { "forall", 0x2200 },
    { "frac12", 0x00bd },
    { "frac14", 0x00bc },
    { "frac34", 0x00be },
    { "frasl", 0x2044 },
    { "gamma", 0x03b3 },
    { "ge", 0x2265 },
    { "gt", 62 },
    { "harr", 0x21d4 },
    { "harr", 0x2194 },
    { "hearts", 0x2665 },
    { "hellip", 0x2026 },
    { "iacute", 0x00ed },
    { "icirc", 0x00ee },
    { "iexcl", 0x00a1 },
    { "igrave", 0x00ec },
    { "image", 0x2111 },
    { "infin", 0x221e },
    { "int", 0x222b },
    { "iota", 0x03b9 },
    { "iquest", 0x00bf },
    { "isin", 0x2208 },
    { "iuml", 0x00ef },
    { "kappa", 0x03ba },
    { "larr", 0x21d0 },
    { "lambda", 0x03bb },
    { "lang", 0x2329 },
    { "laquo", 0x00ab },
    { "larr", 0x2190 },
    { "lceil", 0x2308 },
    { "ldquo", 0x201c },
    { "le", 0x2264 },
    { "lfloor", 0x230a },
    { "lowast", 0x2217 },
    { "loz", 0x25ca },
    { "lrm", 0x200e },
    { "lsaquo", 0x2039 },
    { "lsquo", 0x2018 },
    { "lt", 60 },
    { "macr", 0x00af },
    { "mdash", 0x2014 },
    { "micro", 0x00b5 },
    { "middot", 0x00b7 },
    { "minus", 0x2212 },
    { "mu", 0x03bc },
    { "nabla", 0x2207 },
    { "nbsp", 0x00a0 },
    { "ndash", 0x2013 },
    { "ne", 0x2260 },
    { "ni", 0x220b },
    { "not", 0x00ac },
    { "notin", 0x2209 },
    { "nsub", 0x2284 },
    { "ntilde", 0x00f1 },
    { "nu", 0x03bd },
    { "oacute", 0x00f3 },
    { "ocirc", 0x00f4 },
    { "oelig", 0x0153 },
    { "ograve", 0x00f2 },
    { "oline", 0x203e },
    { "omega", 0x03c9 },
    { "omicron", 0x03bf },
    { "oplus", 0x2295 },
    { "or", 0x22a6 },
    { "ordf", 0x00aa },
    { "ordm", 0x00ba },
    { "oslash", 0x00f8 },
    { "otilde", 0x00f5 },
    { "otimes", 0x2297 },
    { "ouml", 0x00f6 },
    { "para", 0x00b6 },
    { "part", 0x2202 },
    { "percnt", 0x0025 },
    { "permil", 0x2030 },
    { "perp", 0x22a5 },
    { "phi", 0x03c6 },
    { "pi", 0x03c0 },
    { "piv", 0x03d6 },
    { "plusmn", 0x00b1 },
    { "pound", 0x00a3 },
    { "prime", 0x2032 },
    { "prod", 0x220f },
    { "prop", 0x221d },
    { "psi", 0x03c8 },
    { "quot", 34 },
    { "rarr", 0x21d2 },
    { "radic", 0x221a },
    { "rang", 0x232a },
    { "raquo", 0x00bb },
    { "rarr", 0x2192 },
    { "rceil", 0x2309 },
    { "rdquo", 0x201d },
    { "real", 0x211c },
    { "reg", 0x00ae },
    { "rfloor", 0x230b },
    { "rho", 0x03c1 },
    { "rlm", 0x200f },
    { "rsaquo", 0x203a },
    { "rsquo", 0x2019 },
    { "sbquo", 0x201a },
    { "scaron", 0x0161 },
    { "sdot", 0x22c5 },
    { "sect", 0x00a7 },
    { "shy", 0x00ad },
    { "sigma", 0x03c3 },
    { "sigmaf", 0x03c2 },
    { "sim", 0x223c },
    { "spades", 0x2660 },
    { "sub", 0x2282 },
    { "sube", 0x2286 },
    { "sum", 0x2211 },
    { "sup", 0x2283 },
    { "sup1", 0x00b9 },
    { "sup2", 0x00b2 },
    { "sup3", 0x00b3 },
    { "supe", 0x2287 },
    { "szlig", 0x00df },
    { "tau", 0x03c4 },
    { "there4", 0x2234 },
    { "theta", 0x03b8 },
    { "thetasym", 0x03d1 },
    { "thinsp", 0x2009 },
    { "thorn", 0x00fe },
    { "tilde", 0x02dc },
    { "times", 0x00d7 },
    { "trade", 0x2122 },
    { "uarr", 0x21d1 },
    { "uacute", 0x00fa },
    { "uarr", 0x2191 },
    { "ucirc", 0x00fb },
    { "ugrave", 0x00f9 },
    { "uml", 0x00a8 },
    { "upsih", 0x03d2 },
    { "upsilon", 0x03c5 },
    { "uuml", 0x00fc },
    { "weierp", 0x2118 },
    { "xi", 0x03be },
    { "yacute", 0x00fd },
    { "yen", 0x00a5 },
    { "yuml", 0x00ff },
    { "zeta", 0x03b6 },
    { "zwj", 0x200d },
    { "zwnj", 0x200c },
};

