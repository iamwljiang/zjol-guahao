#include "HttpProcesser.h"
#include "ParseResponse.h"
#include "RequestHeader.h"
#include "ParseHtml.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "polarssl/md5.h"
#include "Urlcode.h"
#include "Strutil.h"
#include <algorithm>
#ifdef APR

#define IE_USER_AGENT "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; WOW64; Trident/5.0)"
#define CHROME_USER_AGENT "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.4 (KHTML, like Gecko) Chrome/22.0.1229.94 Safari/537.4"

#define LOG_TO_FILE(filename,data) std::ofstream out(filename); out << data ; out.close();
#define LOG_SOCK_ERROR(rv,errstr) char buf[200];apr_strerror(rv,buf,sizeof(buf));printf("%s:%s\n",errstr,buf);

typedef int (*PRINT_TYPE)(const char*,...);

PRINT_TYPE LOG_DEBUG = debug_log;

int CHttpProcesser::MAX_CONNECTION  = 1;
CHttpProcesser::CHttpProcesser()
{
	apr_initialize();
	is_first_login = true;
	exit_flag = 0;
	sum = 0;
	user = "350521197404071798";
	password = "071798";
	user_date_hospital = "浙二医院";
	//user_date_department = "内分泌科";
	//user_date_doctor   = "王永健";
	user_date_department = "外科";
	user_date_doctor   = "徐斌";
	memset(verify_login_image,0,255);
}

CHttpProcesser::~CHttpProcesser()
{
	apr_terminate();
}

int	CHttpProcesser::Init(const char* addr,int port)
{
	if(APR_SUCCESS != apr_pool_create(&sub_pool,NULL))
		return -1;
	if(APR_SUCCESS != apr_sockaddr_info_get(&server_address,addr,AF_INET,port,0,sub_pool)){
		return -2;
	}

	if(APR_SUCCESS != apr_pollset_create(&pollset, MAX_CONNECTION, sub_pool, 0)){
		return -3;
	}
}

apr_socket_t* 	CHttpProcesser::make_socket()
{
	apr_socket_t *sock = NULL;
	if(APR_SUCCESS != apr_socket_create(&sock, APR_INET, SOCK_STREAM, 0, sub_pool))
		return NULL;

	//apr_socket_opt_set(sock,APR_SO_NONBLOCK,1);
	apr_socket_opt_set(sock,APR_SO_KEEPALIVE,1);

	return sock;
}

int CHttpProcesser::NewConnection()
{
	if(sum >= MAX_CONNECTION){
		return 0;
	}

	apr_socket_t *sock = make_socket();
	if(sock == NULL){
		//TODO:log here
		LOG_DEBUG("make socket error");
		return -1;
	}

	apr_status_t rv = apr_socket_connect(sock,server_address);
	if(rv == APR_SUCCESS){
		//connection complete
		CLIENT_INFO * ci = new CLIENT_INFO;
		ci->current_request_type = BEGIN;
		manager_connections.insert(std::make_pair(sock,ci));
		add_socket_to_pollset(sock,APR_POLLIN,NULL);
		sum +=1;
		LOG_DEBUG("NewConnection connected");
	}else if (APR_STATUS_IS_EINPROGRESS(rv)){
		//async connect,check again
		add_socket_to_pollset(sock,APR_POLLIN|APR_POLLOUT,NULL);
		wait_connections.insert(std::make_pair(sock,time(NULL)));
		sum +=1;
		LOG_DEBUG("NewConnection socket interprocess,next check");
	}else{
		//TODO:log here
		LOG_DEBUG("NewConnection can't connect,close socket");
		apr_socket_close(sock);
		return -2;
	}

	return 0;
}

int 	CHttpProcesser::add_socket_to_pollset(apr_socket_t* s,int events,void *data)
{
	apr_pollfd_t pfd ;
	pfd.p 			= sub_pool;
	pfd.desc_type 	= APR_POLL_SOCKET;
	pfd.reqevents 	= events;
	pfd.desc.s  	= s;
	if(data == NULL){
		pfd.client_data = s;
	}else{
		pfd.client_data = data;
	}
	apr_pollset_add(pollset,&pfd);
	return 0;
}

int 	CHttpProcesser::clear_socket_from_pollset(apr_socket_t* s,void *data)
{
	apr_pollfd_t pfd ;
	pfd.p 			= sub_pool;
	pfd.desc_type 	= APR_POLL_SOCKET;
	pfd.desc.s  	= s;
	if(data == NULL){
		pfd.client_data = s;
	}else{
		pfd.client_data = data;
	}

	apr_pollset_remove(pollset,&pfd);
	return 0;
}

void	CHttpProcesser::close_connection(apr_socket_t* s ,CLIENT_INFO* ci)
{
	LOG_DEBUG("close_connection called");
	MCMAP::iterator iter = manager_connections.find(s);
	if(iter != manager_connections.end()){
		ci == iter->second ? ci : iter->second;
		manager_connections.erase(iter);
	}

	clear_socket_from_pollset(s,NULL);

	apr_socket_close(s);
	DEL(ci);
	sum -= 1;
	s = NULL;
}
int 	CHttpProcesser::ProcessActive(int num,const apr_pollfd_t *pfd)
{
	apr_socket_t* s;
	for(int i = 0; i < num; ++i){
		if(pfd[i].rtnevents == (APR_POLLIN | APR_POLLOUT)){
			s = (apr_socket_t*)pfd[i].client_data;
			wait_connections.erase(s);
			CLIENT_INFO * ci = new CLIENT_INFO;
			ci->current_request_type = BEGIN;
			ci->last_active_time     = time(NULL);
			manager_connections.insert(std::make_pair(s,ci));
			clear_socket_from_pollset(s,NULL);
			add_socket_to_pollset(s,APR_POLLIN,NULL);
			sum+=1;
			LOG_DEBUG("ProcessActive use pollin and pollout checked connected");
		}else if(pfd[i].rtnevents & APR_POLLIN){
			s = (apr_socket_t*)pfd[i].client_data;
			LOG_DEBUG("ProcessActive socket have data need read,socket is %s,sock addr:%p,map size:%d"\
				,s == NULL? "null" : "not null",s,manager_connections.size());
			MCMAP::iterator iter = manager_connections.find(s);
			if(iter != manager_connections.end()){
				//NOTE:在处理函数借正确结束时才解锁
				//iter->second->is_lock = 0;
				//iter->second->last_active_time = time(NULL);

				LOG_DEBUG("ProcessActive response type:%d",iter->second->current_request_type);
				int ret = 0;
				switch(iter->second->current_request_type){
					case INDEX:
						ret = ProcessIndexResult(iter->first,iter->second);
						break;
					case VERIA:
						ret = ProcessVerifyResult(iter->first,iter->second);
						break;					  	
					case LOGIN:
						ret = ProcessLoginResult(iter->first,iter->second);
						break;
					case HOS:
						ret = ProcessHospitalResult(iter->first,iter->second);
						break;
					case DEPART:
						ret = ProcessDepartmentResult(iter->first,iter->second);
						break;
					case DOCTOR:
						ret = ProcessDoctorResult(iter->first,iter->second);
						break;
					case JS:
						ret = ProcessDialogbJSResult(iter->first,iter->second);
						break;
					case VERIB:
					case CONFIRM:
						ret = ProcessConfirmResult(iter->first,iter->second);
						break;
					default:
						break;
				}//end switch
				if(ret < 0){
					LOG_DEBUG("ProcessActive call process error,ret:%d",ret);
				}
			}else{
				LOG_DEBUG("ProcessActive can't find socket from connection manager");
				exit(1);
			}
			//end if
		}else if(pfd[i].rtnevents & APR_POLLOUT){
			s = pfd[i].desc.s;
			WTMAP::iterator iter = wait_connections.find(s);
			if(wait_connections.end() != iter){
				//check apr connection is connected we need connect again ,this method from ab
				int rv = apr_socket_connect(s,server_address);
				if(rv != APR_SUCCESS){
					if(time(NULL) - iter->second > 2){
						clear_socket_from_pollset(s,NULL);
						wait_connections.erase(iter);
					}
				}else{
					wait_connections.erase(iter);
					clear_socket_from_pollset(s,NULL);
					add_socket_to_pollset(s,APR_POLLIN,NULL);
					CLIENT_INFO * ci = new CLIENT_INFO;
					ci->current_request_type = BEGIN;
					ci->last_active_time     = time(NULL);
					manager_connections.insert(std::make_pair(s,ci));
					LOG_DEBUG("ProcessActive checked connection connected");
					sum += 1;
				}
			}
		}else{
			//LOG here
			LOG_DEBUG("unkown event:%d",pfd[i].rtnevents);
			continue;
		}
	}//end for

	return 0;
}


