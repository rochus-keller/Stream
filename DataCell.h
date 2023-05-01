#ifndef __stream_datacell__
#define __stream_datacell__

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

#include <Stream/NameTag.h>
#include <Stream/TimeSlot.h>
#include <Stream/Exceptions.h>
#include <QString>
#include <QIODevice>
#include <QDateTime>
#include <QUrl>
#include <QUuid>
#include <QVariant>
#if defined( QT_GUI ) || defined(QT_GUI_LIB)
#include <QImage>
#include <QPicture>
#endif

namespace Stream
{
	class DataCell
	{
	public:
		static const char* bmlMimeType;
		static const quint32 null; 
		typedef quint32 Atom;
		typedef quint64 OID;

		enum DataType
		{
			TypeNull,
			TypeTrue,
			TypeFalse,
			TypeAtom,	// Codierter 8-Bit-ANSI-String, für Feldnamen/Elemente verwendet
			TypeOid,	// 64-Bit-Objekt-ID
			TypeRid,	// 64-Bit-Relation-ID
			TypeSid,	// 32-Bit-Stream-ID, Multibyte
			TypeId32,	// Generische unsigned 32-Bit-ID als Multibyte gespeichert 
			TypeId64,	// Generische unsigned 64-Bit-ID als Multibyte gespeichert 
			TypeUInt8,	
			TypeUInt16,
			TypeInt32,
			TypeUInt32,
			TypeInt64,
			TypeUInt64,
			TypeDouble,
            TypeFloat,
			TypeLatin1,	// Nullterminierter String, Latin1 im Memory und Stream
			TypeAscii,	// Nullterminierter String, 7-Bit-Ascii im Memory und Stream
			TypeString,	// Nullterminierter String, Memory als QString, gespeichert als Utf8
			TypeLob,	// ByteArray, Inhalt nicht interpretiert
			TypeBml,	// BML-Stream, wie TypeLob
			TypeDate,
			TypeTime,
			TypeDateTime,
			TypeTimeSlot,
			TypeUrl,	// Intern QUrl, gespeichert als encoded Ascii
			TypeImg,	// Qt-Image-Format, alle unterstützten Formate, wie TypeLob
			TypePic,	// Qt-Picture-Format, wie TypeLob
			TypeUuid,	// QUuid, gespeichert als ByteArray _UUID_LEN of data1..data4
			TypeHtml,	// Wie TypeString
			TypeXml,	// Wie TypeString
			TypeTag,

			MaxType,

			FrameStart,
			FrameName,	// Name ist Atom
			FrameNameStr,// Name ist ASCII-String
			FrameNameIdx,// Name ist TypeId32-Index in die implizite Stringtabelle des BML
			FrameNameTag, // Name ist Tag
			FrameEnd,
			SlotName,	// Name ist Atom
			SlotNameStr, // Name ist ASCII-String
			SlotNameIdx,// Name ist TypeId32-Index in die implizite Stringtabelle des BML
			SlotNameTag, // Name ist Tag

			TypeInvalid
		};
		static const int UNISTR; 
		static const int CSTRING; 
		static const int BINARY; 
		static const int MBYTE64;		
		static const int MBYTE32;		
		static const int typeByteCount[];
		static const char* typePrettyName[];

        bool isTime() const { return d_type == TypeTime; }
		bool isDateTime() const { return d_type == TypeDateTime; }
		bool isTimeSlot() const { return d_type == TypeTimeSlot; }
		bool isDate() const { return d_type == TypeDate; }
		bool hasValue() const { return isValid() && !isNull(); }
		bool isValid() const { return d_type != TypeInvalid; }
		bool isNull() const { return d_type == TypeNull; }
		bool isLatin1() const { return d_type == TypeLatin1; }
		bool isUtf8() const { return d_type == TypeString; }
		bool isString() const { return d_type == TypeString; }
		bool isBml() const { return d_type == TypeBml; }
		bool isHtml() const { return d_type == TypeHtml; }
		bool isXml() const { return d_type == TypeXml; }
		bool isBool() const { return d_type == TypeTrue || d_type == TypeFalse; }
		bool isTrue() const { return d_type == TypeTrue; }
		bool isInt32() const { return d_type == TypeInt32; }
		bool isUInt32() const { return d_type == TypeUInt32; }
		bool isUuid() const { return d_type == TypeUuid; }
		bool isImg() const { return d_type == TypeImg; }
		bool isOid() const { return d_type == TypeOid; }
		bool isRid() const { return d_type == TypeRid; }
		bool isSid() const { return d_type == TypeSid; }
		bool isAtom() const { return d_type == TypeAtom; }
		bool isTag() const { return d_type == TypeTag; }
		bool isStr() const { return typeByteCount[d_type] == UNISTR; }
		bool isCStr() const { return typeByteCount[d_type] == CSTRING; }
		bool isArr() const { return typeByteCount[d_type] == CSTRING || 
			typeByteCount[d_type] == BINARY; }
		int getByteCount();


