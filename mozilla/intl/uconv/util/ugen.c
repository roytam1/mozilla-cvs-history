/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
#include "unicpriv.h"
/*=================================================================================

=================================================================================*/
typedef  PRBool (*uSubGeneratorFunc) (PRUint16 in, unsigned char* out);
/*=================================================================================

=================================================================================*/

typedef PRBool (*uGeneratorFunc) (
		uShiftTable 			*shift,
		PRInt32*				state,
		PRUint16				in,
		unsigned char*		out,
		PRUint32 				outbuflen,
		PRUint32*				outlen
);

MODULE_PRIVATE PRBool uGenerate(		
		uShiftTable 			*shift,
		PRInt32*				state,
		PRUint16				in,
		unsigned char*		out,
		PRUint32 				outbuflen,
		PRUint32*				outlen
);

#define uSubGennerator(sub,in,out)	(* m_subgenerator[sub])((in),(out))

PRIVATE PRBool uCheckAndGenAlways1Byte(
		uShiftTable 			*shift,
		PRInt32*				state,
		PRUint16				in,
		unsigned char*		out,
		PRUint32 				outbuflen,
		PRUint32*				outlen
);
PRIVATE PRBool uCheckAndGenAlways2Byte(
		uShiftTable 			*shift,
		PRInt32*				state,
		PRUint16				in,
		unsigned char*		out,
		PRUint32 				outbuflen,
		PRUint32*				outlen
);
PRIVATE PRBool uCheckAndGenAlways2ByteShiftGR(
		uShiftTable 			*shift,
		PRInt32*				state,
		PRUint16				in,
		unsigned char*		out,
		PRUint32 				outbuflen,
		PRUint32*				outlen
);
PRIVATE PRBool uCheckAndGenByTable(
		uShiftTable 			*shift,
		PRInt32*				state,
		PRUint16				in,
		unsigned char*		out,
		PRUint32 				outbuflen,
		PRUint32*				outlen
);
PRIVATE PRBool uCheckAndGen2ByteGRPrefix8F(
		uShiftTable 			*shift,
		PRInt32*				state,
		PRUint16				in,
		unsigned char*		out,
		PRUint32 				outbuflen,
		PRUint32*				outlen
);
PRIVATE PRBool uCheckAndGen2ByteGRPrefix8EA2(
		uShiftTable 			*shift,
		PRInt32*				state,
		PRUint16				in,
		unsigned char*		out,
		PRUint32 				outbuflen,
		PRUint32*				outlen
);

PRIVATE PRBool uCheckAndGenAlways2ByteSwap(
		uShiftTable 			*shift,
		PRInt32*				state,
		PRUint16				in,
		unsigned char*		out,
		PRUint32 				outbuflen,
		PRUint32*				outlen
);

PRIVATE PRBool uCheckAndGenAlways4Byte(
		uShiftTable 			*shift,
		PRInt32*				state,
		PRUint16				in,
		unsigned char*		out,
		PRUint32 				outbuflen,
		PRUint32*				outlen
);

PRIVATE PRBool uCheckAndGenAlways4ByteSwap(
		uShiftTable 			*shift,
		PRInt32*				state,
		PRUint16				in,
		unsigned char*		out,
		PRUint32 				outbuflen,
		PRUint32*				outlen
);
PRIVATE PRBool uGenAlways2Byte(
		PRUint16 				in,
		unsigned char*		out
);
PRIVATE PRBool uGenAlways2ByteShiftGR(
		PRUint16 				in,
		unsigned char*		out
);
PRIVATE PRBool uGenAlways1Byte(
		PRUint16 				in,
		unsigned char*		out
);
PRIVATE PRBool uGenAlways1BytePrefix8E(
		PRUint16 				in,
		unsigned char*		out
);
PRIVATE PRBool uGenAlways2ByteUTF8(
		PRUint16 				in,
		unsigned char*		out
);
PRIVATE PRBool uGenAlways3ByteUTF8(
		PRUint16 				in,
		unsigned char*		out
);
/*=================================================================================

=================================================================================*/
PRIVATE uGeneratorFunc m_generator[uNumOfCharsetType] =
{
	uCheckAndGenAlways1Byte,
	uCheckAndGenAlways2Byte,
	uCheckAndGenByTable,
	uCheckAndGenAlways2ByteShiftGR,
	uCheckAndGen2ByteGRPrefix8F,
	uCheckAndGen2ByteGRPrefix8EA2,
	uCheckAndGenAlways2ByteSwap,
	uCheckAndGenAlways4Byte,
	uCheckAndGenAlways4ByteSwap,
};