int 	CHttpProcesser::ProcessNotActive(int interval_time)
{
	if(!manager_connections.empty()){
		MCMAP::iterator conn_iter = manager_connections.begin();
		MCMAP::iterator conn_end  = manager_connections.end();
		MCMAP::iterator next_iter ;
		time_t now = time(NULL);
		for(; conn_iter != conn_end; ){
			next_iter = conn_iter;
			++next_iter;
			if(now - conn_iter->second->last_active_time > interval_time){
				LOG_DEBUG("find connection but not active,close it");
				close_connection(conn_iter->first,conn_iter->second);
			}
			conn_iter = next_iter;
		}
		manager_connections.clear();
	}

	if(!wait_connections.empty()){
		WTMAP::iterator wait_iter = wait_connections.begin();
		WTMAP::iterator wait_end  = wait_connections.end();
		time_t now = time(NULL);
		for(; wait_iter != wait_end; ++wait_end){
			if(now - wait_iter->second > interval_time){
				LOG_DEBUG("find wait_connection not active,close it");
				apr_socket_close(wait_iter->first);
			}
		}
		wait_connections.clear();
	}

	return 0;
}

void 	CHttpProcesser::Run()
{
	apr_interval_time_t wait_time = 30;
	int 				active_num= 0;
	
	time_t 				last_check_time = time(NULL);
	int 				check_interval  = 10;
	int 				not_active_interval = 10;

	const apr_pollfd_t 		*pfd      = NULL;
	while(!apr_atomic_read32(&exit_flag)){
		
		NewConnection();
		
		if(APR_SUCCESS == apr_pollset_poll(pollset,wait_time,&active_num,&pfd)){

			ProcessActive(active_num,pfd);
		}

		MCMAP::iterator iter = manager_connections.begin();
		MCMAP::iterator next_iter ;
		for(; iter != manager_connections.end()/*map会变动*/; ){
			next_iter = iter;
			++next_iter;
			//TODO:可以通过type确定调用哪个函数,而不用每个函数都调用一次
			//还需要保证迭代器删除的时候,不影响下后续迭代器移动
			if(iter->second->is_lock == 0){
				LOG_DEBUG("CHttpProcesser::Run connection manager size:%d",manager_connections.size());
				switch(iter->second->current_request_type){
					case BEGIN:
						RequestIndex(iter->first,iter->second);
						break;
					case INDEX:
					case VERIA:		
						RequestLogin(iter->first,iter->second);
						break;
					case LOGIN:	
						RequestHospital(iter->first,iter->second);
						break;	
					case HOS:
						RequestDepartment(iter->first,iter->second);
			 			break;
			 		case DEPART:	
						RequestDoctor(iter->first,iter->second);
						break;
					case DOCTOR:
						RequestDialogbJS(iter->first,iter->second);
						break;
					case JS:
					case VERIB:
						RequestConfirmDoctor(iter->first,iter->second);
					default:
						break;
				}
			}

			iter = next_iter;
		}

		/*
		time_t now = time(NULL);
		if(now - last_check_time > check_interval){
			ProcessNotActive(check_interval);
		}
		*/
		usleep(30);
	}//end while
	//TODO:清理
	Clear();
}

void CHttpProcesser::Clear()
{
	if(!manager_connections.empty()){
		MCMAP::iterator conn_iter = manager_connections.begin();
		MCMAP::iterator conn_end  = manager_connections.end();
		MCMAP::iterator next_iter ;
		for(; conn_iter != conn_end; ){
			next_iter = conn_iter;
			++next_iter;
			close_connection(conn_iter->first,conn_iter->second);
			conn_iter = next_iter;
		}
		manager_connections.clear();
	}

	if(!wait_connections.empty()){
		WTMAP::iterator wait_iter = wait_connections.begin();
		WTMAP::iterator wait_end  = wait_connections.end();
		for(; wait_iter != wait_end; ++wait_end){
			apr_socket_close(wait_iter->first);
		}
		wait_connections.clear();
	}
}




bool 	CHttpProcesser::send_data(apr_socket_t* s,const char* data,int len)
{
	size_t remind_len = len;
	size_t sended_len = 0;
	bool   finish   = true;
	//初步不考虑如果eagain的时候下次继续尝试
	apr_status_t rv;
	do{
	    rv = apr_socket_send(s,data+sended_len,&remind_len);

	    if(rv != APR_SUCCESS && !APR_STATUS_IS_EAGAIN(rv)){
	     	LOG_SOCK_ERROR(rv,"apr_socket_send error");
	     	//外部检测到需要关闭连接
	    	return false;
	    }

		sended_len += remind_len;
		remind_len = len - sended_len;
	}while(remind_len);
	
	return finish;
}


char*	CHttpProcesser::recv_data(apr_socket_t* s,int* result_len)
{
	*result_len = 0;
	size_t want_recv_len = RECV_LEN;
	size_t recv_len = 0;
	size_t current_recv_len = 0;
	size_t total_buffer_len = RECV_LEN;
	char * buffer = (char*)apr_pcalloc(sub_pool,total_buffer_len);
	int    mix = 1;//buffer 增长基数

	do{
		current_recv_len = want_recv_len;
		apr_status_t rv = apr_socket_recv(s,buffer+recv_len,&want_recv_len);
		
		
		recv_len += want_recv_len;	
		//std::cout << "current want recv len:" << current_recv_len << " recv len:"<< want_recv_len<< "\n";
		//如果需要接受的长度>大于返回的长度则break！
		if(current_recv_len > want_recv_len){
			break;
		}
		//赋予当前返回长度,作为之后while判断条件
		current_recv_len = want_recv_len;

		if(current_recv_len == 0 && APR_STATUS_IS_EOF(rv)){
			LOG_SOCK_ERROR(rv,"apr_socket_recv eof");
			break;
		}else if(APR_STATUS_IS_EAGAIN(rv)){
			LOG_SOCK_ERROR(rv,"apr_socket_recv eagain");
		}else if(rv != APR_SUCCESS){
			LOG_SOCK_ERROR(rv,"apr_socket_recv error");
			break;
		}
		
		
		if(recv_len > 3*(total_buffer_len / 4)){
			mix = mix << 1;
			total_buffer_len = total_buffer_len * mix;
			char *temp_buf = (char*)apr_pcalloc(sub_pool,total_buffer_len);
			memcpy(temp_buf,buffer,recv_len);
			buffer = temp_buf;
		}
		
		//剩余可用于接收的空闲buffer长度
		want_recv_len = total_buffer_len - recv_len;
	}while(current_recv_len);

	if(recv_len > 0){
		*result_len = recv_len;
		return buffer;
	}else{
		return NULL;
	}
}

