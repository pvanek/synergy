/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "CProtocolUtil.h"
#include "IInputStream.h"
#include "IOutputStream.h"
#include "CLog.h"
#include "stdvector.h"
#include <cctype>
#include <cstring>

//
// CProtocolUtil
//

void
CProtocolUtil::writef(IOutputStream* stream, const char* fmt, ...)
{
	assert(stream != NULL);
	assert(fmt != NULL);
	LOG((CLOG_DEBUG2 "writef(%s)", fmt));

	va_list args;

	// determine total size to write
	va_start(args, fmt);
	UInt32 count = getLength(fmt, args);
	va_end(args);

	// done if nothing to write
	if (count == 0) {
		return;
	}

	// fill buffer
	UInt8* buffer = new UInt8[count];
	va_start(args, fmt);
	writef(buffer, fmt, args);
	va_end(args);

	// write buffer
	UInt8* scan = buffer;
	while (count > 0) {
		const UInt32 n = stream->write(scan, count);
		LOG((CLOG_DEBUG2 "wrote %d of %d bytes", n, count));
		count -= n;
		scan  += n;
	}

	delete[] buffer;
}

void
CProtocolUtil::readf(IInputStream* stream, const char* fmt, ...)
{
	assert(stream != NULL);
	assert(fmt != NULL);
	LOG((CLOG_DEBUG2 "readf(%s)", fmt));

	va_list args;
	va_start(args, fmt);

	// begin scanning
	while (*fmt) {
		if (*fmt == '%') {
			// format specifier.  determine argument size.
			++fmt;
			UInt32 len = eatLength(&fmt);
			switch (*fmt) {
			case 'i': {
				// check for valid length
				assert(len == 1 || len == 2 || len == 4);

				// read the data
				UInt8 buffer[4];
				read(stream, buffer, len);

				// convert it
				void* v = va_arg(args, void*);
				switch (len) {
				case 1:
					// 1 byte integer
					*reinterpret_cast<UInt8*>(v) = buffer[0];
					LOG((CLOG_DEBUG2 "readf: read %d byte integer: %d (0x%x)", len, *reinterpret_cast<UInt8*>(v), *reinterpret_cast<UInt8*>(v)));
					break;

				case 2:
					// 2 byte integer
					*reinterpret_cast<UInt16*>(v) =
						static_cast<UInt16>(
						(static_cast<UInt16>(buffer[0]) << 8) |
						 static_cast<UInt16>(buffer[1]));
					LOG((CLOG_DEBUG2 "readf: read %d byte integer: %d (0x%x)", len, *reinterpret_cast<UInt16*>(v), *reinterpret_cast<UInt16*>(v)));
					break;

				case 4:
					// 4 byte integer
					*reinterpret_cast<UInt32*>(v) =
						(static_cast<UInt32>(buffer[0]) << 24) |
						(static_cast<UInt32>(buffer[1]) << 16) |
						(static_cast<UInt32>(buffer[2]) <<  8) |
						 static_cast<UInt32>(buffer[3]);
					LOG((CLOG_DEBUG2 "readf: read %d byte integer: %d (0x%x)", len, *reinterpret_cast<UInt32*>(v), *reinterpret_cast<UInt32*>(v)));
					break;
				}
				break;
			}

			case 'I': {
				// check for valid length
				assert(len == 1 || len == 2 || len == 4);

				// read the vector length
				UInt8 buffer[4];
				read(stream, buffer, 4);
				UInt32 n = (static_cast<UInt32>(buffer[0]) << 24) |
						   (static_cast<UInt32>(buffer[1]) << 16) |
						   (static_cast<UInt32>(buffer[2]) <<  8) |
							static_cast<UInt32>(buffer[3]);

				// convert it
				void* v = va_arg(args, void*);
				switch (len) {
				case 1:
					// 1 byte integer
					for (UInt32 i = 0; i < n; ++i) {
						read(stream, buffer, 1);
						reinterpret_cast<std::vector<UInt8>*>(v)->push_back(
							buffer[0]);
						LOG((CLOG_DEBUG2 "readf: read %d byte integer[%d]: %d (0x%x)", len, i, reinterpret_cast<std::vector<UInt8>*>(v)->back(), reinterpret_cast<std::vector<UInt8>*>(v)->back()));
					}
					break;

				case 2:
					// 2 byte integer
					for (UInt32 i = 0; i < n; ++i) {
						read(stream, buffer, 2);
						reinterpret_cast<std::vector<UInt16>*>(v)->push_back(
							static_cast<UInt16>(
							(static_cast<UInt16>(buffer[0]) << 8) |
							 static_cast<UInt16>(buffer[1])));
						LOG((CLOG_DEBUG2 "readf: read %d byte integer[%d]: %d (0x%x)", len, i, reinterpret_cast<std::vector<UInt16>*>(v)->back(), reinterpret_cast<std::vector<UInt16>*>(v)->back()));
					}
					break;

				case 4:
					// 4 byte integer
					for (UInt32 i = 0; i < n; ++i) {
						read(stream, buffer, 4);
						reinterpret_cast<std::vector<UInt32>*>(v)->push_back(
							(static_cast<UInt32>(buffer[0]) << 24) |
							(static_cast<UInt32>(buffer[1]) << 16) |
							(static_cast<UInt32>(buffer[2]) <<  8) |
							 static_cast<UInt32>(buffer[3]));
						LOG((CLOG_DEBUG2 "readf: read %d byte integer[%d]: %d (0x%x)", len, i, reinterpret_cast<std::vector<UInt32>*>(v)->back(), reinterpret_cast<std::vector<UInt32>*>(v)->back()));
					}
					break;
				}
				break;
			}

			case 's': {
				assert(len == 0);

				// read the string length
				UInt8 buffer[128];
				read(stream, buffer, 4);
				UInt32 len = (static_cast<UInt32>(buffer[0]) << 24) |
							 (static_cast<UInt32>(buffer[1]) << 16) |
							 (static_cast<UInt32>(buffer[2]) <<  8) |
							  static_cast<UInt32>(buffer[3]);

				// use a fixed size buffer if its big enough
				const bool useFixed = (len <= sizeof(buffer));

				// allocate a buffer to read the data
				UInt8* sBuffer;
				if (useFixed) {
					sBuffer = buffer;
				}
				else {
					sBuffer = new UInt8[len];
				}

				// read the data
				try {
					read(stream, sBuffer, len);
				}
				catch (...) {
					if (!useFixed) {
						delete[] sBuffer;
					}
					throw;
				}
				LOG((CLOG_DEBUG2 "readf: read %d byte string: %.*s", len, len, sBuffer));

				// save the data
				CString* dst = va_arg(args, CString*);
				dst->assign((const char*)sBuffer, len);

				// release the buffer
				if (!useFixed) {
					delete[] sBuffer;
				}
				break;
			}

			case '%':
				assert(len == 0);
				break;

			default:
				assert(0 && "invalid format specifier");
			}

			// next format character
			++fmt;
		}
		else {
			// read next character
			char buffer[1];
			read(stream, buffer, 1);

			// verify match
			if (buffer[0] != *fmt) {
				LOG((CLOG_DEBUG2 "readf: format mismatch: %c vs %c", *fmt, buffer[0]));
				throw XIOReadMismatch();
			}

			// next format character
			++fmt;
		}
	}

	va_end(args);
}

