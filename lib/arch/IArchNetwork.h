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

#ifndef IARCHNETWORK_H
#define IARCHNETWORK_H

#include "IInterface.h"
#include "stdstring.h"

class CArchThreadImpl;
typedef CArchThreadImpl* CArchThread;

/*!      
\class CArchSocketImpl
\brief Internal socket data.
An architecture dependent type holding the necessary data for a socket.
*/
class CArchSocketImpl;

/*!      
\var CArchSocket
\brief Opaque socket type.
An opaque type representing a socket.
*/
typedef CArchSocketImpl* CArchSocket;

/*!      
\class CArchNetAddressImpl
\brief Internal network address data.
An architecture dependent type holding the necessary data for a network
address.
*/
class CArchNetAddressImpl;

/*!      
\var CArchNetAddress
\brief Opaque network address type.
An opaque type representing a network address.
*/
typedef CArchNetAddressImpl* CArchNetAddress;

//! Interface for architecture dependent networking
/*!
This interface defines the networking operations required by
synergy.  Each architecture must implement this interface.
*/
class IArchNetwork : public IInterface {
public:
	//! Supported address families
	enum EAddressFamily {
		kUNKNOWN,
		kINET,
	};

	//! Supported socket types
	enum ESocketType {
		kDGRAM,
		kSTREAM
	};

	//! Events for \c poll()
	/*!
	Events for \c poll() are bitmasks and can be combined using the
	bitwise operators.
	*/
	enum {
		kPOLLIN   = 1,		//!< Socket is readable
		kPOLLOUT  = 2,		//!< Socket is writable
		kPOLLERR  = 4,		//!< The socket is in an error state
		kPOLLNVAL = 8		//!< The socket is invalid
	};

	//! A socket query for \c poll()
	class CPollEntry {
	public:
		//! The socket to query
		CArchSocket		m_socket;

		//! The events to query for
		/*!
		The events to query for can be any combination of kPOLLIN and
		kPOLLOUT.
		*/
		unsigned short	m_events;

		//! The result events
		unsigned short	m_revents;
	};

	//! @name manipulators
	//@{

	//! Create a new socket
	/*!
	The socket is an opaque data type.
	*/
	virtual CArchSocket	newSocket(EAddressFamily, ESocketType) = 0;

	//! Copy a socket object
	/*!
	Returns a reference to to socket referred to by \c s.
	*/
	virtual CArchSocket	copySocket(CArchSocket s) = 0;

	//! Release a socket reference
	/*!
	Deletes the given socket object.  This does not destroy the socket
	the object referred to until there are no remaining references for
	the socket.
	*/
	virtual void		closeSocket(CArchSocket s) = 0;

	//! Close socket for further reads
	/*!
	Calling this disallows future reads on socket \c s.
	*/
	virtual void		closeSocketForRead(CArchSocket s) = 0;

	//! Close socket for further writes
	/*!
	Calling this disallows future writes on socket \c s.
	*/
	virtual void		closeSocketForWrite(CArchSocket s) = 0;

	//! Bind socket to address
	/*!
	Binds socket \c s to the address \c addr.
	*/
	virtual void		bindSocket(CArchSocket s, CArchNetAddress addr) = 0;

	//! Listen for connections on socket
	/*!
	Causes the socket \c s to begin listening for incoming connections.
	*/
	virtual void		listenOnSocket(CArchSocket s) = 0;

	//! Accept connection on socket
	/*!
	Accepts a connection on socket \c s, returning a new socket for the
	connection and filling in \c addr with the address of the remote
	end.  \c addr may be NULL if the remote address isn't required.
	The original socket \c s is unaffected and remains in the listening
	state.  The new socket shares most of the properties of \c s except
	it's not in the listening state and it's connected.  Returns NULL
	if there are no pending connection requests.
	*/
	virtual CArchSocket	acceptSocket(CArchSocket s, CArchNetAddress* addr) = 0;