/*
char*	CHttpProcesser::recv_data(apr_socket_t* s,int* result_len)
{
	size_t want_recv_len = RECV_LEN;
	size_t prev_len = want_recv_len;
	size_t recv_len = 0;
	char * buffer = (char*)apr_pcalloc(sub_pool,want_recv_len);
	int    mix = 1;
	while(1){
		apr_status_t rv = apr_socket_recv(s,buffer+recv_len,&want_recv_len);
		if(rv != APR_SUCCESS){
			LOG_SOCK_ERROR(rv,"apr_socket_recv error");
			break;
		}
		
		std::cout << "want recv len:" << want_recv_len << "\n";

		recv_len += want_recv_len;	
		if(want_recv_len < prev_len){
			break;	
		}

		if(recv_len > 3*(RECV_LEN / 4)){
			mix = mix << 1;
			char *temp_buf = (char*)apr_pcalloc(sub_pool,RECV_LEN * mix);
			memcpy(temp_buf,buffer,recv_len);
			buffer = temp_buf;
			want_recv_len = mix * RECV_LEN  - recv_len;
		}else{
			want_recv_len = RECV_LEN - recv_len;
		}

		prev_len = want_recv_len;
	}

	if(recv_len > 0){
		*result_len = recv_len;
		return buffer;
	}

	*result_len = 0;
	return NULL;
}
*/

int 	CHttpProcesser::read_data(apr_socket_t* s ,CLIENT_INFO* ci)
{
	int recv_len;
	char * recv_buffer = recv_data(s,&recv_len);
	if(recv_buffer == NULL){
		LOG_DEBUG("CHttpProcesser::read_data recv_buffer == NULL");
		close_connection(s,ci);
		return -1;
	}

	int rv = has_chunk(recv_buffer);
	if(rv < 0){
		LOG_DEBUG("CHttpProcesser::read_data has_chunk < 0");
		close_connection(s,ci);
		return -2;
	}else if(rv > 0){
		if(read_chunk(s,recv_buffer,recv_len,&ci->res_data,&ci->res_len) < 0){
			LOG_DEBUG("CHttpProcesser::read_data read_chunk < 0");
			close_connection(s,ci);
			return -3;
		}
	}else{
		ci->res_data = recv_buffer;
		ci->res_len  = recv_len   ;
	}

	return 0;
}

int 	CHttpProcesser::has_chunk(const char *data)
{
	const char * header_end = strstr(data,"\r\n\r\n");
	if(header_end == NULL)
		return -1;
	const char * chunk = strstr(data,"chunked");
	if(chunk == NULL || chunk > header_end)
		return 0;
	return 1;
}


int 	CHttpProcesser::parse_chunk(char *chunk,int chunk_len,int last_chunk_len,char *save,int *save_pos)
{

	int chunk_data_len = 0;
	char *endptr ;
	char *chunk_data_begin;
	int remind_len = 0;
	for(;;){
		//printf("data_addr:%p,current_len:%d,chunk_len:%d,last_chunk_len:%d\n",*save,*save_pos,chunk_len,last_chunk_len);
		if(last_chunk_len < 0){//有剩余的忽略符号
			chunk += abs(chunk_len);
			chunk_len -= abs(chunk_len);
			last_chunk_len = 0;
		}if(last_chunk_len == 0){//传进来的chunk有chunk header
			chunk_data_len = strtol(chunk,&endptr,16);
			if(chunk_data_len == 0){
				//0\r\n\r\n
				return 0;
			}
			chunk_data_begin = endptr + 2 /*忽略\r\n*/;
			remind_len  =  chunk_len - (chunk_data_begin - chunk);
			//printf("data_len:%d,all_len:%d,remind_len:%d\n",chunk_data_len,chunk_len,remind_len);

			if(remind_len - chunk_data_len > 0){
				//copy一个完整的块再解析下一个
				memcpy(save+*save_pos,chunk_data_begin,chunk_data_len);
				*save_pos += chunk_data_len;
				chunk =  chunk_data_begin + chunk_data_len + 2 /*忽略\r\n*/;
				chunk_len  = remind_len - chunk_data_len - 2 /*忽略\r\n*/;
				continue;
			}else{//如果是最后一个,则解析后break
				if(remind_len == 0)
					return -2;
				memcpy(save+*save_pos,chunk_data_begin,remind_len);
				*save_pos += remind_len;
				//刚好一个完整的chunk data
				if(remind_len == chunk_data_len)
					return -2;
				else if(chunk_len == last_chunk_len + 2)
					return 0;
				else{
					//还有多少长度的chunk data没有接受
					return chunk_data_len - remind_len;
				}
			}
		}else{
		//传递进来的chunk开头没有chunk header
		//需要再接受last_chunk_len个长度然后才解析chunk header;
			if(chunk_len > last_chunk_len){
				memcpy(save+*save_pos,chunk,last_chunk_len);
				*save_pos += last_chunk_len;
				chunk_len -= last_chunk_len;
				chunk     += last_chunk_len;
				if(chunk_len <= 2){
					return chunk_len - 2;
				}
				last_chunk_len = 0;
			}else{
				//如果当前传递进来的chunk数据长度不足以补足当前chunk 块的长度
				memcpy(save+*save_pos,chunk,chunk_len);
				*save_pos += chunk_len;
				last_chunk_len -= chunk_len;
				return last_chunk_len;
			}
			
		}
	}
}
int 	CHttpProcesser::read_chunk(apr_socket_t* s,char *fsrc,int flen,char **data,int *tlen)
{
	char *header_end  = strstr(fsrc,"\r\n\r\n");
	//如果header头结束后没有跟chunk header则返回出错
	if(header_end - fsrc + 4 + 4+ 2 > flen){
		return -1;
	}
		
	
	//先copy http头
	int data_total_len = flen * 2;
	int data_current_len = header_end - fsrc + 4; 
	*data = (char*)apr_pcalloc(sub_pool,data_total_len);
	memcpy(*data,fsrc,data_current_len);

	//chunk头开始位置
	char *chunk_header_begin = header_end+4;
	int  chunk_len = flen + (fsrc - chunk_header_begin);
	int last_chunk_len = parse_chunk(chunk_header_begin,chunk_len,0,*data,&data_current_len);

	//判断最后一个buffer的结尾是否是chunk结尾
	int recv_len = flen;
	while(strncmp(fsrc+recv_len-5,"0\r\n\r\n",5) != 0){
		//判断逻辑,遍历chunk，直到剩余长度不足一个chunk的时候才去recv
		
		char *recv_buffer = recv_data(s,&recv_len);
		if(recv_buffer == NULL)
			return -2;

		if(recv_len + data_current_len > data_total_len - 1){
				//按当前总量的1.5倍进行增长
				data_total_len = static_cast<int> ((recv_len + data_current_len) * 1.5);
				char *temp_buf = (char*)apr_pcalloc(sub_pool,data_total_len);
				memcpy(temp_buf,*data,data_current_len);
				*data = temp_buf;
		}

		last_chunk_len = parse_chunk(recv_buffer,recv_len,last_chunk_len,*data,&data_current_len);
		fsrc = recv_buffer;
	}
	*tlen = data_current_len;
	return 0;
}