UInt32
CProtocolUtil::getLength(const char* fmt, va_list args)
{
	UInt32 n = 0;
	while (*fmt) {
		if (*fmt == '%') {
			// format specifier.  determine argument size.
			++fmt;
			UInt32 len = eatLength(&fmt);
			switch (*fmt) {
			case 'i':
				assert(len == 1 || len == 2 || len == 4);
				(void)va_arg(args, UInt32);
				break;

			case 'I':
				assert(len == 1 || len == 2 || len == 4);
				switch (len) {
				case 1:
					len = (va_arg(args, std::vector<UInt8>*))->size() + 4;
					break;

				case 2:
					len = 2 * (va_arg(args, std::vector<UInt16>*))->size() + 4;
					break;

				case 4:
					len = 4 * (va_arg(args, std::vector<UInt32>*))->size() + 4;
					break;
				}
				break;

			case 's':
				assert(len == 0);
				len = (va_arg(args, CString*))->size() + 4;
				(void)va_arg(args, UInt8*);
				break;

			case 'S':
				assert(len == 0);
				len = va_arg(args, UInt32) + 4;
				(void)va_arg(args, UInt8*);
				break;

			case '%':
				assert(len == 0);
				len = 1;
				break;

			default:
				assert(0 && "invalid format specifier");
			}

			// accumulate size
			n += len;
			++fmt;
		}
		else {
			// regular character
			++n;
			++fmt;
		}
	}
	return n;
}

