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
 
#include "RequestHeader.h"
#include <algorithm>

#ifdef DEBUG_REQUESTHEADER
#include <iostream>
#endif 

#define CRLR "\r\n"

CRequestHeader::CRequestHeader()
{
	headers.clear();
	request_line    = "";
	request_content = "";
}

CRequestHeader::~CRequestHeader()
{
	headers.clear();
	request_line    = "";
	request_content = "";
}


void 				CRequestHeader::SetHeaderField(const std::string &key,const std::string &value,bool is_merged)
{
	if(key.empty() || value.empty())
		return ;
	if(is_merged == false)
		headers.insert(std::make_pair(key,value));
	else{
		std::map<std::string,std::string>::iterator iter = headers.find(key);
		if(iter == headers.end()){
			headers.insert(std::make_pair(key,value));
		}else{
			iter->second += " " + value;
		}
	}
}

void 				CRequestHeader::SetContent(const std::string &data)
{
	request_content = data;
}

bool   				CRequestHeader::SetRequestLine(const std::string &data)
{
	size_t e = 0;
	size_t b = 0;
	std::string method;
	std::string uri;
	std::string protocol;

	while((e=data.find(" ",b)) != std::string::npos){
		std::string item  = data.substr(b,e-b);
		b = e+1;
		item.erase(std::remove(item.begin(),item.end(),' '),item.end());
		if(item.empty())
			continue;
		if(method.empty()){
			method = item;
		}else if(uri.empty()){
			uri = item;
		}else if(protocol.empty()){
			protocol = item;
		}
	}

	if(!protocol.empty()){
		return false;
	}
	
	protocol = data.substr(b);
	protocol.erase(std::remove(protocol.begin(),protocol.end(),' '),protocol.end());
	
	//检测每个单元是否合法
	if(method != "GET" && method != "POST" && method != "HEAD"){
		return false;
	}
	
	if(protocol != "HTTP/1.1" && protocol != "HTTP/1.0"){
		return false;
	}

	if(uri.at(0) != '/'){
		return false;
	}	

	request_line = data;
}

std::string 		CRequestHeader::GetRequest()
{
	if(request_line.empty()){
		return "";
	}

	std::string all_header = request_line + CRLR;

	std::map<std::string,std::string>::iterator iter;
	for(iter = headers.begin(); iter != headers.end(); ++iter){
		all_header += iter->first + ":" + iter->second + CRLR;
	}

	all_header += CRLR;
	if(!request_content.empty()){
		all_header += request_content + CRLR;
	}

	return all_header;
}


#ifdef DEBUG_REQUESTHEADER

void test-no-content()
{
	CRequestHeader rh;
	std::string header="";
	header="GET / HTTP/1.1";
	rh.SetRequestLine(header);
	rh.SetHeaderField("HOST","www.google.com");
	rh.SetHeaderField("Cache-Control","no-cache");
	rh.SetHeaderField("Cookie","NID=65=ttMRcuI6xR551KN7vFGU36BnhxW5yIOEgcGsIh7g9KF3omIcXnTVwylhUctndhP8_QX_BcRmj1GnFZt0oYKlruKqehZTIWJVNqQScNPu5SSknltg2amt0jDzzJJ1KxB2; expires=FrRIGIN");
	rh.SetHeaderField("Connection","Keep-Alive");
	rh.SetHeaderField("User-Agent","Mozilla/5.0");
	std::cout << rh.GetRequest() << std::endl;
}

vpod test-have-content()
{
	CRequestHeader rh;
	rh.SetRequestLine("POST /ashx/gethy.ashx HTTP/1.1");
	rh.SetHeaderField("x-requested-with","XMLHttpRequest");
	rh.SetHeaderField("Accept-Language","zh-cn");
	rh.SetHeaderField("Referer","http://guahao.zjol.com.cn/DepartMent.Aspx?ID=94");
	rh.SetHeaderField("Accept","*/*");
	rh.SetHeaderField("Content-Type","application/x-www-form-urlencoded");
	rh.SetHeaderField("Accept-Encoding","gzip, deflate");
	rh.SetHeaderField("User-Agent","Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; WOW64; Trident/5.0)");
	rh.SetHeaderField("Host","guahao.zjol.com.cn");
	rh.SetHeaderField("Content-Length","131");
	rh.SetHeaderField("Connection","Keep-Alive");
	rh.SetHeaderField("Cache-Control","no-cache");
	rh.SetHeaderField("Cookie""visitor_user_id=1350455740756; lastVisitTime=Thu%20Oct%2025%2017%3A22%3A08%20UTC+0800%202012; \
	CNZZDATA30020775=cnzz_eid=87815617-1350455744-&ntime=1351156943&cnzz_a=60&retime=1351156928080&sin=none&ltime=1351156928080&rtime=4; \
	Hm_lvt_59e375a00c82de49535582c44a6bd80e=1351127484386,1351142391627,1351144458869,1351156794040; UserId=158790; \
	ASP.NET_SessionId=5xyn4d55el53rrnzzph5ui45; Hm_lpvt_59e375a00c82de49535582c44a6bd80e=1351156928403; \
	Hm_lvt_419cfa1cc17e2e1dc6d4f431f8d19872=1351127482942,1351142389356,1351144457949,1351156793159; \
	Hm_lpvt_419cfa1cc17e2e1dc6d4f431f8d19872=1351156928024");

	rh.SetContent("sg=A3036D3357970C8EA046F1EB9ED6D79BDCA9A35CE87C688C3CBF2DC58A6A5FC5\
		14EB265F97DF3437A434BFCD4478FEA4816B6AA6DBC980F84406FF1E5C6889DB");
		std::cout << rh.GetRequest() << std::endl;
}
int main()
{
	test-no-content();

	test-have-content();
	return 0;
}
#endif