int 	CHttpProcesser::use_feh_read_verify_code(const char *img,int len,char *result)
{

	char img_file_path[] = "./login_verify.jpg";
	FILE *fp = fopen(img_file_path,"w");
	if(fp == NULL){
		return -1;
	}

	if(len != fwrite(img,1,len,fp)){
		return -2;
	}
	fclose(fp);

	char cmd[255]="";
	strcat(cmd,"bash -c feh ");
	strcat(cmd,img_file_path);
	system(cmd);
	
	puts("INPUT IMAGE STRNG:");
	fgets(result,255,stdin);
	//去掉不要的\n
	*(result+strlen(result)-1) = '\0';
	return 0;
}

void 	CHttpProcesser::set_visit_cookie(void * rh/*CRequestHeader* */)
{

	std::ostringstream oss;	

	time_t t  = time(NULL);
	CRequestHeader *local_rh_ptr  = (CRequestHeader *)rh;
	
#ifndef WIN32
	char time_buf[100];
	struct tm stm ;
	localtime_r(&t,&stm);
	asctime_r(&stm,time_buf);
	//去掉不要的\n
	time_buf[strlen(time_buf)-1] = '\0';
	//encode time
	char *encode_time;
	url_encode(&encode_time,time_buf,strlen(time_buf));
	oss.str("");
	oss << "lastVisitTime=" << encode_time << ";";
	local_rh_ptr->SetHeaderField("Cookie",oss.str(),1);
	free(encode_time);

	struct timeval tv;
	gettimeofday(&tv,NULL);
	oss.str("");
	oss << "visitor_user_id=" << tv.tv_sec * 1000 + tv.tv_usec << ";";
	local_rh_ptr->SetHeaderField("Cookie",oss.str(),1);
#else
	//NOTE:asctime not thread safe
	time_t t  = time(NULL);
	oss.str("");
	oss << "visitor_user_id=" << t*1000 << ";";
	local_rh_ptr->SetHeaderField("Cookie",oss.str(),1);

	struct tm *stm = localtime(&t);
	char *asc_time = asctime(stm);
	char *encode_time;
	url_encode(&encode_time,asc_time,strlen(asc_time));
	oss << "lastVisitTime=" << encode_time << ";";
	local_rh_ptr->SetHeaderField("Cookie",oss.str(),1);
	free(encode_time);
#endif 	
}

int	CHttpProcesser::RequestIndex(apr_socket_t* s,CLIENT_INFO* ci)
{

	if(ci->is_lock == 1 || ci->current_request_type != INDEX-1)
		return -1;

	CRequestHeader rheader;
	rheader.SetRequestLine("GET / HTTP/1.1");
	rheader.SetHeaderField("Host","guahao.zjol.com.cn");
	rheader.SetHeaderField("Accept-Encoding","gzip, deflate");
	rheader.SetHeaderField("Connection","Keep-Alive");
	rheader.SetHeaderField("User-Agent",IE_USER_AGENT);
	rheader.SetHeaderField("Accept","text/html, application/xhtml+xml, */*");
	rheader.SetHeaderField("Accept-Language","zh-CN");

	std::string req_header = rheader.GetRequest();

	if(req_header.empty()){
		//LOG here
		//let next time try agian
		LOG_DEBUG("RequestIndex request header empty");
		return -1;
	}

	LOG_DEBUG("开始请求浙江在线挂号首页,http头:\n%s",req_header.c_str());
	
	int req_header_len = req_header.size();
	bool   finish   = false;
	
	finish = send_data(s,req_header.c_str(),req_header_len);

	if(finish){
		ci->is_lock = 1;
		ci->current_request_type = INDEX;
	}else{
		close_connection(s,ci);
		return -2;
	}

}

int	CHttpProcesser::RequestVerify(apr_socket_t* s,CLIENT_INFO* ci)
{
	if(ci->is_lock == 0 && ci->current_request_type == INDEX){
		CRequestHeader rh;
		std::ostringstream oss;
		srandom(time(NULL));
		oss << "GET /VerifyCodeCH.aspx?0." << random() << " HTTP/1.1"; 
		rh.SetRequestLine(oss.str());

		rh.SetHeaderField("Accept","image/png, image/svg+xml, image/*;q=0.8, */*;q=0.5");
		rh.SetHeaderField("Referer","http://guahao.zjol.com.cn/");
		rh.SetHeaderField("Accept-Language","zh-CN");
		rh.SetHeaderField("Accept-Encoding","gzip, deflate");
		rh.SetHeaderField("Host","guahao.zjol.com.cn");
		rh.SetHeaderField("Connection","Keep-Alive");		
		rh.SetHeaderField("Cache-Control","no-cache");
		rh.SetHeaderField("User-Agent",IE_USER_AGENT);
		rh.SetHeaderField("Cookie",session_cookies);

		std::string req_header = rh.GetRequest();
		if(req_header.empty()){
			//LOG here
			LOG_DEBUG("RequestVerify request header is empty");
			close_connection(s,ci);
			return -1;
		}
		
		LOG_DEBUG("request login verify image http header:\n%s",req_header.c_str());

		int req_header_len = req_header.size();
		bool finish = send_data(s,req_header.c_str(),req_header_len);

		if(finish){
			ci->is_lock = 1;
			ci->current_request_type = 	VERIA;
			ci->last_active_time = time(NULL);
		}else{
			close_connection(s,ci);
			return -2;
		}	

		return 0;
	}else if(ci->is_lock == 0 && ci->current_request_type == JS){
		CRequestHeader rh;
		std::ostringstream oss;
		srandom(time(NULL));
		oss << "GET /VerifyCodeCH_SG.aspx?0." << random() << " HTTP/1.1"; 
		rh.SetRequestLine(oss.str());

		//如果refer空则要去查
		oss.str();
		oss << refer_uri;
		rh.SetHeaderField("Referer",oss.str());
		
		rh.SetHeaderField("Accept","image/png, image/svg+xml, image/*;q=0.8, */*;q=0.5");
		rh.SetHeaderField("Accept-Language","zh-CN");
		rh.SetHeaderField("Accept-Encoding","gzip, deflate");
		rh.SetHeaderField("Host","guahao.zjol.com.cn");
		rh.SetHeaderField("Connection","Keep-Alive");		
		rh.SetHeaderField("Cache-Control","no-cache");
		rh.SetHeaderField("User-Agent",IE_USER_AGENT);
		rh.SetHeaderField("Cookie",session_cookies);

		set_visit_cookie(&rh);

		std::string req_header = rh.GetRequest();
		if(req_header.empty()){
			//LOG here
			LOG_DEBUG("request header is empty");
			close_connection(s,ci);
			return -1;
		}

		LOG_DEBUG("request verify image http header:\n%s",req_header.c_str());

		int req_header_len = req_header.size();
		bool finish = send_data(s,req_header.c_str(),req_header_len);

		if(finish){
			ci->is_lock = 1;
			ci->current_request_type = 	VERIB;
			ci->last_active_time = time(NULL);
		}else{
			close_connection(s,ci);
			return -2;
		}	
		return 0;
	}

	return -1;
}