/*=================================================================================

=================================================================================*/

PRIVATE uSubGeneratorFunc m_subgenerator[uNumOfCharType] =
{
	uGenAlways1Byte,
	uGenAlways2Byte,
	uGenAlways2ByteShiftGR,
	uGenAlways1BytePrefix8E,
	uGenAlways2ByteUTF8,
	uGenAlways3ByteUTF8

};
/*=================================================================================

=================================================================================*/
MODULE_PRIVATE PRBool uGenerate(		
		uShiftTable 			*shift,
		PRInt32*				state,
		PRUint16				in,
		unsigned char*		out,
		PRUint32 				outbuflen,
		PRUint32*				outlen
)
{
	return (* m_generator[shift->classID]) (shift,state,in,out,outbuflen,outlen);
}
/*=================================================================================

=================================================================================*/
PRIVATE PRBool uGenAlways1Byte(
		PRUint16 				in,
		unsigned char*		out
)
{
	out[0] = (unsigned char)in;
	return PR_TRUE;
}

/*=================================================================================

=================================================================================*/
PRIVATE PRBool uGenAlways2Byte(
		PRUint16 				in,
		unsigned char*		out
)
{
	out[0] = (unsigned char)((in >> 8) & 0xff);
	out[1] = (unsigned char)(in & 0xff);
	return PR_TRUE;
}
/*=================================================================================

=================================================================================*/
PRIVATE PRBool uGenAlways2ByteShiftGR(
		PRUint16 				in,
		unsigned char*		out
)
{
	out[0] = (unsigned char)(((in >> 8) & 0xff) | 0x80);
	out[1] = (unsigned char)((in & 0xff) | 0x80);
	return PR_TRUE;
}
/*=================================================================================

=================================================================================*/
PRIVATE PRBool uGenAlways1BytePrefix8E(
		PRUint16 				in,
		unsigned char*		out
)
{
	out[0] = 0x8E;
	out[1] = (unsigned char)(in  & 0xff);
	return PR_TRUE;
}
/*=================================================================================

=================================================================================*/
PRIVATE PRBool uGenAlways2ByteUTF8(
		PRUint16 				in,
		unsigned char*		out
)
{
	out[0] = (unsigned char)(0xC0 | (( in >> 6 ) & 0x1F));
	out[1] = (unsigned char)(0x80 | (( in      ) & 0x3F));
	return PR_TRUE;
}

/*=================================================================================

=================================================================================*/
PRIVATE PRBool uGenAlways3ByteUTF8(
		PRUint16 				in,
		unsigned char*		out
)
{
	out[0] = (unsigned char)(0xE0 | (( in >> 12 ) & 0x0F));
	out[1] = (unsigned char)(0x80 | (( in >> 6  ) & 0x3F));
	out[2] = (unsigned char)(0x80 | (( in       ) & 0x3F));
	return PR_TRUE;
}
/*=================================================================================

=================================================================================*/
PRIVATE PRBool uCheckAndGenAlways1Byte(
		uShiftTable 			*shift,
		PRInt32*				state,
		PRUint16				in,
		unsigned char*		out,
		PRUint32 				outbuflen,
		PRUint32*				outlen
)
{
	/*	Don't check inlen. The caller should ensure it is larger than 0 */
    /*  Oops, I don't agree. Code changed to check every time. [CATA] */
	if(outbuflen < 1)
		return PR_FALSE;
	else
	{
        *outlen = 1;
	    out[0] = in & 0xff;
	    return PR_TRUE;
    }
}

