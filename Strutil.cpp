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
#include "Strutil.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <KmMutex.h>

static KmMutex log_mutex;
Logger run_logger;
Logger result_logger;

//PRINT_TYPE LOG_DEBUG =debug_log;
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
	
	free(local_data);
	local_data = NULL;
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

/*
static FILE* fp = NULL;
int open_file(const char* filename)
{
	if(fp == NULL){
		fp = fopen(filename,"w+");
		if(fp == NULL){
			return -1;
		}
	}
	return 0;
}

void close_file()
{
	if(fp != NULL){
		fclose(fp);
	}
}
*/
int debug_log(const char* fmt,...)
{
	const int MAX_BUF_LEN = 8190;
	char  buf[MAX_BUF_LEN];
	va_list ap;
	va_start(ap, fmt);
	{
		KmScopedLock lock(log_mutex);
		printf("%s [DEBUG] ",get_current_time_line(buf,MAX_BUF_LEN));
		//fprintf(fp,"%s [DEBUG] ",get_current_time_line(buf,MAX_BUF_LEN));
		memset(buf,0,MAX_BUF_LEN);
		vsnprintf(buf, MAX_BUF_LEN, fmt, ap);
		printf("%s\n",buf);
		//fprintf(fp,"%s\n",buf);
	}
	va_end(ap);

	return 0;
}


int   strwtoi(const char* day)
{
	int nweekday = -1;
	if(strcmp(day,"星期一") == 0){
		nweekday = 1;
	}else if(strcmp(day,"星期二") == 0){
		nweekday = 2;
	}else if(strcmp(day,"星期三") == 0){
		nweekday = 3;
	}else if(strcmp(day,"星期四") == 0){
		nweekday = 4;
	}else if(strcmp(day,"星期五") == 0){
		nweekday = 5;
	}else if(strcmp(day,"星期六") == 0){
		nweekday = 6;
	}else if(strcmp(day,"星期日") == 0){
		nweekday = 0;
	}

	return nweekday;
}