int	CHttpProcesser::RequestLogin(apr_socket_t* s,CLIENT_INFO* ci)
{
	//确保是前一个是在完成INDEX请求或者验证码请求
	if(ci->is_lock == 1 || (ci->current_request_type != INDEX 
		&& ci->current_request_type != VERIA))
		return -1;
	
	//先请求验证码,如果已经请求过则切换标识,跳过请求验证码
	if(ci->current_request_type == INDEX){
		if(strlen(verify_login_image) == 0)			
			return RequestVerify(s,ci);
		else{
			ci->current_request_type = VERIA;
			ci->last_active_time = time(NULL);
			return 0;
		}
	}else if(ci->current_request_type == VERIA){
	//在请求登陆
		if(strlen(verify_login_image) == 0 && use_feh_read_verify_code(ci->res_data,ci->res_len,verify_login_image) < 0){
			close_connection(s,ci);
			return -1;
		}

		LOG_DEBUG("RequestLogin login verify image:%s",verify_login_image);
		//组织http请求头并请求登陆
		CRequestHeader rh;
		std::ostringstream oss;
		
		oss << "GET /ashx/LoginDefault.ashx?idcode=" 
			<< user << "&pwd=" << password << "&txtVerifyCode=" 
		 	<< verify_login_image << " HTTP/1.1";
		rh.SetRequestLine(oss.str());

		rh.SetHeaderField("Accept","*/*");
		rh.SetHeaderField("Accept-Language","zh-cn");
		rh.SetHeaderField("Referer","http://guahao.zjol.com.cn/#");
		rh.SetHeaderField("User-Agent",IE_USER_AGENT);
		rh.SetHeaderField("Accept-Encoding","gzip, deflate");
		rh.SetHeaderField("Host","guahao.zjol.com.cn");
		rh.SetHeaderField("Connection","Keep-Alive");
		rh.SetHeaderField("Cache-Control","no-cache");
		rh.SetHeaderField("Cookie",session_cookies);

		//在cookie中设置visit time相关的cookie
		set_visit_cookie(&rh);

		std::string req_header = rh.GetRequest();
		if(req_header.empty()){
			//LOG here
			LOG_DEBUG("RequestLogin request header is empty"); 
			close_connection(s,ci);
			return -2;
		}

		LOG_DEBUG("请求登陆的http请求头:\n%s",req_header.c_str());
		int req_header_len = req_header.size();
		bool finish = send_data(s,req_header.c_str(),req_header_len);

		if(finish){
			ci->is_lock = 1;
			ci->current_request_type = 	LOGIN;
			ci->last_active_time = time(NULL);
		}else{
			close_connection(s,ci);
			return -3;
		}
	}

	return 0;
}

int	CHttpProcesser::RequestHospital(apr_socket_t* s,CLIENT_INFO* ci)
{
	if(ci->is_lock == 1 || ci->current_request_type != LOGIN || user_date_hospital.empty())
		return -1;

	CRequestHeader rh;
	std::ostringstream oss;
	HOSMAP::iterator iter = hospital_map.find(user_date_hospital);
	if(iter == hospital_map.end()){
		//等待外部设置预约的医院在hospital_map中
		return -2;
	}

	oss << "GET /" << iter->second.uri << " HTTP/1.1";
	rh.SetRequestLine(oss.str());
	rh.SetHeaderField("Accept","text/html, application/xhtml+xml, */*");
	rh.SetHeaderField("Referer","http://guahao.zjol.com.cn/");
	rh.SetHeaderField("Accept-Language","zh-CN");
	rh.SetHeaderField("Host","guahao.zjol.com.cn");
	rh.SetHeaderField("Connection","Keep-Alive");
	rh.SetHeaderField("Cookie",session_cookies);

	//在cookie中设置vist time相关的cookie
	set_visit_cookie(&rh);

	std::string req_header = rh.GetRequest();
	
	if(req_header.empty()){
		//LOG here
		LOG_DEBUG("RequestHospital request header is empty");
		close_connection(s,ci);
		return -3;
	}

	LOG_DEBUG("请求医院的http请求头:\n%s",req_header.c_str());
	int req_header_len = req_header.size();
	bool finish = send_data(s,req_header.c_str(),req_header_len);

	if(finish){
		ci->is_lock = 1;
		ci->current_request_type = 	HOS;
		ci->last_active_time = time(NULL);
	}else{
		close_connection(s,ci);
		return -4;
	}
	return 0;
}

int 	CHttpProcesser::RequestDepartment(apr_socket_t* s,CLIENT_INFO* ci)
{

	if(ci->is_lock == 1 || ci->current_request_type != HOS || user_date_department.empty())
		return -1;

	CRequestHeader rh;
	std::ostringstream oss;
	DEPMAP::iterator iter = department_map.find(user_date_department);
	if(iter == department_map.end()){
		//等待外部设置预约的医院在hospital_map中
		return -1;
	}

	HOSMAP::iterator ref_iter = hospital_map.find(user_date_hospital);
	if(ref_iter == hospital_map.end()){
		return -2;
	}

	oss << "GET /" << iter->second.uri << " HTTP/1.1";
	rh.SetRequestLine(oss.str());
	rh.SetHeaderField("Accept","text/html, application/xhtml+xml, */*");

	oss.str("");
	oss << "http://guahao.zjol.com.cn/" << ref_iter->second.uri;
	rh.SetHeaderField("Referer",oss.str());

	rh.SetHeaderField("Accept-Language","zh-CN");
	rh.SetHeaderField("Host","guahao.zjol.com.cn");
	rh.SetHeaderField("User-Agent",IE_USER_AGENT);
	rh.SetHeaderField("Connection","Keep-Alive");
	rh.SetHeaderField("Cookie",session_cookies);

	//在cookie中设置vist time相关的cookie
	set_visit_cookie(&rh);

	std::string req_header = rh.GetRequest();
	
	if(req_header.empty()){
		//LOG here
		LOG_DEBUG("RequestDepartment request header is empty");
		close_connection(s,ci);
		return -3;
	}

	LOG_DEBUG("RequestDepartment request http header:\n%s",req_header.c_str());
	int req_header_len = req_header.size();
	bool finish = send_data(s,req_header.c_str(),req_header_len);

	if(finish){
		ci->is_lock = 1;
		ci->current_request_type = 	DEPART;
		ci->last_active_time = time(NULL);
	}else{
		close_connection(s,ci);
		return -4;
	}

	return 0;
}

