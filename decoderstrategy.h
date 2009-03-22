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
	  virtual ULONG DecodeScan(void* pvoidOut, const Size& size, LONG cline, const void* pvoidIn, ULONG cbyte, bool bCheck) = 0;

	  void Init(BYTE* pbyteCompressed, ULONG cbyte)
	  {
		  _cbitValid = 0;
		  _valcurrent = 0;
		  _pbyteCompressed = pbyteCompressed;
		  _cbyteCompressed = cbyte;
		  MakeValid();
	  }

		

	  inlinehint void Skip(LONG length)
	  {
		  _cbitValid -= length;
		  _valcurrent = _valcurrent << length; 
	  }

	
	  void OnLineBegin(void* /*ptypeCur*/, void* /*ptypeLine*/, LONG /*cpixel*/) {}

	  template <class T>
	  void OnLineEnd(T* ptypeCur, T* ptypeLine, LONG cpixel)
	  {
#ifdef _DEBUG
			for (LONG i = 0; i < cpixel; ++i)
			{
				//CheckedAssign(ptypeLine[i], ptypeCur[i]);
				ptypeLine[i] = ptypeCur[i];
			}
#else
			memcpy(ptypeLine, ptypeCur, cpixel * sizeof(T));
#endif
	  }

	  typedef ULONG bufType;

	  enum { 
		  bufferbits = sizeof( bufType ) * 8,
	  };

	 

	  void MakeValid()
	  {
		  LONG  cbitValid = _cbitValid;
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
		  LONG  cbitValid = _cbitValid;
		  BYTE* pbyteCompressed = _pbyteCompressed;

		  for (;;)
		  {
			  LONG cbitLast = pbyteCompressed[-1] == 0xFF ? 7 : 8;

			  if (cbitValid < cbitLast )
				  return pbyteCompressed;

			  cbitValid -= cbitLast; 
			  pbyteCompressed--;
		  }	
	  }



	  inlinehint ULONG ReadValue(LONG length)
	  {
		  if (_cbitValid < length)
		  {
			  MakeValid();
		  }

		  ASSERT(length != 0 && length <= _cbitValid);
		  ASSERT(length < 32);
		  ULONG result = ULONG(_valcurrent >> (bufferbits - length));
		  Skip(length);		
		  return result;
	  }


	  inlinehint LONG PeekByte()
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



	  inlinehint LONG Peek0Bits()
	  {
		  if (_cbitValid < 16)
		  {
			  MakeValid();
		  }
		  bufType valTest = _valcurrent;

		  for (LONG cbit = 0; cbit < 16; cbit++)
		  {
			  if ((valTest & (1LL << (bufferbits - 1))) != 0)
				  return cbit;

			  valTest <<= 1;
		  }
		  return -1;
	  }



	  inlinehint ULONG ReadHighbits()
	  {
		  LONG cbit = Peek0Bits();
		  if (cbit >= 0)
		  {
			  Skip(cbit + 1);
			  return cbit;
		  }
		  Skip(15);

		  for (ULONG highbits = 15; ; highbits++)
		  { 
			  if (ReadBit())
				  return highbits;
		  }                 	
	  }



	  inlinehint ULONG ReadLongValue(LONG length)
	  {
		  if (length <= 24)
		  {
			  return ReadValue(length);
		  }

		  return (ReadValue(length - 24) << 24) + ReadValue(24);
	  }


private:
	// decoding
	bufType _valcurrent;
	LONG _cbitValid;
	BYTE* _pbyteCompressed;
	ULONG _cbyteCompressed;
};


#endif
