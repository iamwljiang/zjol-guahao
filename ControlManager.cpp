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

#include "ControlManager.h"
#include "Strutil.h"
unsigned int CControlManager::THREAD_NUMBER = 4;
extern PRINT_TYPE LOG_DEBUG ;

CControlManager::CControlManager()
{
	server_address = "";

	server_port = -1;

	hospital_name = "";

	department_name = "";

	doctor_name = "";

	run_log_file = "";

	result_log_file = "";

	day = "";
	apr_initialize();
}


CControlManager::~CControlManager()
{
	apr_terminate();
}

int CControlManager::Init(const char* address,int port)
{

	int r = 0;

	if((r = first_connection.Init(address,port)) < 0){
		LOG_DEBUG("CControlManager::Init init first connection error");
		return r;
	}

	return 0;	
}


int CControlManager::GetHositalList(void *out)
{
	int rv = 0;
	rv = first_connection.RunOnceGetHosmap(out);
	return rv == 0 ? 0 : rv;
}

int CControlManager::GetDepartListOfHospital(const std::string& hos_name, void *out)
{
	int rv = 0;
	rv = first_connection.RunOnceGetDepmap(hos_name,out);
	return rv == 0 ? 0 : rv;
}

int CControlManager::GetDoctorListOfDepart(const std::string&   depart_name,void *out)
{
	int rv = 0;
	rv = first_connection.RunOnceGetDocmap(depart_name,out);
	return rv == 0 ? 0 : rv;
}

void CControlManager::SetWeekday(const std::string& day)
{
	
}

void CControlManager::SetLogfile(const std::string& run_log,const std::string& result_log)
{
	
}

int CControlManager::Run(const std::string& hos,const std::string& dep,const std::string &doc)
{
	return 0;
}

int CControlManager::Stop()
{
	return 0;
}
