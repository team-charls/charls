// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 


#ifndef CHARLS_DECODERSTRATEGY
#define CHARLS_DECODERSTRATEGY

#include "streams.h"

class DecoderStrategy
{
public:
	DecoderStrategy() :
	  _valcurrent(0),
		  _cbitValid(0),
		  _pbyteCompressed(0)
	  {}

	 enum { IsDecoding = 1};

	  virtual ~DecoderStrategy()
	  {}

	  virtual void SetPresets(const JlsCustomParameters& presets) = 0;
	  virtual size_t DecodeScan(void* pvoidOut, const Size& size, int cline, const void* pvoidIn, size_t cbyte, bool bCheck) = 0;

	  void Init(BYTE* pbyteCompressed, size_t cbyte)
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

	
	  void OnLineBegin(void* /*ptypeCur*/, void* /*ptypeLine*/, int /*cpixel*/) {}

	  template <class T>
	  void OnLineEnd(T* ptypeCur, T* ptypeLine, int cpixel)
	  {
#ifdef _DEBUG
			for (int i = 0; i < cpixel; ++i)
			{
				//CheckedAssign(ptypeLine[i], ptypeCur[i]);
				ptypeLine[i] = ptypeCur[i];
			}
#else
			memcpy(ptypeLine, ptypeCur, cpixel * sizeof(T));
#endif
	  }

	  typedef size_t bufType;

	  enum { 
		  bufferbits = sizeof( bufType ) * 8,
	  };

	 

	  void MakeValid()
	  {
		  int  cbitValid = _cbitValid;
		  BYTE* pbyteCompressed = _pbyteCompressed;
		  bufType valcurrent = 0;

		  while (cbitValid <= bufferbits - 8)
		  {
			  bufType valnew		  = *pbyteCompressed;
			  valcurrent		 |= valnew << (bufferbits - 8  - cbitValid);
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
		  ASSERT(length < 32);
		  UINT result = UINT(_valcurrent >> (bufferbits - length));
		  Skip(length);		
		  return result;
	  }


	  inlinehint int PeekByte()
	  { 
		  if (_cbitValid < 8)
		  {
			  MakeValid();
		  }

		  return _valcurrent >> (bufferbits - 8); 
	  }



	  inlinehint bool ReadBit()
	  {
		  if (_cbitValid == 0)
		  {
			  MakeValid();
		  }

		  bool bSet = (_valcurrent & (1LL << (bufferbits - 1))) != 0;
		  Skip(1);
		  return bSet;
	  }



	  inlinehint int Peek0Bits()
	  {
		  if (_cbitValid < 16)
		  {
			  MakeValid();
		  }
		  bufType valTest = _valcurrent;

		  for (int cbit = 0; cbit < 16; cbit++)
		  {
			  if ((valTest & (1LL << (bufferbits - 1))) != 0)
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
	bufType _valcurrent;
	BYTE* _pbyteCompressed;
	size_t _cbyteCompressed;
};


#endif
