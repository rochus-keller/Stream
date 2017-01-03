#ifndef __Stream_BmlRecord__
#define __Stream_BmlRecord__

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
#include <QList>
#include <QMap>

namespace Stream
{
	class BmlRecord // Value
	{
	public:
		BmlRecord() {}
		BmlRecord( const QByteArray& );
		BmlRecord( const DataCell& );

		void clear();
		void readFrom( const QByteArray& bml );
		void readFrom( const DataCell& bml );
		void dump();

		QList<DataCell> d_array;
		QMap<quint32,DataCell> d_atoms;
		QMap<NameTag,DataCell> d_tags;
		QMap<QByteArray,DataCell> d_strings;
	};
}

#endif // __Stream_BmlRecord__