/*=================================================================================

=================================================================================*/
PRIVATE PRBool uCheckAndGenAlways2Byte(
		uShiftTable 			*shift,
		PRInt32*				state,
		PRUint16				in,
		unsigned char*		out,
		PRUint32 				outbuflen,
		PRUint32*				outlen
)
{
	if(outbuflen < 2)
		return PR_FALSE;
	else
	{
		*outlen = 2;
		out[0] = ((in >> 8 ) & 0xff);
		out[1] = in  & 0xff;
		return PR_TRUE;
	}
}
/*=================================================================================

=================================================================================*/
PRIVATE PRBool uCheckAndGenAlways2ByteShiftGR(
		uShiftTable 			*shift,
		PRInt32*				state,
		PRUint16				in,
		unsigned char*		out,
		PRUint32 				outbuflen,
		PRUint32*				outlen
)
{
	if(outbuflen < 2)
		return PR_FALSE;
	else
	{
		*outlen = 2;
		out[0] = ((in >> 8 ) & 0xff) | 0x80;
		out[1] = (in  & 0xff)  | 0x80;
		return PR_TRUE;
	}
}
/*=================================================================================

=================================================================================*/
PRIVATE PRBool uCheckAndGenByTable(
		uShiftTable 			*shift,
		PRInt32*				state,
		PRUint16				in,
		unsigned char*		out,
		PRUint32 				outbuflen,
		PRUint32*				outlen
)
{
	PRInt16 i;
	uShiftCell* cell = &(shift->shiftcell[0]);
	PRInt16 itemnum = shift->numOfItem;
	unsigned char inH, inL;
	inH =	(in >> 8) & 0xff;
	inL = (in & 0xff );
	for(i=0;i<itemnum;i++)
	{
		if( ( inL >=  cell[i].shiftout.MinLB) &&
			( inL <=  cell[i].shiftout.MaxLB) &&
			( inH >=  cell[i].shiftout.MinHB) &&
			( inH <=  cell[i].shiftout.MaxHB)	)
		{
			if(outbuflen < cell[i].reserveLen)
				return PR_FALSE;
			else
			{
				*outlen = cell[i].reserveLen;
				return (uSubGennerator(cell[i].classID,in,out));
			}
		}
	}
	return PR_FALSE;
}
/*=================================================================================

=================================================================================*/
PRIVATE PRBool uCheckAndGen2ByteGRPrefix8F( uShiftTable 			*shift,
		PRInt32*				state,
		PRUint16				in,
		unsigned char*		out,
		PRUint32 				outbuflen,
		PRUint32*				outlen
)
{
	if(outbuflen < 3)
		return PR_FALSE;
	else
	{
		*outlen = 3;
		out[0] = 0x8F;
		out[1] = ((in >> 8 ) & 0xff) | 0x80;
		out[2] = (in  & 0xff)  | 0x80;
		return PR_TRUE;
	}
}
/*=================================================================================

=================================================================================*/
PRIVATE PRBool uCheckAndGen2ByteGRPrefix8EA2( uShiftTable 			*shift,
		PRInt32*				state,
		PRUint16				in,
		unsigned char*		out,
		PRUint32 				outbuflen,
		PRUint32*				outlen
)
{
	if(outbuflen < 4)
		return PR_FALSE;
	else
	{
		*outlen = 4;
		out[0] = 0x8E;
		out[1] = 0xA2;
		out[2] = ((in >> 8 ) & 0xff) | 0x80;
		out[3] = (in  & 0xff)  | 0x80;
		return PR_TRUE;
	}
}


/*=================================================================================

=================================================================================*/
PRIVATE PRBool uCheckAndGenAlways2ByteSwap(
		uShiftTable 			*shift,
		PRInt32*				state,
		PRUint16				in,
		unsigned char*		out,
		PRUint32 				outbuflen,
		PRUint32*				outlen
)
{
	if(outbuflen < 2)
		return PR_FALSE;
	else
	{
		*outlen = 2;
		out[0] = in  & 0xff;
		out[1] = ((in >> 8 ) & 0xff);
		return PR_TRUE;
	}
}
/*=================================================================================

=================================================================================*/
PRIVATE PRBool uCheckAndGenAlways4Byte(
		uShiftTable 			*shift,
		PRInt32*				state,
		PRUint16				in,
		unsigned char*		out,
		PRUint32 				outbuflen,
		PRUint32*				outlen
)
{
	if(outbuflen < 4)
		return PR_FALSE;
	else
	{
		*outlen = 4;
                out[0] = out[1] = 0x00;
		out[2] = ((in >> 8 ) & 0xff);
		out[3] = in  & 0xff;
		return PR_TRUE;
	}
}
/*=================================================================================

=================================================================================*/
PRIVATE PRBool uCheckAndGenAlways4ByteSwap(
		uShiftTable 			*shift,
		PRInt32*				state,
		PRUint16				in,
		unsigned char*		out,
		PRUint32 				outbuflen,
		PRUint32*				outlen
)
{
	if(outbuflen < 4)
		return PR_FALSE;
	else
	{
		*outlen = 4;
		out[0] = ((in >> 8 ) & 0xff);
		out[1] = in  & 0xff;
                out[2] = out[3] = 0x00;
		return PR_TRUE;
	}
}
