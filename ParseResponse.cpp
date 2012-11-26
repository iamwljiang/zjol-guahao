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
 
#include "ParseResponse.h"
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <stdio.h>
#include <Zutil.h>
#include <iostream>
#include <stdlib.h>
#include "polarssl/md5.h"
#ifdef DEBUG_PARSERESPONSE
#include <iostream>
#endif

#ifdef WIN32
#define STRTOK strtok_s
#else
#define STRTOK strtok_r
#endif

#define SKIP_HEAD_BLANK(p,f) while((p+f) != NULL && (*(p+f)==' ' || *(p+f)=='\t')){f+=1;}

#define TOUPPER(str) for(size_t i=0;i<str.size();++i){str.at(i)=toupper(str.at(i));}
CParseResponse::CParseResponse(const char* res,int len)
{
	res_headers.clear();
	res_content.clear();
	protocol.clear();
	single_cookie_value.clear();
	set_cookies.clear();
	status_code = -1;
	res_buf = NULL;
	res_buf_len = len;
	parsed(res);
}

CParseResponse::~CParseResponse()
{
	res_headers.clear();
	res_content.clear();
	protocol.clear();
	status_code = -1;

	if(res_buf != NULL){
		free(res_buf);
	}
}

void CParseResponse::parsed(const char* res)
{
	if(res == NULL)
		return;

	//copy response data to inner buffer
	//int res_buf_len  = strlen(res);
	res_buf = (char*)malloc(res_buf_len+1);
	memset(res_buf,res_buf_len+1,0);
	memcpy(res_buf,res,res_buf_len);

	char sep1[] = "\r\n\r\n";
	char sep2[] = "\r\n";
	char* save_ptr1 = NULL;
	char* save_ptr2 = NULL;
	char* token1 = NULL;
	char* token2 = NULL;
	
	int header_size = NULL;
	
	//printf("token2:%s,res_buf_len:%d\n",token2,res_buf_len);
	char *str = NULL;
	int i = 0;
	for(str=res_buf; ;++i,str=NULL){
		token2 = STRTOK(str,sep2,&save_ptr2);
		if(token2 == NULL){
			break;
		}
		//printf("token2:%s,header_len:%d\n",token2,header_size);
		char * tsave_ptr = NULL;
		char * tresult   = NULL;
		if( i == 0){//parse response status line
			res_line.assign(token2);
			header_size+= strlen(token2)+2;
			tresult = STRTOK(token2," ",&tsave_ptr);
			if(tresult != NULL){
				protocol.assign(tresult,strlen(tresult));
				tresult = STRTOK(NULL," ",&tsave_ptr);
				if(tresult != NULL){
					status_code = atoi(tresult);
					tresult = STRTOK(NULL," ",&tsave_ptr);
					if(tresult == NULL){
						protocol.clear();
						status_code = -1;
						return;
					}
				}
			}
		}else{
			header_size+= strlen(token2)+2;
			tresult = STRTOK(token2,":",&tsave_ptr);
			if(tresult != NULL){
				std::string key ="";
				key.assign(tresult);
				tresult = STRTOK(NULL,":",&tsave_ptr);
				if(tresult != NULL){
					std::string value = "";
					value.assign(tresult);
					TOUPPER(key);
					res_headers.insert(std::make_pair(key,value));	
				}
			}
		}
		if(strncmp(save_ptr2,"\n\r\n",3) == 0){
			header_size+=2;
			break;
		}
	}
	
	//拆分出content
	std::map<std::string,std::string>::iterator iter = res_headers.find("CONTENT-LENGTH");
	if(iter != res_headers.end()){
		int content_length = 0;
		content_length = atoi(iter->second.c_str());
		if(content_length > 0){
			token1=STRTOK(NULL,sep2,&save_ptr2);
			if(token1 != NULL){
				res_content.assign(token1,content_length);
			}
		}
	}else{
		iter = res_headers.find("CONTENT-ENCODING");
		//有压缩初步都解释为gzip
		if(strlen(save_ptr2) > 3 && iter != res_headers.end()){
			char *decode_buf = NULL ;
			/*
			unsigned char md5_str[17]="";
			md5((unsigned char*)res_buf+header_size,res_buf_len-header_size,md5_str);
			printf("md5:");
			for(int i = 0; i < 16; ++i){
				printf("%02x",md5_str[i]);
			}
			printf("\n");
			*/
			int decode_len = 0;
			int ret = CZutil::decode_http_gzip(res_buf+header_size,res_buf_len - header_size,&decode_buf,&decode_len);
			if(ret != 0){
				//LOG here
				;
			}
			res_content.assign(decode_buf,decode_len);
			free(decode_buf);

		}else{//无压缩,但却没有指定content-length
			res_content.assign(res_buf + header_size,res_buf_len - header_size);
		}
	}

	iter = res_headers.find("SET-COOKIE");
	if(iter != res_headers.end()){
		parse_cookie(iter->second);
	}
}

