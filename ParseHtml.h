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
 
/**************************************
*author:chlaws
*date  :2012-10-25
*desc  :用于解析html页面提取有用内容
*
*
***************************************/

#ifndef GUAHAO_PARSEHTML_H_
#define GUAHAO_PARSEHTML_H_
#include "DataType.h"

class CParseHtml{
public:
	CParseHtml();

	~CParseHtml();

	HOSMAP ParseIndexHtml( char * data);

	DEPMAP ParseHospitalHtml( char * data);

	DETMAP ParseDeparmentHtml( char * data);

//private:
//	void   split(char * data,std::vector<std::string>& items,const std::string& sep);
public:
	//用于寻找间隔中的数据
	//n 表示要去第几个间隔数据;-表示逆向,+正向
	//支持尾部和正向索引
	//如:find_item(">徐斌</b></a> <br />副主任医师</td>",">","<",0,v);
	void   find_item(const std::string& data,const char* bstr, const char* estr, int n,std::string &result);
};


#endif //GUAHAO_PARSEHTML_H_