		DataCell& setTag( const NameTag& );
		NameTag getTag() const;
		DataCell& setAtom( Atom l )
		{
			clear();
			d_type = TypeAtom;
			d_uint32 = l;
			return *this;
		}
		Atom getAtom() const { return ( d_type == TypeAtom )?d_uint32:0; }
		DataCell& setId64( quint64 l )
		{
			clear();
			d_type = TypeId64;
			d_uint64 = l;
			return *this;
		}
		quint64 getId64() const { return ( d_type == TypeId64 )?d_uint64:0; }
		DataCell& setId32( quint32 l )
		{
			clear();
			d_type = TypeId32;
			d_uint32 = l;
			return *this;
		}
		quint32 getId32() const { return ( d_type == TypeId32 )?d_uint32:0; }
		DataCell& setSid( quint32 l )
		{
			clear();
			d_type = TypeSid;
			d_uint32 = l;
			return *this;
		}
		quint32 getSid() const { return ( d_type == TypeSid )?d_uint32:0; }
		DataCell& setRid( OID l )
		{
			clear();
			d_type = TypeRid;
			d_uint64 = l;
			return *this;
		}
		OID getRid() const { return ( d_type == TypeRid )?d_uint64:0; }
		DataCell& setOid( OID l )
		{
			clear();
			d_type = TypeOid;
			d_uint64 = l;
			return *this;
		}
		OID getOid() const { return ( d_type == TypeOid )?d_uint64:0; }
		DataCell& setBml( const QByteArray& );
		QByteArray getBml() const { return ( d_type == TypeBml )?getArr():QByteArray(); }
		DataCell& setPicture( const QPicture& );
		bool getPicture( QPicture& ) const;
		DataCell& setImage( const QImage& );
		bool getImage( QImage& ) const;
		DataCell& setUuid( const QUuid& );
		QUuid getUuid() const;
		DataCell& setDateTime( QDateTime );
		QDateTime getDateTime() const;
		DataCell& setTimeSlot( const TimeSlot& ts );
		TimeSlot getTimeSlot() const;
		DataCell& setDate( const QDate& );
		QDate getDate() const;
		DataCell& setTime( const QTime& );
		QTime getTime() const;
		DataCell& setUInt8( quint8 l )
		{
			clear();
			d_type = TypeUInt8;
			d_uint8 = l;
			return *this;
		}
		quint8 getUInt8() const { return ( d_type == TypeUInt8 )?d_uint8:0; }
		DataCell& setUInt16( quint16 l )
		{
			clear();
			d_type = TypeUInt16;
			d_uint16 = l;
			return *this;
		}
		quint16 getUInt16() const { return ( d_type == TypeUInt16 )?d_uint16:0; }
		DataCell& setUInt64( qint64 l )
		{
			clear();
			d_type = TypeUInt64;
			d_uint64 = l;
			return *this;
		}
		quint64 getUInt64() const { return ( d_type == TypeUInt64 )?d_uint64:0; }
		DataCell& setInt64( qint64 l )
		{
			clear();
			d_type = TypeInt64;
			d_int64 = l;
			return *this;
		}
		qint64 getInt64() const { return ( d_type == TypeInt64 )?d_int64:0; }
		DataCell& setUInt32( quint32 l )
		{
			clear();
			d_type = TypeUInt32;
			d_uint32 = l;
			return *this;
		}
		quint32 getUInt32() const { return ( d_type == TypeUInt32 )?d_uint32:0; }
		DataCell& setInt32( qint32 l )
		{
			clear();
			d_type = TypeInt32;
			d_int32 = l;
			return *this;
		}
		qint32 getInt32() const { return ( d_type == TypeInt32 )?d_int32:0; }
		DataCell& setDouble( double d )
		{
			clear();
			d_type = TypeDouble;
			d_double = d;
			return *this;
		}
        DataCell& setFloat( float f )
		{
			clear();
			d_type = TypeFloat;
			d_float = f;
			return *this;
		}
		double getDouble() const { return ( d_type == TypeDouble )?d_double:0.0; }
        float getFloat() const { return ( d_type == TypeFloat )?d_float:0.0; }
		DataCell& setUrl( const QByteArray& encoded );
		DataCell& setUrl( const QString& unencoded );
		DataCell& setUrl( const QUrl& );
		QUrl getUrl() const; // Alternativ ist getArr() verfügbar
		DataCell& setBool( bool b )
		{
			clear();
			d_type = (b)?TypeTrue:TypeFalse;
			return *this;
		}
		bool getBool() const { return ( d_type == TypeTrue || d_type == TypeFalse )?d_type == TypeTrue:false; }
		DataCell& setLob( const QByteArray& str, bool nullIfEmpty = true );
		DataCell& setHtml( const QString& str, bool nullIfEmpty = true );
		DataCell& setXml( const QString& str, bool nullIfEmpty = true );
		DataCell& setString( const QString& str, bool nullIfEmpty = true );
		DataCell& setLatin1( const QByteArray& str, bool nullIfEmpty = true );
		DataCell& setAscii( const QByteArray& str, bool nullIfEmpty = true );
		DataCell& setNull();

