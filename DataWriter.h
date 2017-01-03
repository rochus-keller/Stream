#ifndef __stream_datawriter__
#define __stream_datawriter__

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
#include <QMap>

namespace Stream
{
	// Value Class
	// Generiert einen BML-Stream. Wenn man gültige BML-Streams aneinanderhängt, ist das Ergebnis wieder 
	// ein gültiger BML-Stream.
	class DataWriter
	{
	public:
		DataWriter( QIODevice*, bool owner = false );
		DataWriter(); // Erzeuge einen Writer mit QBuffer
		DataWriter( const DataWriter& rhs ); // dummy für Default-Constructor
		~DataWriter();

		DataWriter& operator=( const DataWriter& ) { return *this; } // Dummy

		void setDevice( QIODevice* = 0, bool owner = false ); // 0..QBuffer

		void startFrame( DataCell::Atom name = DataCell::null );
		void startFrame( NameTag name );
		void startFrame( const char* ascii ); 
		void endFrame();
		void writeSlot( const DataCell&, DataCell::Atom name = DataCell::null, bool compress = false );
		void writeSlot( const DataCell&, NameTag name, bool compress = false );
		void writeSlot( const DataCell&, const char* ascii, bool compress = false ); 

		quint16 getLevel() const { return d_level; }
		quint16 getCells() const { return d_cells; }
		quint16 getNulls() const { return d_nulls; }
		bool isNull() const { return d_cells == d_nulls; }
		QByteArray getStream() const; // Im Falle von QBuffer gebe buffer() zurück.
		DataCell getBml() const { return DataCell().setBml( getStream() ); }
	private:
		void open();
		void begin();
		QIODevice* d_out;
		QMap<QByteArray,quint32> d_names;
		quint16 d_level;
		// RISK: genügen #16bit Cells?
		quint16 d_cells; // Anzahl Top-Level-Cells
		quint16 d_nulls; // Anzahl Top-Level-Nulls
		bool d_owner;

		// DONT_CREATE_ON_HEAP;
	};
}

#endif
