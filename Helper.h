#ifndef __stream_streamhelper__
#define __stream_streamhelper__

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

#include <QIODevice>

namespace Stream
{
	class Helper
	{
	public:
		enum { 
			multiByte29MaxLen = sizeof( quint32 ),
			multiByte32MaxLen = sizeof( quint32 ) + 1,
			multiByte64MaxLen = sizeof( quint64 ) + 1
		};

		template<class T>
		static quint32 read( QIODevice* in, T& i )
		{
			in->read( (char*)(&i), sizeof(T) );
			adjustSex( (char*)(&i), sizeof(T) );
			return sizeof(T);
		}

		template<class T>
		static quint32 read( const char* ptr, T& i )
		{
			// RISK: Diese Routine geht davon aus, dass ptr auf der richtigen
			// Position steht und noch auf mindestens sizeof(T) Bytes zeigt.
			::memcpy( (char*)(&i), ptr, sizeof(T) );
			adjustSex( (char*)(&i), sizeof(T) );
			return sizeof(T);
		}

		static int readMultibyte29( QIODevice* in, quint32& i );
		static int readMultibyte29( const char* in, quint32& i, int n );
		static int peekMultibyte29( const char* in, int len = 999 );
		static int readMultibyte32( QIODevice* in, quint32& i );
		static int readMultibyte32( const char *in, quint32& i, int n );
		static int peekMultibyte32( const char* in, int len = 999 );
		static int readMultibyte64( QIODevice* in, quint64& i );
		static int readMultibyte64( const char* in, quint64& i, int n );
		static int peekMultibyte64( const char* in, int len = 999 );

		template<class T>
		static quint32 write( QIODevice* out, T i )
		{
			char buf[sizeof(T)];
			::memcpy( buf, (char*)(&i), sizeof(T) );
			adjustSex( buf, sizeof(T) );
			out->write( buf, sizeof(T) );
			return sizeof(T);
		}

		template<class T>
		static quint32 write( char* out, T i )
		{
			// RISK: Diese Routine geht davon aus, dass noch mindestens sizeof(T) Bytes vorhanden.
			::memcpy( out, (char*)(&i), sizeof(T) );
			adjustSex( out, sizeof(T) );
			return sizeof(T);
		}

		static quint32 writeMultibyte29( char* out, quint32 i ); 
		static quint32 writeMultibyte29( QIODevice* out, quint32 i ); 
		static quint32 writeMultibyte32( char* out, quint32 i );
		static quint32 writeMultibyte32( QIODevice* out, quint32 i );
		static quint32 writeMultibyte64( char* out, quint64 i );
		static quint32 writeMultibyte64( QIODevice* out, quint64 i );

		static void adjustSex( char* ptr, quint32 len );

		static void test();
		static void test2();
		static void test3();

        struct HtmlEntity { const char *name; quint16 code; };
        enum { MAX_ENTITY = 258 };
        static const HtmlEntity s_entities[MAX_ENTITY];
        static QChar resolveEntity(const QString &entity);
	private:
		Helper() {}
	};
}

#endif