int 	CHttpProcesser::RequestDoctor(apr_socket_t* s,CLIENT_INFO* ci)
{
	if(ci->is_lock == 1 || ci->current_request_type != DEPART)
		return -1;

	if(detail_doc_map.empty() || department_map.empty()){
		//需要回滚请求
		LOG_DEBUG("RequestDoctor but doc map and depart map empty");
		return -1;
	}

	CRequestHeader rh;
	std::ostringstream oss;

	rh.SetRequestLine("POST /ashx/gethy.ashx HTTP/1.1");
	rh.SetHeaderField("x-requested-with","XMLHttpRequest");
	rh.SetHeaderField("Accept-Language","zh-cn");
	if(refer_uri.empty()){
		DEPMAP::iterator iter = department_map.find(user_date_department);
		if(iter == department_map.end()){
			//找不到referer
			return -2;
		}
		oss << "http://guahao.zjol.com.cn/" << iter->second.uri;
	}else{
		oss << refer_uri;
	}
	rh.SetHeaderField("Referer",oss.str());

	rh.SetHeaderField("Content-Type","application/x-www-form-urlencoded");
	rh.SetHeaderField("Content-Type","application/x-www-form-urlencoded");
	rh.SetHeaderField("Accept-Encoding","gzip, deflate");
	rh.SetHeaderField("User-Agent",IE_USER_AGENT);
	rh.SetHeaderField("Host","guahao.zjol.com.cn");
	rh.SetHeaderField("Connection","Keep-Alive");
	rh.SetHeaderField("Cache-Control","no-cache");
	rh.SetHeaderField("Cookie",session_cookies);
	set_visit_cookie(&rh);

	DETMAP::iterator doc_iter = detail_doc_map.find(user_date_doctor);
	if(doc_iter == detail_doc_map.end()){
		//暂时抽取第一个医生
		doc_iter = detail_doc_map.begin();
	}
	
	oss.str("");
	oss << strlen("sg=")+doc_iter->second.at(0).content.size();
	rh.SetHeaderField("Content-Length",oss.str());

	oss.str("");
	oss << "sg=" << doc_iter->second.at(0).content;
	rh.SetContent(oss.str());

	std::string req_header = rh.GetRequest();
	if(req_header.empty()){
		LOG_DEBUG("RequestDoctor http header is empty,close connection");
		close_connection(s,ci);
		return -3;
	}	

	LOG_DEBUG("RequestDoctor request http header:\n%s",req_header.c_str());
	int req_header_len = req_header.size();
	bool finish = send_data(s,req_header.c_str(),req_header_len);

	if(finish){
		ci->is_lock = 1;
		ci->current_request_type = 	DOCTOR;
		ci->last_active_time = time(NULL);
	}else{
		close_connection(s,ci);
		return -4;
	}

	return 0;
}

int 	CHttpProcesser::RequestDialogbJS(apr_socket_t* s,CLIENT_INFO* ci)
{

	if(ci->is_lock == 1 || ci->current_request_type != DOCTOR)
		return -1;

	CRequestHeader rh;
	std::ostringstream oss;

	rh.SetRequestLine("GET /js/dialogB.js HTTP/1.1");
	rh.SetHeaderField("Accept","application/javascript, */*;q=0.8");

	if(refer_uri.empty()){
		DEPMAP::iterator iter = department_map.find(user_date_department);
		if(iter == department_map.end()){
			//找不到referer
			return -2;
		}
		oss << "http://guahao.zjol.com.cn/" << iter->second.uri;
	}else{
		oss << refer_uri;
	}
	rh.SetHeaderField("Referer",oss.str());

	rh.SetHeaderField("Accept-Language","zh-CN");
	rh.SetHeaderField("User-Agent",IE_USER_AGENT);
	rh.SetHeaderField("Host","guahao.zjol.com.cn");
	rh.SetHeaderField("Connection","Keep-Alive");
	rh.SetHeaderField("Cookie",session_cookies);
	set_visit_cookie(&rh);

	std::string req_header = rh.GetRequest();
	if(req_header.empty()){
		LOG_DEBUG("RequestDialogbJS request header is empty,close connection");
		close_connection(s,ci);
		return -3;
	}	

	LOG_DEBUG("RequestDialogbJS request  header:\n%s" ,req_header.c_str());
	int req_header_len = req_header.size();
	bool finish = send_data(s,req_header.c_str(),req_header_len);

	if(finish){
		ci->is_lock = 1;
		ci->current_request_type = 	JS;
		ci->last_active_time = time(NULL);
	}else{
		close_connection(s,ci);
		return -4;
	}

}

int	CHttpProcesser::RequestConfirmDoctor(apr_socket_t *s,CLIENT_INFO* ci)
{

	if(ci->is_lock == 1 || (ci->current_request_type 
		!= JS && ci->current_request_type != VERIB))
		return -1;

	if(ci->current_request_type == JS){
		return RequestVerify(s,ci);
	}else{
		CRequestHeader rh;
		std::ostringstream oss;
		
		DEPMAP::iterator iter = department_map.find(user_date_department);
		if(iter == department_map.end()){
			//找不到referer
			return -1;
		}

		rh.SetRequestLine("POST /ashx/TreadYuyue.ashx HTTP/1.1");
		rh.SetHeaderField("x-requested-with","XMLHttpRequest");


		oss << "http://guahao.zjol.com.cn/" << iter->second.uri;
		rh.SetHeaderField("Referer",oss.str());

		rh.SetHeaderField("Accept","*/*");
		rh.SetHeaderField("Content-Type","application/x-www-form-urlencoded");
		rh.SetHeaderField("Accept-Encoding","gzip, deflate");
		rh.SetHeaderField("User-Agent",IE_USER_AGENT);
		rh.SetHeaderField("Host","guahao.zjol.com.cn");
		rh.SetHeaderField("Connection","Keep-Alive");
		rh.SetHeaderField("Cache-Control","no-cache");
		rh.SetHeaderField("Cookie",session_cookies);
		set_visit_cookie(&rh);

		//在这里组织请求正文
		RES_DOC* doc = (RES_DOC*)ci->data;
		oss.str("");
		oss << doc->js << "=";
		std::vector<std::string> items; 
		std::vector<std::string> item;
		split(doc->hy,items,"$");
		split(items[0].c_str(),item,"|");
		oss << item[0] << "&";

		oss << "yzm=" << doc->yzm << "&"
			<< "xh="   << item[1] << "&"
			<< "qhsj=" << item[2] << "&"
			<< "sg="   << doc->mgenc;
		rh.SetContent(oss.str());
		int len  = oss.str().size();
		oss.str("");
		oss << len;
		rh.SetHeaderField("Content-Length",oss.str());

		std::string req_header = rh.GetRequest();
		if(req_header.empty()){
			LOG_DEBUG("RequestConfirmDoctor request CONFIRM http header is empty,close connection");
			close_connection(s,ci);
			return -2;
		}	

		LOG_DEBUG("request CONFIRM http header:\n%s",req_header.c_str());
		int req_header_len = req_header.size();
		bool finish = send_data(s,req_header.c_str(),req_header_len);

		if(finish){
			ci->is_lock = 1;
			ci->current_request_type = 	CONFIRM;
			ci->last_active_time = time(NULL);
		}else{
			close_connection(s,ci);
			return -3;
		}
	}

	/*
	//set content
	//selval&verifycode&order_number&watch_time&sg
	f9s0jae=19271449&yzm=12&xh=12&qhsj=0910&sg=1ADA82D053F629079A39B3B9033F5A1BF2\
	D7033D2B47999039D87AB8AFE7FD446490FF2C6F51C1D3385EC355B2B5EFDEE71D7F05C8A43550\
	A589FAA7EE9ABAAD17973A39BE8B2609C7D83BE175C92A3F5F31AF02938F3872

	//响应切割后要字段数>10
	//unkown_number|hospital|department|doctor_name|user_id|id_card(user)|user_name|unkown_number|date|unkown_number|cost|date_password|order_number|time
	957102|浙二医院|内分泌科|王永健|1069018|350521197404071798|高乐音|1605581|20121116|0|5|43682460|42680825|12|0910
	*/
	return 0;
}

