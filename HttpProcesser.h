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
*desc  :用于HTTP消息发送和接受
*warn  :最好将socket部分抽取出来,在socket部分来支持apr,asio,libevent,nature socket
***************************************/
#ifndef GUAHAO_HTTPPROCESSER_H_
#define GUAHAO_HTTPPROCESSER_H_
#include <string>
#include <map>
#include <vector>
#include "apr.h"
#include "apr_lib.h"
#include "apr_general.h"
#include "apr_poll.h"
#include "apr_pools.h"
#include "apr_network_io.h"
#include "apr_atomic.h"
#include "apr_time.h"
#include "DataType.h"
#include "ParseResponse.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#define DEL(o) if(o){delete o;o=NULL;}

#define RECV_LEN 1024*10

enum REQUEST_TYPE{
	BEGIN=0,
	INDEX=1,	//首页
	VERIA=2,	//首页登陆验证码
	LOGIN=3,	//发起登陆
	HOS=4,  	//医院科室
	DEPART=5, //科室医生,解析出医生存在号源信息
	DOCTOR=6, //请求挂号到医生
	JS=7,		//确认预约的js,需要从其中抽取一个字段	
	VERIB=8,  //请求挂号到医生的验证码
	CONFIRM=9	//提交确认预约
};

//class CParseResponse;
//class CParseHtml;

//NOTE:关于返回值
//如果= 0表示处理完成
//如果< 0表示处理失败,或条件不符
#ifdef APR
class CHttpProcesser
{
public:
	CHttpProcesser();
	
	~CHttpProcesser();

	int				Init(const char* addr,int port);

	void 			Run();

	//copy session,hospital_name,depart_name,doctor_name,date and the other things;
	void			Clone(CHttpProcesser &rhs);

	void 			SetDay(const std::string& day);

	void 			SetDatename(const std::string& hos,const std::string& dep,const std::string& doc);
#ifdef USE_BOOST_THREAD
private:
	boost::thread   proc_thread;
public:
	void 			start_thread();
#endif

public:	
	//设置socket参数并发起连接
	int 			NewConnection(int block_flag = 1);
	//处理活跃连接
	int 			ProcessActive(int,const apr_pollfd_t*);
	//剔除不活跃连接
	int 			ProcessNotActive(int interval_time);
	//请求首页
	int				RequestIndex(apr_socket_t* s,CLIENT_INFO* ci);
	//后期改成登陆一次
	int				RequestLogin(apr_socket_t* s,CLIENT_INFO* ci);
	//请求医院
	int 			RequestHospital(apr_socket_t* s,CLIENT_INFO* ci);
	//请求科室
	int 			RequestDepartment(apr_socket_t* s,CLIENT_INFO* ci);
	//请求挂号
	int 			RequestDoctor(apr_socket_t* s,CLIENT_INFO* ci);
	//请求验证码,分别是登陆验证码和取号验证码
	int 			RequestVerify(apr_socket_t* s,CLIENT_INFO* ci);;
	//请求确认预约的js,提取个字段
	int 			RequestDialogbJS(apr_socket_t* s,CLIENT_INFO* ci);
	//组织最终的预约请求
	int				RequestConfirmDoctor(apr_socket_t *s,CLIENT_INFO* ci);
	
	//处理首页
	int 			ProcessIndexResult(apr_socket_t* s,CLIENT_INFO* ci);
	//处理验证码
	int 			ProcessVerifyResult(apr_socket_t* s,CLIENT_INFO* ci);
	//处理登陆
	int				ProcessLoginResult(apr_socket_t* s,CLIENT_INFO* ci);
	//处理医院
	int 			ProcessHospitalResult(apr_socket_t* s,CLIENT_INFO* ci);
	//处理科室
	int 			ProcessDepartmentResult(apr_socket_t* s,CLIENT_INFO* ci);
	//处理挂号
	int 			ProcessDoctorResult(apr_socket_t* s,CLIENT_INFO* ci);
	//处理js提取个字段
	int 			ProcessDialogbJSResult(apr_socket_t* s,CLIENT_INFO* ci);

	int				ProcessConfirmResult(apr_socket_t *s,CLIENT_INFO* ci);
	//结束运行
	void 			Stop();

	void 			Clear();

	//执行一次用于获取hospital map
	int 			RunOnceGetHosmap(void* out_map);

	//执行一次用于获取department map
	int 			RunOnceGetDepmap(const std::string& name,void* out_map);

	//执行一次用于获取doctor map
	int 			RunOnceGetDocmap(const std::string& name,void* out_map);

	//登陆一次
	int 			RunOnceTestLogin(const std::string& user,const std::string& passwd);
	//只创建socket
	apr_socket_t* 	make_socket();

	int 			add_socket_to_pollset(apr_socket_t* s,int events,void* data);

	int 			clear_socket_from_pollset(apr_socket_t* s,void* data);

	void			close_connection(apr_socket_t* s ,CLIENT_INFO* ci);

	char* 			recv_data(apr_socket_t* s,int *result_len);

	bool 			send_data(apr_socket_t* s,const char* data,int len);

	int 			has_chunk(const char *data);

	//初步通过read_chunk读取剩余数据,直到读取完成
	int 			read_chunk(apr_socket_t* s,char *fsrc,int flen,char **data,int *tlen);

	//解析不完整的chunk段,可能有多个chunk组成
	int 			parse_chunk(char *chunk,int chunk_len,int last_chunk_len,char *save,int *save_pos);

	//用于封装recv_data
	int 			read_data(apr_socket_t* s ,CLIENT_INFO* ci);

	//导出图片数据并从输入流中读取验证码结果
	int 			use_feh_read_verify_code(const char *img,int len,char *result);

	void 			set_visit_cookie(void * /*CRequestHeader* */);
private:
	/***************start 用户可设置数据 start********************/
	std::string		user;     //登陆账号

	std::string		password; //登陆密码

	//如果是ide界面,预约信息可以将所有可以预约的医院,科室，医生返回给用户,让其选择
	std::string 	user_date_hospital; //用户要预约哪个医院

	std::string 	user_date_department;//用户要预约的科室

	std::string 	user_date_doctor;	 //用户要预约的医生

	int 			nwant_day;			//需要预约到那天
	/***************end 用户可设置数据 end**********************/

	/***************start session 会话过程需要使用的数据*************/
	std::string 	session_cookies;

	char 			verify_login_image[255];

	std::string		refer_uri;

	std::string		user_id; //保存，但不使用

	//医院信息
	HOSMAP 			hospital_map;
	//科室信息
	DEPMAP  		department_map;
	//医生信息
	DETMAP  		detail_doc_map;
	/***************end session 会话过程需要使用的数据*************/

	/***************connection*************************************/
	//活跃连接管理
	//TODO:其实可以用不map,用vector就可以
	typedef std::map<apr_socket_t*,CLIENT_INFO*> MCMAP;
	MCMAP 			manager_connections;
	//等待连接管理
	typedef std::map<apr_socket_t*,long>		 WTMAP;
	WTMAP 			wait_connections;
	/***************connection*************************************/
	
	apr_sockaddr_t 	*server_address;
	
	apr_pool_t 		*sub_pool;

	apr_pollset_t 	*pollset;

	unsigned int	exit_flag;

	int 			is_first_login;

	//新发起的连接数,包含等待连接和已连接,随新建连接和关闭连接进行动态变化
	int 			new_connection_num; 
public:
	static int 		MAX_CONNECTION;
};

#elif defined(ASIO)
#else
#endif

#endif //GUAHAO_HTTPPROCESSER_H_