	//! Connect socket
	/*!
	Connects the socket \c s to the remote address \c addr.  Returns
	true if the connection succeed immediately, false if the connection
	is in progress, and throws if the connection failed	immediately.
	If it returns false, \c pollSocket() can be used to wait on the
	socket for writing to detect when the connection finally succeeds
	or fails.
	*/
	virtual bool		connectSocket(CArchSocket s, CArchNetAddress addr) = 0;

	//! Check socket state
	/*!
	Tests the state of \c num sockets for readability and/or writability.
	Waits up to \c timeout seconds for some socket to become readable
	and/or writable (or indefinitely if \c timeout < 0).  Returns the
	number of sockets that were readable (if readability was being
	queried) or writable (if writablility was being queried) and sets
	the \c m_revents members of the entries.  \c kPOLLERR and \c kPOLLNVAL
	are set in \c m_revents as appropriate.  If a socket indicates
	\c kPOLLERR then \c throwErrorOnSocket() can be used to determine
	the type of error.  Returns 0 immediately regardless of the \c timeout
	if no valid sockets are selected for testing.

	(Cancellation point)
	*/
	virtual int			pollSocket(CPollEntry[], int num, double timeout) = 0;

	//! Unblock thread in pollSocket()
	/*!
	Cause a thread that's in a pollSocket() call to return.  This
	call may return before the thread is unblocked.  If the thread is
	not in a pollSocket() call this call has no effect.
	*/
	virtual void		unblockPollSocket(CArchThread thread) = 0;

	//! Read data from socket
	/*!
	Read up to \c len bytes from socket \c s in \c buf and return the
	number of bytes read.  The number of bytes can be less than \c len
	if not enough data is available.  Returns 0 if the remote end has
	disconnected and/or there is no more queued received data.
	*/
	virtual size_t		readSocket(CArchSocket s, void* buf, size_t len) = 0;

	//! Write data from socket
	/*!
	Write up to \c len bytes to socket \c s from \c buf and return the
	number of bytes written.  The number of bytes can be less than
	\c len if the remote end disconnected or the internal buffers fill
	up.
	*/
	virtual size_t		writeSocket(CArchSocket s,
							const void* buf, size_t len) = 0;

	//! Check error on socket
	/*!
	If the socket \c s is in an error state then throws an appropriate
	XArchNetwork exception.
	*/
	virtual void		throwErrorOnSocket(CArchSocket s) = 0;

	//! Turn Nagle algorithm on or off on socket
	/*!
	Set socket to send messages immediately (true) or to collect small
	messages into one packet (false).  Returns the previous state.
	*/
	virtual bool		setNoDelayOnSocket(CArchSocket, bool noDelay) = 0;

	//! Return local host's name
	virtual std::string		getHostName() = 0;

	//! Create an "any" network address
	virtual CArchNetAddress	newAnyAddr(EAddressFamily) = 0;

	//! Copy a network address
	virtual CArchNetAddress	copyAddr(CArchNetAddress) = 0;

	//! Convert a name to a network address
	virtual CArchNetAddress	nameToAddr(const std::string&) = 0;

	//! Destroy a network address
	virtual void			closeAddr(CArchNetAddress) = 0;

	//! Convert an address to a host name
	virtual std::string		addrToName(CArchNetAddress) = 0;

	//! Convert an address to a string
	virtual std::string		addrToString(CArchNetAddress) = 0;

	//! Get an address's family
	virtual EAddressFamily	getAddrFamily(CArchNetAddress) = 0;

	//! Set the port of an address
	virtual void			setAddrPort(CArchNetAddress, int port) = 0;

	//! Get the port of an address
	virtual int				getAddrPort(CArchNetAddress) = 0;

	//! Test addresses for equality
	virtual bool			isEqualAddr(CArchNetAddress, CArchNetAddress) = 0;

	//! Test for the "any" address
	/*!
	Returns true if \c addr is the "any" address.  \c newAnyAddr()
	returns an "any" address.
	*/
	virtual bool			isAnyAddr(CArchNetAddress addr) = 0;

	//@}
};

#endif