		// NOTE: setter geben DataCell& zurück, damit DataCell().setXY(..) als Parameter funktioniert.

		QString getStr() const;
		QByteArray getArr() const;

		// Konvertierungstroutinen
		QString toPrettyString() const;
		QString toString( bool strip_markup = false) const;
        quint64 toId64() const;
		QVariant toVariant() const;
		void fromVariant( const QVariant& );

		DataCell& operator=( const DataCell& rhs ) { assign( rhs ); return *this; }
		void assign( const DataCell& rhs );
		bool equals( const DataCell& rhs ) const;
		bool operator==( const DataCell& rhs ) const { return equals( rhs ); }

		void clear();
		DataType getType() const { return (DataType)d_type; }
		QByteArray getTypeName() const { return typePrettyName[d_type]; }

		DataCell():d_type( TypeInvalid ) { d_uint64 = 0; }
		DataCell( const DataCell& rhs ):d_type( TypeInvalid ) { d_uint64 = 0; assign( rhs ); }
		~DataCell() { clear(); }

		static DataType symToType( quint8 sym );
		static bool symIsCompressed( quint8 sym );
		static quint8 typeToSym( DataType );
		// dataOnly..ohne type und len
		// compressed..Wert wird komprimiert gespeichert (nur Strings und Binaries und > 64)
		void writeCell( QIODevice*, bool dataOnly = false, bool compressed = false ) const; 
		QByteArray writeCell( bool dataOnly = false, bool compressed = false ) const; // Abgekürzte Version mit Buffer
		long readCell( QIODevice* ); // returns read or -1
		bool readCell( const QByteArray& ); // Abgekürzte Version mit Buffer; true..ok

		struct Peek
		{
			Peek():d_type(TypeInvalid),d_off(0),d_len(0) {}
			quint32 getCellLength() const { return 1 + d_off + d_len; }
			quint32 getHeaderLength() const { return 1 + d_off; }
			bool isValid() const { return d_type != TypeInvalid; }
			DataType d_type; // TypeInvalid..pending
			quint8 d_off;  // Länge des Anzahlfelds
			quint32 d_len; // Länge der Daten
		};
		static Peek peekCell( QIODevice*); 

		static bool checkAscii( const char* );
        static QString stripMarkup( const QString&, bool interpreteMarkup = true );
	private:
		void setStr( const QString& );
		void setArr( const QByteArray& ); 
		union
		{
			quint8 d_uint8;
			quint16 d_uint16;
			quint32 d_uint32;
			quint64 d_uint64;
			qint32 d_int32;
			qint64 d_int64;
			double d_double;
            float d_float;
			quint8 d_buf[ sizeof(double) ];
			quint32 d_pair[2]; // z.B. für DateTime
		};

		quint8 d_type;
	};
}

#if defined( QT_GUI ) || defined(QT_GUI_LIB)
Q_DECLARE_METATYPE( QPicture )
#endif
Q_DECLARE_METATYPE( QUuid )

#endif
