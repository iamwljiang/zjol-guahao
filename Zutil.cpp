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
 
#include "Zutil.h"
#include "zlib.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

//有足够内存的时候,设置zlib库使用的内存为128k
#define CHUNK 1024*128 

int CZutil::ungzip(char *in_src,int in_len,char *out_dst ,int *out_len)
{
	z_stream c_stream;
	int err = 0;

	if(in_src && in_len > 0)
	{
		c_stream.zalloc = (alloc_func)0;
		c_stream.zfree = (free_func)0;
		c_stream.opaque = (voidpf)0;
		if(deflateInit2(&c_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK) 
			return -1;
		c_stream.next_in  = (Bytef*)in_src;
		c_stream.avail_in  = in_len;
		c_stream.next_out = (Bytef*)out_dst;
		c_stream.avail_out  = *out_len;
		while (c_stream.avail_in != 0 && c_stream.total_out < *out_len) 
		{
			if(::deflate(&c_stream, Z_NO_FLUSH) != Z_OK) return -1;
		}
		
		if(c_stream.avail_in != 0) return c_stream.avail_in;
		for (;;) 
		{
			if((err = ::deflate(&c_stream, Z_FINISH)) == Z_STREAM_END) break;
			if(err != Z_OK) return -1;
		}

		if(deflateEnd(&c_stream) != Z_OK) return -1;
		
		*out_len = c_stream.total_out;
		return 0;
	}
	return -1;
}

int CZutil::gzip()
{
	return 0;
}

int CZutil::deflate()
{
	return 0;
}

int CZutil::undeflate()
{
	return 0;
}

int CZutil::zip()
{
	return 0;
}

int CZutil::unzip()
{
	return 0;
}

int   CZutil::decode_http_gzip(char *source,int len,char **dest,int *decode_len)
{ 
	*dest = NULL;
	int   		ret; 
	unsigned	have; 
	z_stream    strm; 
	unsigned char  out[CHUNK]=""; 
	int   		totalsize   =   0; 

	//初始化前清空
	strm.zalloc = Z_NULL; 
	strm.zfree  = Z_NULL; 
	strm.opaque = Z_NULL; 

	strm.avail_in=0; 
	strm.next_in= Z_NULL; 

	ret   =   inflateInit2(&strm,   47); 
	if(ret != Z_OK)
		return ret; 

	int current_pos = 0;
	int buffer_size = 0;
	strm.avail_in   = len; 
	strm.next_in    = (Bytef*)source; 

	do   { 
		strm.avail_out = CHUNK; 
		strm.next_out  = out; 
		ret = inflate(&strm,Z_NO_FLUSH); 
		assert(ret != Z_STREAM_ERROR);
		switch(ret){ 
		case Z_NEED_DICT: 
			ret = Z_DATA_ERROR;
		case Z_DATA_ERROR: 
		case Z_MEM_ERROR: 
			inflateEnd(&strm); 
			return ret; 
		}

		have = CHUNK - strm.avail_out;

		if(*dest == NULL){
			buffer_size += have+1;
			*dest = (char*)malloc(have+1);
		}else{
			buffer_size += have;
			*dest = (char*)realloc(*dest,buffer_size); 
		}

		memcpy(*dest + current_pos,out,have);
		current_pos += have;
	}while(strm.avail_out == 0); //remaining free space at next_out

	*decode_len = current_pos;
	(void)inflateEnd(&strm); 
	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR; 
}
