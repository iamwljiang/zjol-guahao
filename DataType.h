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
#ifndef GUAHAO_DATATYPE_H_
#define GUAHAO_DATATYPE_H_
#include <string>
#include <vector>
#include <map>

typedef struct  _tag_HOSPITAL_INFO
{
	std::string addr; //哪的医院
	std::string tel;  //联系电话
	std::string uri;  //uri
	std::string level;//医院级别
	std::string name; //医院名称
}HOS_INFO;


typedef struct _tag_DEPARTMENT_INFO
{
	std::string parent;//这个暂时不解析出来
	std::string uri;
	std::string name;//科室名称
}DEPART_INFO;

typedef struct _tag_DETAIL_DOC{
	std::string content;//请求正文
	std::string date;   //日期
	std::string weekday;//星期几
	int 		nweekday;//数字表示的礼拜几(0-6);
	int			ampm;	//上午下午
	int 		emptyflag;//空标志 0不空,1空
	std::string cost;   //费用
}DETAIL_INFO;

typedef struct _tag_REVERSTION_DOC
{
	std::string name;                 //医生名称
	std::vector<DETAIL_INFO> DVECTOR; //只存储有号源的数据
}REVERSTION_DOC;

struct _tag_CLIENT_INFO{
		int current_request_type;
		int is_lock;	//如果在请求没有完成,则这个状态进行锁定
		long last_active_time;
		char *res_data; //apr_pool 分配,保持响应内容
		int  res_len;
		void *data;		//allow user use data;
		_tag_CLIENT_INFO(){
			current_request_type  = 0;
			is_lock = 0;
			last_active_time = time(NULL);
			res_data = NULL;
			res_len  = 0;
			data    = NULL;
		}
		~_tag_CLIENT_INFO(){
		}

};
typedef _tag_CLIENT_INFO CLIENT_INFO;

typedef std::map<std::string,HOS_INFO> 	 HOSMAP;
typedef std::map<std::string,DEPART_INFO> DEPMAP;
typedef std::map<std::string,std::vector<DETAIL_INFO> > DETMAP;

#define MAX_ITEM_LEN 20
typedef struct _tag_RES_DOC{
	char	sfz[MAX_ITEM_LEN];
	char	xm[MAX_ITEM_LEN];
	char	yymc[MAX_ITEM_LEN];
	char 	ksmc[MAX_ITEM_LEN];
	char 	ysmc[MAX_ITEM_LEN]; 
	char  	ghfy[MAX_ITEM_LEN];
	char  	jzrq[MAX_ITEM_LEN];

	char 	mgenc[256];
	char    hy[256];//应该够了,如果超出则截断就行

	char    js[MAX_ITEM_LEN];//dialogb.js中第一个变量名

	char    yzm[MAX_ITEM_LEN];//验证码
}RES_DOC;
#endif //GUAHAO_DATATYPE_H_
