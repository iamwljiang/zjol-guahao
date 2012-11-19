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
*desc  :用于组织合法的http请求
*
*
***************************************/
#ifndef GUAHAO_REQUESTHEADER_H_
#define GUAHAO_REQUESTHEADER_H_
#include <string>
#include <map>

class CRequestHeader{
public:
	CRequestHeader();
	
	//CRequestHeader(const CRquestHeader& rhs);
	
	//CRequestHeader& operator= (const CRequestHeader& rhs);
	
	~CRequestHeader();

public:
	//请求头字段设置
	void 				SetHeaderField(const std::string &key,const std::string &value,bool is_merged=false);

	//请求正文设置
	void 				SetContent(const std::string &data);

	//请求行设置
	bool   				SetRequestLine(const std::string &data);

	//如果请求不合法返回空
	std::string 		GetRequest();

private:
	std::map<std::string,std::string> headers;

	std::string 		request_line;

	std::string 		request_content;
};

#endif //GUAHAO_REQUESTHEADER_H_
