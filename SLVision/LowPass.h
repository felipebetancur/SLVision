/*
 * Copyright (C) 2011-2013  Music Technology Group - Universitat Pompeu Fabra
 *
 * This file is part of SLVision
 *
 * SLVision is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License as published by the Free
 * Software Foundation (FSF), either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the Affero GNU General Public License
 * version 3 along with this program.  If not, see http://www.gnu.org/licenses/
 */

/*
 *	Daniel Gallardo Grassot
 *	daniel.gallardo@upf.edu
 *	Barcelona 2011
 */

#pragma once
#include <list>

class LowPass
{
private:
	unsigned  queuesize;
	std::list<float> data;
	float value;
public:

	///folat addvalue(float value);
	//adds a velue to the queue and return the med value

	///float getvalue ()
	//return the med value

	///LowPass(int queue_size)
	LowPass(unsigned  queue_size);
	LowPass(const LowPass &copy);
	float addvalue(float value);
	float getvalue ();
	void Reset();
	~LowPass(void);
};

