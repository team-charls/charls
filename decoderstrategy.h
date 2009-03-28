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
	  _readCache(0),
		  _validBits(0),
		  _pbyteCompressed(0)
	  {}

	 enum { IsDecoding = 1};

	  virtual ~DecoderStrategy()
	  {}

	  virtual void SetPresets(const JlsCustomParameters& presets) = 0;
	  virtual size_t DecodeScan(void* pvoidOut, const Size& size, LONG cline, const void* pvoidIn, size_t cbyte, bool bCheck) = 0;

	  void Init(BYTE* pbyteCompressed, size_t cbyte)
	  {
		  _validBits = 0;
		  _readCache = 0;
		  _pbyteCompressed = pbyteCompressed;
		  _pbyteCompressedEnd = pbyteCompressed + cbyte;
		  _pbyteNextFF = FindNextFF();
		  MakeValid();
	  }

		

	  inlinehint void Skip(LONG length)
	  {
		  _validBits -= length;
		  _readCache = _readCache << length; 
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
		  ASSERT(_validBits <=bufferbits - 8);

		  if (_pbyteCompressed < _pbyteNextFF)
		  {
			  while (_validBits <= bufferbits - 8)
			  {
				  _readCache		 |= _pbyteCompressed[0] << (bufferbits - 8  - _validBits);
				  _validBits		 += 8; 				  
				  _pbyteCompressed  += 1;				
			  }

			  ASSERT(_validBits >= bufferbits - 8);
			  return;
		  }

		  while (_validBits < bufferbits - 8)
		  {
			  if (_pbyteCompressed >= _pbyteCompressedEnd)
			  {
				  if (_validBits <= 0)
					  throw JlsException(InvalidCompressedData);

				  return;
			  }

			  bufType valnew	  = *_pbyteCompressed;
			  _readCache		 |= valnew << (bufferbits - 8  - _validBits);
			  _pbyteCompressed  += 1;				
			  _validBits		 += 8; 

			  if (valnew == 0xFF)		
			  {
				  _validBits--;		
			  }
		  }

		  _pbyteNextFF = FindNextFF();

	  }


	  BYTE* FindNextFF()
	  {
		  BYTE* pbyteNextFF =_pbyteCompressed;

		  while (pbyteNextFF < _pbyteCompressedEnd && *pbyteNextFF != 0xFF)
	      {
			   pbyteNextFF++;
		  }
		  
		  return pbyteNextFF - 3;
	  }

	  BYTE* GetCurBytePos() const
	  {
		  LONG  cbitValid = _validBits;
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


	  inlinehint LONG ReadValue(LONG length)
	  {
		  if (_validBits < length)
		  {
			  MakeValid();
		  }

		  ASSERT(length != 0 && length <= _validBits);
		  ASSERT(length < 32);
		  LONG result = LONG(_readCache >> (bufferbits - length));
		  Skip(length);		
		  return result;
	  }


	  inlinehint LONG PeekByte()
	  { 
		  if (_validBits < 8)
		  {
			  MakeValid();
		  }

		  return _readCache >> (bufferbits - 8); 
	  }



	  inlinehint bool ReadBit()
	  {
		  if (_validBits == 0)
		  {
			  MakeValid();
		  }

		  bool bSet = (_readCache & (bufType(1) << (bufferbits - 1))) != 0;
		  Skip(1);
		  return bSet;
	  }



	  inlinehint LONG Peek0Bits()
	  {
		  if (_validBits < 16)
		  {
			  MakeValid();
		  }
		  bufType valTest = _readCache;

		  for (LONG cbit = 0; cbit < 16; cbit++)
		  {
			  if ((valTest & (bufType(1) << (bufferbits - 1))) != 0)
				  return cbit;

			  valTest <<= 1;
		  }
		  return -1;
	  }



	  inlinehint LONG ReadHighbits()
	  {
		  LONG cbit = Peek0Bits();
		  if (cbit >= 0)
		  {
			  Skip(cbit + 1);
			  return cbit;
		  }
		  Skip(15);

		  for (LONG highbits = 15; ; highbits++)
		  { 
			  if (ReadBit())
				  return highbits;
		  }                 	
	  }


	  LONG ReadLongValue(LONG length)
	  {
		  if (length <= 24)
			  return ReadValue(length);

		  return (ReadValue(length - 24) << 24) + ReadValue(24);
	  }


private:
	// decoding
	bufType _readCache;
	LONG _validBits;
	BYTE* _pbyteCompressed;
	BYTE* _pbyteNextFF;
	BYTE* _pbyteCompressedEnd;
};


#endif
