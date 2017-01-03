#ifndef __Stream_Exceptions__
#define __Stream_Exceptions__

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

#include <exception>
#include <QString>

namespace Stream
{
	class StreamException : public std::exception
	{
	public:
		enum Code {
			IncompleteImplementation,
			WrongDataFormat,
			InvalidProtocol,
			InvalidDevice,
			DeviceAccess
		};
		StreamException( Code c, const QString& msg = "" ):d_err(c),d_msg(msg){}
		~StreamException() throw() {}
		Code getCode() const { return d_err; }
		const QString& getMsg() const { return d_msg; }
	private:
		Code d_err;
		QString d_msg;
	};

}

#endif
