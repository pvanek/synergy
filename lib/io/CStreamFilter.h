/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#ifndef CSTREAMFILTER_H
#define CSTREAMFILTER_H

#include "IStream.h"

//! A stream filter
/*!
This class wraps a stream.  Subclasses provide indirect access
to the wrapped stream, typically performing some filtering.
*/
class CStreamFilter : public IStream {
public:
	/*!
	Create a wrapper around \c stream.  Iff \c adoptStream is true then
	this object takes ownership of the stream and will delete it in the
	d'tor.
	*/
	CStreamFilter(IStream* stream, bool adoptStream = true);
	~CStreamFilter();

	// IStream overrides
	// These all just forward to the underlying stream except getEventTarget.
	// Override as necessary.  getEventTarget returns a pointer to this.
	virtual void		close();
	virtual UInt32		read(void* buffer, UInt32 n);
	virtual void		write(const void* buffer, UInt32 n);
	virtual void		flush();
	virtual void		shutdownInput();
	virtual void		shutdownOutput();
	virtual void*		getEventTarget() const;
	virtual bool		isReady() const;
	virtual UInt32		getSize() const;

protected:
	//! Get the stream
	/*!
	Returns the stream passed to the c'tor.
	*/
	IStream*			getStream() const;

	//! Handle events from source stream
	/*!
	Does the event filtering.  The default simply dispatches an event
	identical except using this object as the event target.
	*/
	virtual void		filterEvent(const CEvent&);

private:
	void				handleUpstreamEvent(const CEvent&, void*);

private:
	IStream*			m_stream;
	bool				m_adopted;
};

#endif
