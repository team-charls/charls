// 
// (C) Jan de Vaan 2007-2010, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 


#ifndef CHARLS_LOOKUPTABLE
#define CHARLS_LOOKUPTABLE


// Tables for fast decoding of short Golomb Codes.
struct Code
{
	Code() :
		_value(),
		_length()
	{
	}

	Code(LONG value, LONG length) :
		_value(value),
		_length(length)
	{
	}

	LONG GetValue() const
	{
		return _value;
	}

	LONG GetLength() const
	{
		return _length;
	}

	LONG _value;
	LONG _length;
};


class CTable
{
public:

	enum { cbit = 8 } ;

	CTable()
	{
		::memset(_rgtype, 0, sizeof(_rgtype));
	}

	void AddEntry(BYTE bvalue, Code c)
	{
		LONG length = c.GetLength();
		ASSERT(length <= cbit);

		for (LONG i = 0; i < LONG(1) << (cbit - length); ++i)
		{
			ASSERT(_rgtype[(bvalue << (cbit - length)) + i].GetLength() == 0);
			_rgtype[(bvalue << (cbit - length)) + i] = c;
		}
	}

	inlinehint const Code& Get(LONG value)
	{
		return _rgtype[value];
	}

private:
	Code _rgtype[1 << cbit];
};


#endif
