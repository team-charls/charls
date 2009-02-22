// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 


#pragma once



struct Code
{
	Code()
	{
	}

	Code(int value, int length)	:
		_value(value),
		_length(length)
	{
	}

	int GetValue() const 
		{ return _value; }
	int GetLength() const 
		{ return _length; }

	int _value;
	int _length;
};



class CTable
{
public:

	enum { cbit = 8 } ;

	CTable()
	{
		::memset(rgtype, 0, sizeof(rgtype));
	}

	void AddEntry(BYTE bvalue, Code c);
	
	inlinehint const Code& Get(int value)
		{ return rgtype[value]; }
private:
	Code rgtype[1 << cbit];
};


//
// AddEntry
//
void CTable::AddEntry(BYTE bvalue, Code c)
{
	int length = c.GetLength();
	ASSERT(length <= cbit);
	
	for (int i = 0; i < 1 << (cbit - length); ++i)
	{
		ASSERT(rgtype[(bvalue << (cbit - length)) + i].GetLength() == 0);
		rgtype[(bvalue << (cbit - length)) + i] = c;					
	}
}