int 	CHttpProcesser::ProcessIndexResult(apr_socket_t* s,CLIENT_INFO* ci)
{
	if(ci->current_request_type != INDEX)
		return -1;
	//chunk有可能会分为多次发送,
	//但这里处理初步接受到一个一个带chunk的http头就开始持续接受指导完成
	if(read_data(s,ci) < 0){
		//TODO:一会考虑没接受完的处理
		return -1;
	}

	/*
	unsigned char md5_str[17]="";
	md5((unsigned char*)ci->res_data,ci->res_len,md5_str);
	std::cout << "recv len:" << ci->res_len << " md5:"  ;
	for(int i = 0; i < 16; ++i){
		printf("%02x",md5_str[i]);
	}
	printf("\n");
	*/

	CParseResponse parse_res (ci->res_data,ci->res_len);
	std::string content = parse_res.GetContent();
	if(content.empty()){
		//TODO:容错处理
		ci->is_lock = 0;
		ci->current_request_type = BEGIN;
		LOG_DEBUG("ProcessIndexResult response content is empty,set request type to BEGIN");
		return -2;
	}
	
	LOG_TO_FILE("index.html",content);
	LOG_DEBUG("ProcessIndexResult response header:\n%s",parse_res.GetHeader().c_str());
	session_cookies = parse_res.GetSetCookies();

	CParseHtml phtml;
	hospital_map = phtml.ParseIndexHtml((char*)content.c_str());
	if(hospital_map.empty()){
		//TODO:如果响应正文解析不到hospital map则close连接
		LOG_DEBUG("ProcessIndexResult response content can't find hospital list info,close connection");
		close_connection(s,ci);
		return -3;
	}
	
	ci->is_lock  = 0;
	ci->last_active_time = time(NULL);

	return 0;
}

int 	CHttpProcesser::ProcessVerifyResult(apr_socket_t* s,CLIENT_INFO* ci)
{
	//数据转存到ci->res_data中,更新最后活跃时间并解锁
	if(ci->current_request_type == VERIA){		
		if(read_data(s,ci) < 0){
			return -1;
		}

		CParseResponse parse_res(ci->res_data,ci->res_len);
		std::string res_content = parse_res.GetContent();
		if(res_content.empty()){
			ci->is_lock = 0;
			ci->current_request_type = INDEX;
			LOG_DEBUG("ProcessVerifyResult response login image is empty,set request type to INDEX");
			return -2;
		}
		
		LOG_DEBUG("ProcessVerifyResult VERIA response");
		//std::cout << "verify response content:" << res_content << "\n";
		//取图片内容保持到ci->res_data中
		memset(ci->res_data,0,ci->res_len);
		memcpy(ci->res_data,res_content.c_str(),res_content.size());
		ci->res_len = res_content.size();

		ci->is_lock = 0;
		ci->last_active_time = time(NULL);
	}else if(ci->current_request_type == VERIB){
		if(read_data(s,ci) < 0){
			return -1;
		}

		CParseResponse parse_res(ci->res_data,ci->res_len);
		std::string res_content = parse_res.GetContent();
		if(res_content.empty()){
			ci->is_lock = 0;
			ci->current_request_type = JS;
			LOG_DEBUG("ProcessVerifyResult response login image is empty,set request type to JS");
			return -2;
		}

		LOG_DEBUG("ProcessVerifyResult VERIB response");

		//std::cout << "verify response content:" << res_content << "\n";
		//取图片内容保持到ci->res_data中
		memset(ci->res_data,0,ci->res_len);
		memcpy(ci->res_data,res_content.c_str(),res_content.size());
		ci->res_len = res_content.size();

		ci->is_lock = 0;
		ci->last_active_time = time(NULL);
		
		RES_DOC* doc = (RES_DOC*)ci->data;
		memset(doc->yzm,0,MAX_ITEM_LEN);
		use_feh_read_verify_code(res_content.c_str(),res_content.size(),doc->yzm);
	}else{
		//unkown type
		return -5;
	}

	return 0;
}

int	CHttpProcesser::ProcessLoginResult(apr_socket_t* s,CLIENT_INFO* ci)
{
	if(ci->current_request_type == LOGIN){
		if(read_data(s,ci) < 0){
			return -1;
		}

		CParseResponse parse_res(ci->res_data,ci->res_len);
		std::string res_content = parse_res.GetContent();
		if(res_content.empty()){
			ci->is_lock = 0;
			ci->current_request_type = VERIA;
			LOG_DEBUG("ProcessLoginResult response content is empty,set request type to VERIA");
			return -2;
		}

		if(strncmp(res_content.c_str(),"OK",2) != 0 || std::string::npos != res_content.find("验证码已过期")){
			//登陆失败,重新请求验证码
			ci->is_lock = 0;
			ci->current_request_type = INDEX;
			LOG_DEBUG("ProcessLoginResult login failed,set request type to VERIA");
			memset(verify_login_image,0,255);
			return -3;
		}

		LOG_DEBUG("请求登陆的响应正文:%s\n",res_content.c_str());
		memset(ci->res_data,0,ci->res_len);
		memcpy(ci->res_data,res_content.c_str(),res_content.size());
		
		ci->res_len = res_content.size();	
		ci->is_lock = 0;
		ci->last_active_time = time(NULL);

		//TODO:解析响应成功的正文
		std::vector<std::string> items;
		split(res_content.c_str(),items,"|");
		if(items.size() >= 2){
			user_id = items.at(1) ;
		} 

	}else{
		//unkown
		return -5;
	}

	return 0;
}

int 	CHttpProcesser::ProcessHospitalResult(apr_socket_t* s,CLIENT_INFO* ci)
{
	if(ci->current_request_type == HOS){
		if(read_data(s,ci) < 0){
			return -1;
		}

		CParseResponse parse_res(ci->res_data,ci->res_len);
		std::string res_content = parse_res.GetContent();
		if(res_content.empty()){
			ci->is_lock = 0;
			ci->current_request_type = LOGIN;
			LOG_DEBUG("ProcessHospitalResult response content is empty,set request type to LOGIN");
			return -2;
		}

		LOG_DEBUG("请求指定医院的科室列表的响应头:\n%s",parse_res.GetHeader().c_str());
		memset(ci->res_data,0,ci->res_len);
		memcpy(ci->res_data,res_content.c_str(),res_content.size());
		ci->res_len = res_content.size();	
		LOG_TO_FILE("hospital.html",res_content);
		CParseHtml phtml;
		department_map = phtml.ParseHospitalHtml(ci->res_data);
		if(department_map.empty()){
			ci->is_lock = 0;
			ci->current_request_type = LOGIN;
			LOG_DEBUG("ProcessHospitalResult find hospital's department error,set request type to LOGIN\n");
			return -3;
		}

		//优先组织出refer_uri
		if(!user_date_department.empty()){
			DEPMAP::iterator iter = department_map.find(user_date_department);
			if(iter != department_map.end())
				refer_uri = "http://guahao.zjol.com.cn/" + iter->second.uri;
		}

		ci->is_lock = 0;
		ci->last_active_time = time(NULL);
	}else{
		return -5;
	}

	return 0;
}

