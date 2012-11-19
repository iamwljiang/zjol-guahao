#include "Strutil.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
void  split(const char * data,std::vector<std::string>& items,const std::string& sep)
{
	int size = strlen(data)+1;
	char *local_data = (char*)malloc(size);
	memcpy(local_data,data,size-1);
	*(local_data+size-1) = '\0';

	char * save_ptr = NULL;
	char * str      = NULL;
	char * token    = NULL;
	int i = 0;
	for(str=local_data; ;++i,str=NULL){
		token = STRTOK(str,sep.c_str(),&save_ptr);
		if(token == NULL)
			break;
		
		int len = strlen(token);
		char * ptr_token = token;
		int  not_blank_pos = 0;
		SKIP_HEAD_BLANK(ptr_token,not_blank_pos);
		if(len == not_blank_pos){
			//空行
			continue;
		}

		std::string value ;
		value.assign(token);	
		items.push_back(value);		
	}
}

char* get_current_time_line(char* buf,int buf_len)
{
	time_t t = time(NULL);
	
#ifndef WIN32
	struct tm stm;
	localtime_r(&t,&stm); 
	strftime(buf,buf_len,"%F %T",&stm);
#else
	struct tm *stm = localtime(&t);
	strftime(buf,buf_len,"%F %T",stm);
#endif 	
	return buf;	
}
int debug_log(const char* fmt,...)
{
	const int MAX_BUF_LEN = 8190;
	char  buf[MAX_BUF_LEN];
	va_list ap;
	va_start(ap, fmt);
	printf("%s [DEBUG] ",get_current_time_line(buf,MAX_BUF_LEN));
	memset(buf,0,MAX_BUF_LEN);
	vsnprintf(buf, MAX_BUF_LEN, fmt, ap);
	printf("%s\n",buf);
	va_end(ap);

	return 0;
}
