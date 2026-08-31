/* Copyright (C) Teemu Suutari */

#ifndef XANCIENTOVERFLOW_P_H
#define XANCIENTOVERFLOW_P_H

#include "xancientbuffer_p.h"

namespace XAncientPrivate
{

class BoundsCheck
{
public:
	template<typename T,typename U>
	static T sum(T a,U b)
	{
		// TODO: Add type traits to handle signed integers
		T ret=a+b;
		if (ret<a) throw ByteBuffer::OutOfBoundsError();
		return ret;
	}

	template<typename T,typename U,typename ...Args>
	static T sum(T a,U b,Args... args)
	{
		return sum(sum(a,b),args...);
	}
};

}

#endif
