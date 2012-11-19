/*
 * Copyright (c) 2012, chlaws <iamwljiang at gmail.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
#include "Urlcode.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
int url_encode(char** pout, const char* pin ,  int len)
{
	//将保留字符转化为16进制格式.
	// according to RFC 2396
	char sca[4] = "%";
	unsigned char c;
	char* buff = 0 , *pstr = 0;
	int i = 0 , bufflen = 0;

	if( !pin )
		return 0;

	if( len <= 0 ){
		//
		len = strlen( pin );
		if( len <= 0 )
			return 0;
	}
	bufflen = (len * 3) + 2;

	buff = (char*)malloc( bufflen );
	memset(buff , 0 , bufflen );
	pstr = buff;

	while( *pin != '\0' && i < len )
	{
		c = *pin;
		if( isalpha( c ) || isdigit( c ) ||
			c == '-' ||	c == '_' || 	c == '.' ||	c == '!' ||	c == '~' ||	c == '*' ||	c == '\'' || c == '(' || c == ')' )
		{
			// found an unreserved character
			strncat(pstr, pin , 1);
			pstr++;
		}
		else if( c == ' ' )
		{
			strcat(pstr , "%20");
			pstr += 3;
		}
		else
		{
			sprintf( &(sca[1]) , "%02X", c );
			sca[3] = '\0';
			strcat(pstr , sca );
			pstr += 3;
		}
		pin++;
		i++;
	}
	*pstr ='\0';
	*pout = buff;
	return (pstr-buff);
}

int url_decode(char** pout,const char* pin , int len)
{
	char sca[4];
	char* buff = 0 , *pstr = 0;
	const char* pin_end = 0;
	int  c = 0;

	if( !pin )
		return 0;

	if( len <= 0 ){
		len = strlen( pin );
		if( len <= 0 )
			return 0;
	}
	pin_end = pin + len;

	buff = (char*)malloc( len+1 );
	memset(buff , 0 , len+1 );
	pstr= buff;

	while( pin != '\0' && (pin < pin_end) )
	{
		if( *(pin) == '%' )
		{
			if( (pin + 2) < pin_end )
			{				
				sca[0] = *(pin+1);
				sca[1] = *(pin+2);
				sca[2] = '\0';
				sscanf( sca , "%02X", &c );
				*pstr += (char)c;

				pin ++;
				pin ++;
				pin ++;
				pstr++;
			}
			else
			{
				free(buff);			
				return -1;
			}
		}
		else
		{
			*pstr += *pin;
			pin++;
			pstr++;
		}
	}
	*pstr = '\0';
	*pout = buff;
	return (pstr-buff);
}
