// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 
#ifndef CHARLS_COLORTRANSFORM
#define CHARLS_COLORTRANSFORM

struct TransformNoneImpl
{
	inlinehint static Triplet<BYTE> Apply(int v1, int v2, int v3)
	{ return Triplet<BYTE>(v1, v2, v3); }
};

struct TransformNone : public TransformNoneImpl
{
	typedef struct TransformNoneImpl INVERSE;
};


struct TransformHp1ToRgb
{
	inlinehint static Triplet<BYTE> Apply(int v1, int v2, int v3)
	{ return Triplet<BYTE>(v1 + v2 - 0x80, v2, v3 + v2 - 0x80); }
};

struct TransformHp1
{
	typedef struct TransformHp1ToRgb INVERSE;
	inlinehint static Triplet<BYTE> Apply(int R, int G, int B)
	{
		Triplet<BYTE> hp1;
		hp1.v2 = BYTE(G);
		hp1.v1 = BYTE(R - G + 0x80);
		hp1.v3 = BYTE(B - G + 0x80);
		return hp1;
	}
};


struct TransformHp2
{
	typedef struct TransformHp2ToRgb INVERSE;
	inlinehint static Triplet<BYTE> Apply(int R, int G, int B)
	{
		return Triplet<BYTE>(R - G + 0x80, G, B - ((R+G )>>1) - 0x80);
	}
};

struct TransformHp2ToRgb
{
	inlinehint static Triplet<BYTE> Apply(int v1, int v2, int v3)
	{
		Triplet<BYTE> rgb;
		rgb.R  = BYTE(v1 + v2 - 0x80);          // new R
		rgb.G  = BYTE(v2);                     // new G				
		rgb.B  = BYTE(v3 + ((rgb.R + rgb.G) >> 1) - 0x80); // new B
		return rgb;
	}
};



struct TransformHp3
{
	typedef struct TransformHp3ToRgb INVERSE;
	inlinehint static Triplet<BYTE> Apply(int R, int G, int B)
	{
		Triplet<BYTE> hp3;		
		hp3.v2 = BYTE(B - G + 0x80);
		hp3.v3 = BYTE(R - G + 0x80);
		hp3.v1 = BYTE(G + ((hp3.v2 + hp3.v3)>>2)) - 0x40;
		return hp3;
	}
};


struct TransformHp3ToRgb
{
	inlinehint static Triplet<BYTE> Apply(int v1, int v2, int v3)
	{
		int G = v1 - ((v3 + v2)>>2) + 0x40;
		Triplet<BYTE> rgb;
		rgb.R  = BYTE(v3 + G - 0x80); // new R
		rgb.G  = BYTE(G);             // new G				
		rgb.B  = BYTE(v2 + G - 0x80); // new B
		return rgb;
	}
};



#endif
