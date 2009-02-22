// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 


#pragma once

#include <ASSERT.h>
#include "streams.h"

class DecoderStrategy
{
public:
	DecoderStrategy() :
	  _valcurrent(0),
		  _cbitValid(0),
		  _pbyteCompressed(0)
	  {}

	  virtual ~DecoderStrategy()
	  {}

	  virtual void SetPresets(const Presets& presets) = 0;
	  virtual int DecodeScan(void* pvoidOut, const Size& size, const void* pvoidIn, int cbyte, bool bCheck) = 0;

	  void Init(BYTE* pbyteCompressed, int cbyte)
	  {
		  _cbitValid = 0;
		  _valcurrent = 0;
		  _pbyteCompressed = pbyteCompressed;
		  _cbyteCompressed = cbyte;
		  MakeValid();
	  }



	  inlinehint void Skip(int length)
	  {
		  _cbitValid -= length;
		  _valcurrent = _valcurrent << length; 
	  }



	  void MakeValid()
	  {
		  int  cbitValid = _cbitValid;
		  BYTE* pbyteCompressed = _pbyteCompressed;
		  UINT valcurrent = 0;

		  while (cbitValid <= 24)
		  {
			  UINT valnew		  = *pbyteCompressed;
			  valcurrent		 |= valnew << (24 - cbitValid);
			  pbyteCompressed  += 1;				
			  cbitValid		 += 8; 

			  if (valnew == 0xFF)		
			  {
				  cbitValid--;		
			  }
		  }

		  _valcurrent			= _valcurrent | valcurrent;
		  _cbitValid			= cbitValid;
		  _pbyteCompressed	= pbyteCompressed;
	  }



	  BYTE* GetCurBytePos() const
	  {
		  int  cbitValid = _cbitValid;
		  BYTE* pbyteCompressed = _pbyteCompressed;

		  for (;;)
		  {
			  int cbitLast = pbyteCompressed[-1] == 0xFF ? 7 : 8;

			  if (cbitValid < cbitLast )
				  return pbyteCompressed;

			  cbitValid -= cbitLast; 
			  pbyteCompressed--;
		  }	
	  }



	  inlinehint UINT ReadValue(int length)
	  {
		  if (_cbitValid < length)
		  {
			  MakeValid();
		  }

		  ASSERT(length != 0 && length <= _cbitValid);
		  UINT result = _valcurrent >> (32 - length);
		  Skip(length);		
		  return result;
	  }


	  inlinehint int PeekByte()
	  { 
		  if (_cbitValid < 8)
		  {
			  MakeValid();
		  }

		  return _valcurrent >> 24; 
	  }



	  inlinehint bool ReadBit()
	  {
		  if (_cbitValid == 0)
		  {
			  MakeValid();
		  }

		  bool bSet = (_valcurrent & 0x80000000) != 0;
		  Skip(1);
		  return bSet;
	  }



	  inlinehint int Peek0Bits()
	  {
		  if (_cbitValid < 16)
		  {
			  MakeValid();
		  }
		  UINT valTest = _valcurrent;

		  for (int cbit = 0; cbit < 16; cbit++)
		  {
			  if ((valTest & 0x80000000) != 0)
				  return cbit;

			  valTest <<= 1;
		  }
		  return -1;
	  }



	  inlinehint UINT ReadHighbits()
	  {
		  int cbit = Peek0Bits();
		  if (cbit >= 0)
		  {
			  Skip(cbit + 1);
			  return cbit;
		  }
		  Skip(15);

		  for (UINT highbits = 15; ; highbits++)
		  { 
			  if (ReadBit())
				  return highbits;
		  }                 	
	  }



	  inlinehint UINT ReadLongValue(int length)
	  {
		  if (length <= 24)
		  {
			  return ReadValue(length);
		  }

		  return (ReadValue(length - 24) << 24) + ReadValue(24);
	  }


private:
	// decoding
	int _cbitValid;
	UINT _valcurrent;
	BYTE* _pbyteCompressed;
	int _cbyteCompressed;
};