int	CHttpProcesser::ProcessDepartmentResult(apr_socket_t *s,CLIENT_INFO* ci)
{
	if(ci->current_request_type == DEPART){
		if(read_data(s,ci) < 0){
			return -1;
		}

		CParseResponse parse_res(ci->res_data,ci->res_len);
		std::string res_content = parse_res.GetContent();
		if(res_content.empty()){
			ci->is_lock = 0;
			ci->current_request_type = HOS;
			LOG_DEBUG("ProcessDepartmentResult response content is empty,set request type to HOS");
			return -2;
		}

		LOG_DEBUG("请求指定科室的医生列表的响应头:\n%s",parse_res.GetHeader().c_str());

		memset(ci->res_data,0,ci->res_len);
		memcpy(ci->res_data,res_content.c_str(),res_content.size());
		ci->res_len = res_content.size();	
		LOG_TO_FILE("department.html",res_content);
		CParseHtml phtml;
		detail_doc_map = phtml.ParseDeparmentHtml(ci->res_data);
		if(department_map.empty()){
			LOG_DEBUG("ProcessDepartmentResult find department's doctor list error");
			ci->is_lock = 0;
			ci->current_request_type = DEPART;
			return -3;
		}

		ci->is_lock = 0;
		ci->last_active_time = time(NULL);
	}else{
		return -5;
	}

	return 0;
}

int	CHttpProcesser::ProcessDoctorResult(apr_socket_t *s,CLIENT_INFO* ci)
{
	if(ci->current_request_type == DOCTOR){
		if(read_data(s,ci) < 0){
			return -1;
		}

		CParseResponse parse_res(ci->res_data,ci->res_len);
		std::string res_content = parse_res.GetContent();
		if(res_content.empty()){
			ci->is_lock = 0;
			ci->current_request_type = DEPART;
			LOG_DEBUG("ProcessDoctorResult response content is empty,set request type to DEPART");
			return -2;
		}


		LOG_DEBUG("请求指定预约医生的响应头:\n%s",parse_res.GetHeader().c_str());
		LOG_DEBUG("请求指定预约医生的响应正文:\n%s",parse_res.GetContent().c_str());

		LOG_TO_FILE("the_doc_info.html",res_content);
		//not login
		if(res_content ==  "-2"){
			ci->is_lock = 0;
			ci->current_request_type = VERIA;
			LOG_DEBUG("ProcessDoctorResult user not login,go to login!");
			return -3;		
		}

		memset(ci->res_data,0,ci->res_len);
		memcpy(ci->res_data,res_content.c_str(),res_content.size());
		ci->res_len = res_content.size();	

		//解析响应进行组织为确认预约请求组织请求正文
		std::vector<std::string> items;
		split(ci->res_data,items,"#");
		if(items.size() != 13){
			close_connection(s,ci);
			LOG_DEBUG("ProcessDoctorResult item size not avail ,close connection and res_content:%s",ci->res_data);
			return -4;
		}

		RES_DOC *doc = (RES_DOC*)apr_pcalloc(sub_pool,sizeof(RES_DOC));
		strcpy(doc->sfz,items[5].c_str());//身份证 
        strcpy(doc->xm ,items[6].c_str());//姓名
        strcpy(doc->yymc,items[1].c_str());//医院
        strcpy(doc->ksmc,items[2].c_str());//科室
        strcpy(doc->ysmc,items[3].c_str());//医生
        strcpy(doc->ghfy,items[10].c_str());//挂号费
        strcpy(doc->jzrq,items[8].c_str());//就诊日期
        strcpy(doc->mgenc, items[12].c_str());//标示串
         
        strcpy(doc->hy,items[11].c_str());//号分布串
        ci->data = doc;

		ci->is_lock = 0;
		ci->last_active_time = time(NULL);
		//unkown_number#hospital#department#doctor_name#user_id#id_card(user)#user_name#unkown_number#date#unkown_number#cost#confirm_info#sg
		//confirm_info: selval|order_number|time
		/*
		957102#浙二医院#内分泌科#王永健#1069018#350521197404071798#高乐音#1605581#20121116#0#5#$19271449|12|0910$19271454|32|1050#1ADA\
		82D053F629079A39B3B9033F5A1BF2D7033D2B47999039D87AB8AFE7FD446490FF2C6F51C1D3385EC355B2B5EFDEE71D7F05C8A43550A589FAA7EE9ABAAD179\
		73A39BE8B2609C7D83BE175C92A3F5F31AF02938F3872
		*/
	}else{
		return -5;
	}
	return 0;
}

int 	CHttpProcesser::ProcessDialogbJSResult(apr_socket_t* s,CLIENT_INFO* ci)
{
	if(ci->current_request_type == JS){
		if(read_data(s,ci) < 0){
			return  -1;
		}

		CParseResponse parse_res(ci->res_data,ci->res_len);
		std::string res_content = parse_res.GetContent();
		if(res_content.empty()){
			ci->is_lock = 0;
			ci->current_request_type = DOCTOR;
			LOG_DEBUG("ProcessDialogbJSResult response content is empty,set request type to DOCTOR");
			return -2;
		}

		size_t b = 0;
		size_t e = 0;

		b = res_content.find("var");
		e = res_content.find("=",b+1);

		LOG_DEBUG("请求JS的响应头:\n%s",parse_res.GetHeader().c_str());
		LOG_TO_FILE("dialogb.js",res_content);
		if(b == std::string::npos || e == std::string::npos){
			LOG_DEBUG("ProcessDialogbJSResult can't find dialogB.js first item,close connectoin and res_content:%s",res_content.c_str());
			close_connection(s,ci);
			return -3;
		}

		b = b + strlen("var");
		e = e - strlen("=");
		std::string js_var = res_content.substr(b,e-b);
		js_var.erase(std::remove(js_var.begin(),js_var.end(),' '),js_var.end());
		memcpy(((RES_DOC*)ci->data)->js,js_var.c_str(),js_var.size());
		((RES_DOC*)ci->data)->js[js_var.size()] = '\0';

		LOG_DEBUG("ProcessDialogbJSResult 获取js var结果:%s",js_var.c_str());
		ci->is_lock = 0;
		ci->last_active_time = time(NULL);
	}else{
		return -5;
	}

	return 0;
}

int	CHttpProcesser::ProcessConfirmResult(apr_socket_t *s,CLIENT_INFO* ci)
{
	if(ci->current_request_type == VERIB){
		return ProcessVerifyResult(s,ci);
	}else if(ci->current_request_type == CONFIRM){
		if(read_data(s,ci) < 0){
			return -1;
		}

		CParseResponse parse_res(ci->res_data,ci->res_len);
		std::string res_content = parse_res.GetContent();
		if(res_content.empty()){
			ci->is_lock = 0;
			ci->current_request_type = DOCTOR;
			LOG_DEBUG("ProcessConfirmResult response content is empty,set request type to DOCTOR");
			return -2;
		}
		//TODO:还要考虑号已经预约完了和当前预约失败的处理
		LOG_DEBUG("预约结果:%s",res_content.c_str());

		std::vector<std::string> items;
		split(res_content.c_str(),items,"|");
		std::ostringstream oss;
		if(items.size() == 15){
			oss << "已经成功挂到'"<< items.at(1) << " " << items.at(2) << " 医生:" << items.at(3) << "'"
			<< ",取号时间:" << items.at(8).substr(0,4)<< "-" << items.at(8).substr(4,2) << "-" 
			<< items.at(8).substr(6,2) << " " << items.at(14).substr(0,2) << ":" << items.at(14).substr(2)
			<< ",第" << items.at(13) << "号,取号密码:" << items.at(11) << "\n";
			LOG_TO_FILE("result.txt",oss.str());
			exit(0);
		}
		//957102|浙二医院|外科|徐斌|1069018|350521197404071798|高乐音|1616887|20121121|0|5|13842106|42922816|11|0940

	}else{
		return -5;
	}
	return 0;
}

void 	CHttpProcesser::Stop()
{
	apr_atomic_set32 (&exit_flag,1);
}

#elif defined(ASIO)
#else
#endif