void
CProtocolUtil::writef(void* buffer, const char* fmt, va_list args)
{
	UInt8* dst = reinterpret_cast<UInt8*>(buffer);

	while (*fmt) {
		if (*fmt == '%') {
			// format specifier.  determine argument size.
			++fmt;
			UInt32 len = eatLength(&fmt);
			switch (*fmt) {
			case 'i': {
				const UInt32 v = va_arg(args, UInt32);
				switch (len) {
				case 1:
					// 1 byte integer
					*dst++ = static_cast<UInt8>(v & 0xff);
					break;

				case 2:
					// 2 byte integer
					*dst++ = static_cast<UInt8>((v >> 8) & 0xff);
					*dst++ = static_cast<UInt8>( v       & 0xff);
					break;

				case 4:
					// 4 byte integer
					*dst++ = static_cast<UInt8>((v >> 24) & 0xff);
					*dst++ = static_cast<UInt8>((v >> 16) & 0xff);
					*dst++ = static_cast<UInt8>((v >>  8) & 0xff);
					*dst++ = static_cast<UInt8>( v        & 0xff);
					break;

				default:
					assert(0 && "invalid integer format length");
					return;
				}
				break;
			}

			case 'I': {
				switch (len) {
				case 1: {
					// 1 byte integers
					const std::vector<UInt8>* list =
						va_arg(args, const std::vector<UInt8>*);
					const UInt32 n = list->size();
					*dst++ = static_cast<UInt8>((n >> 24) & 0xff);
					*dst++ = static_cast<UInt8>((n >> 16) & 0xff);
					*dst++ = static_cast<UInt8>((n >>  8) & 0xff);
					*dst++ = static_cast<UInt8>( n        & 0xff);
					for (UInt32 i = 0; i < n; ++i) {
						*dst++ = (*list)[i];
					}
					break;
				}

				case 2: {
					// 2 byte integers
					const std::vector<UInt16>* list =
						va_arg(args, const std::vector<UInt16>*);
					const UInt32 n = list->size();
					*dst++ = static_cast<UInt8>((n >> 24) & 0xff);
					*dst++ = static_cast<UInt8>((n >> 16) & 0xff);
					*dst++ = static_cast<UInt8>((n >>  8) & 0xff);
					*dst++ = static_cast<UInt8>( n        & 0xff);
					for (UInt32 i = 0; i < n; ++i) {
						const UInt16 v = (*list)[i];
						*dst++ = static_cast<UInt8>((v >> 8) & 0xff);
						*dst++ = static_cast<UInt8>( v       & 0xff);
					}
					break;
				}

				case 4: {
					// 4 byte integers
					const std::vector<UInt32>* list =
						va_arg(args, const std::vector<UInt32>*);
					const UInt32 n = list->size();
					*dst++ = static_cast<UInt8>((n >> 24) & 0xff);
					*dst++ = static_cast<UInt8>((n >> 16) & 0xff);
					*dst++ = static_cast<UInt8>((n >>  8) & 0xff);
					*dst++ = static_cast<UInt8>( n        & 0xff);
					for (UInt32 i = 0; i < n; ++i) {
						const UInt32 v = (*list)[i];
						*dst++ = static_cast<UInt8>((v >> 24) & 0xff);
						*dst++ = static_cast<UInt8>((v >> 16) & 0xff);
						*dst++ = static_cast<UInt8>((v >>  8) & 0xff);
						*dst++ = static_cast<UInt8>( v        & 0xff);
					}
					break;
				}

				default:
					assert(0 && "invalid integer vector format length");
					return;
				}
				break;
			}

			case 's': {
				assert(len == 0);
				const CString* src = va_arg(args, CString*);
				const UInt32 len = (src != NULL) ? src->size() : 0;
				*dst++ = static_cast<UInt8>((len >> 24) & 0xff);
				*dst++ = static_cast<UInt8>((len >> 16) & 0xff);
				*dst++ = static_cast<UInt8>((len >>  8) & 0xff);
				*dst++ = static_cast<UInt8>( len        & 0xff);
				if (len != 0) {
					memcpy(dst, src->data(), len);
					dst += len;
				}
				break;
			}

			case 'S': {
				assert(len == 0);
				const UInt32 len = va_arg(args, UInt32);
				const UInt8* src = va_arg(args, UInt8*);
				*dst++ = static_cast<UInt8>((len >> 24) & 0xff);
				*dst++ = static_cast<UInt8>((len >> 16) & 0xff);
				*dst++ = static_cast<UInt8>((len >>  8) & 0xff);
				*dst++ = static_cast<UInt8>( len        & 0xff);
				memcpy(dst, src, len);
				dst += len;
				break;
			}

			case '%':
				assert(len == 0);
				*dst++ = '%';
				break;

			default:
				assert(0 && "invalid format specifier");
			}

			// next format character
			++fmt;
		}
		else {
			// copy regular character
			*dst++ = *fmt++;
		}
	}
}

UInt32
CProtocolUtil::eatLength(const char** pfmt)
{
	const char* fmt = *pfmt;
	UInt32 n = 0;
	for (;;) {
		UInt32 d;
		switch (*fmt) {
		case '0': d = 0; break;
		case '1': d = 1; break;
		case '2': d = 2; break;
		case '3': d = 3; break;
		case '4': d = 4; break;
		case '5': d = 5; break;
		case '6': d = 6; break;
		case '7': d = 7; break;
		case '8': d = 8; break;
		case '9': d = 9; break;
		default: *pfmt = fmt; return n;
		}
		n = 10 * n + d;
		++fmt;
	}
}

void
CProtocolUtil::read(IInputStream* stream, void* vbuffer, UInt32 count)
{
	assert(stream != NULL);
	assert(vbuffer != NULL);

	UInt8* buffer = reinterpret_cast<UInt8*>(vbuffer);
	while (count > 0) {
		// read more
		UInt32 n = stream->read(buffer, count, -1.0);

		// bail if stream has hungup
		if (n == 0) {
			LOG((CLOG_DEBUG2 "unexpected disconnect in readf(), %d bytes left", count));
			throw XIOEndOfStream();
		}

		// prepare for next read
		buffer += n;
		count  -= n;
	}
}


//
// XIOReadMismatch
//

CString
XIOReadMismatch::getWhat() const throw()
{
	return format("XIOReadMismatch", "CProtocolUtil::readf() mismatch");
}
