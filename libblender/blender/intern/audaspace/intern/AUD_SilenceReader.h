/*
 * $Id: AUD_SilenceReader.h 35141 2011-02-25 10:21:56Z jesterking $
 *
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * Copyright 2009-2011 Jörg Hermann Müller
 *
 * This file is part of AudaSpace.
 *
 * Audaspace is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * AudaSpace is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Audaspace; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file audaspace/intern/AUD_SilenceReader.h
 *  \ingroup audaspaceintern
 */


#ifndef AUD_SILENCEREADER
#define AUD_SILENCEREADER

#include "AUD_IReader.h"
#include "AUD_Buffer.h"

/**
 * This class is used for sine tone playback.
 * The output format is in the 16 bit format and stereo, the sample rate can be
 * specified.
 * As the two channels both play the same the output could also be mono, but
 * in most cases this will result in having to resample for output, so stereo
 * sound is created directly.
 */
class AUD_SilenceReader : public AUD_IReader
{
private:
	/**
	 * The current position in samples.
	 */
	int m_position;

	/**
	 * The playback buffer.
	 */
	AUD_Buffer m_buffer;

	// hide copy constructor and operator=
	AUD_SilenceReader(const AUD_SilenceReader&);
	AUD_SilenceReader& operator=(const AUD_SilenceReader&);

public:
	/**
	 * Creates a new reader.
	 */
	AUD_SilenceReader();

	virtual bool isSeekable() const;
	virtual void seek(int position);
	virtual int getLength() const;
	virtual int getPosition() const;
	virtual AUD_Specs getSpecs() const;
	virtual void read(int & length, sample_t* & buffer);
};

#endif //AUD_SILENCEREADER