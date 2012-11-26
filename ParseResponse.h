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
*desc  :用于解析HTTP响应
*
*
***************************************/
#ifndef GUAHAO_PARSERESPONSE_H_
#define GUAHAO_PARSERESPONSE_H_

#include<string>
#include<map>

class CParseResponse{
public:
	CParseResponse(const char * res,int len);
	
	~CParseResponse();

public:
	//获取需要设置的cookie
	std::string 		GetSetCookies();
	//从setcookie根据key,取value
	std::string 		GetSetCookie(const std::string &key);

	std::string 		GetHeaderField(const std::string &key);

	std::string 		GetContent();

	std::string 		GetHeader();

private:
	void 				parsed(const char * res);

	void 				parse_cookie(const std::string & str_cookie);
private:
	std::map<std::string,std::string> res_headers;
	
	std::map<std::string,std::string> res_cookies;

	std::string 		res_content;

	std::string 		protocol;

	std::string 		single_cookie_value;//保存在set-cookies中的没有value的单个项,使用';'连接

	std::string 		set_cookies;

	std::string 		res_line;
	
	int					status_code;
	
	int 				res_buf_len;

	char *				res_buf;
};


#endif //GUAHAO_PARSERESPONSE_H_