void 		CParseResponse::parse_cookie(const std::string & str_cookie)
{
	size_t b = 0;
	size_t e = 0;
	std::string item = "";
	std::string key  = "";
	std::string value = "";

	while((e=str_cookie.find(";",b))!= std::string::npos){
		item = str_cookie.substr(b,e-b);
		b = e+1;

		size_t not_blank_pos = 0;
		SKIP_HEAD_BLANK(item.c_str(),not_blank_pos);
		size_t equal_pos = item.find("=");
		if(equal_pos != std::string::npos){
			key = item.substr(not_blank_pos,equal_pos-not_blank_pos);
			value = item.substr(equal_pos+1);
			res_cookies.insert(std::make_pair(key,value));
			if(key != "path")
				set_cookies += key + "=" + value + "; ";
		}else{
			single_cookie_value += item + "; ";
		}
		
	}

	if(b < str_cookie.size() - 1){
		item = str_cookie.substr(b);

		size_t not_blank_pos = 0;
		SKIP_HEAD_BLANK(item.c_str(),not_blank_pos);
		size_t equal_pos = item.find("=");
		if(equal_pos != std::string::npos){
			key = item.substr(not_blank_pos,equal_pos-not_blank_pos);
			value = item.substr(equal_pos+1);
			res_cookies.insert(std::make_pair(key,value));
		}else{
			single_cookie_value += item + "; ";	
		}
	}

	return;
}


std::string 	CParseResponse::GetSetCookies()
{
	return set_cookies;
}

std::string 	CParseResponse::GetSetCookie(const std::string &key)
{
	std::map<std::string,std::string>::iterator iter = res_cookies.find(key);
	if(iter != res_cookies.end()){
		return iter->second;
	
		return "";
	}
}

std::string 		CParseResponse::GetHeaderField(const std::string &key)
{
	std::map<std::string,std::string>::iterator iter = res_headers.find(key);
	if(iter != res_headers.end()){
		return iter->second;
	}else{
		return "";
	}
}

std::string 		CParseResponse::GetContent()
{
	return res_content;
}

std::string 		CParseResponse::GetHeader()
{
	std::string msg = res_line+"\r\n";
	std::map<std::string,std::string>::iterator iter = res_headers.begin();
	for(; iter != res_headers.end(); ++iter){
		msg += iter->first + ":" + iter->second + "\r\n";
	}

	return msg;
}


#ifdef DEBUG_PARSERESPONSE

void test-response-header()
{
	std::string header="";
	header+="HTTP/1.1 200 OK\r\n";
	header+="Date: Thu, 25 Oct 2012 08:04:09 GMT\r\n";
	header+="Expires: -1\r\n";
	header+="Cache-Control: private, max-age=0\r\n";
	header+="Content-Type: text/html; charset=ISO-8859-1\r\n";
	header+="Set-Cookie: NID=65=ttMRcuI6xR551KN7vFGU36BnhxW5yIOEgcGsIh7g9KF3omIcXnTVwylhUctndhP8_QX_BcRmj1GnFZt0oYKlruKqehZTIWJVNqQScNPu5SSknltg2amt0jDzzJJ1KxB2; expires=FrRIGIN\r\n";
	header+="Transfer-Encoding: chunked\r\n\r\n";
	CParseResponse rp(header.c_str());
	std::cout << rp.GetHeader() << std::endl;
	std::cout << rp.GetCookies() << std::endl;
}

void test-response-data()
{
	std::string header="";
	header+="HTTP/1.1 200 OK\r\n";
	header+="Server	nginx/1.0.4\r\n";
	header+="Date	Thu, 25 Oct 2012 09:28:13 GMT\r\n";
	header+="Content-Type	text/plain; charset=utf-8\r\n";
	header+="Connection	keep-alive\r\n";
	header+="X-Powered-By	ASP.NET\r\n";
	header+="X-AspNet-Version	2.0.50727\r\n";
	header+="Cache-Control	private\r\n";
	header+="Content-Length	1146\r\n";
	header+="\r\n";
	header+="957101#浙江省人民医院#方便门诊#普通门诊#158790#330802198702191630#江文龙#\
	1567303#20121030#0#1#$18538308|1|0830$18538309|2|0832$18538310|3|0834$18538311|4|0836$18538312|5|\
	0838$18538313|6|0840$18538314|7|0842$18538315|8|0844$18538316|9|0846$18538317|10|0848$18538318|11|\
	0850$18538319|12|0852$18538320|13|0854$18538321|14|0856$18538322|15|0858$18538323|16|0900$18538324|\
	17|0902$18538325|18|0904$18538326|19|0906$18538327|20|0908$18538328|21|0910$18538329|22|0912$18538330|\
	23|0914$18538331|24|0916$18538332|25|0918$18538333|26|0920$18538334|27|0922$18538335|28|0924$18538336|\
	29|0926$18538337|30|0928$18538338|31|0930$18538339|32|0932$18538340|33|0934$18538341|34|0936$18538342|\
	35|0938$18538343|36|0940$18538344|37|0942$18538345|38|0944$18538346|39|0946$18538347|40|0948$18538348|\
	41|0950$18538349|42|0952$18538350|43|0954$18538351|44|0956$18538352|45|0958$18538353|46|1000$18538354|\
	47|1002$18538355|48|1004$18538356|49|1006$18538357|50|\
	1008#A3036D3357970C8EA046F1EB9ED6D79BDCA9A35CE87C688C3CBF2DC58A6A5FC514EB2\
	65F97DF3437C3E4142123B1CCD1D3BFB8522E4432E4CE12BD5C822BC416A34EE756386C62E\
	973DC25ABCD8553F4A36A31B4DB821A3D505AE4FB8BB0EB0B";
	CParseResponse rp(header.c_str());
	std::cout << rp.GetHeader() << std::endl;
	std::cout << rp.GetContent() << std::endl;
}

int main()
{
	test-response-header();
	test-response-data();
	return 0;
}
#endif
