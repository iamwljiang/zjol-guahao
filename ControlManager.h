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

 #ifndef GUAHAO_CONTROLMANAGER_H_
 #define GUAHAO_CONTROLMANAGER_H_

#include <string>
#include <HttpProcesser.h>
class CControlManager
{
public:
	CControlManager();

	~CControlManager();

	//TODO:set signal function
	int 	Init(const char* address,int port);

	//only request once;interactive interface
	int 	GetHositalList(void *out);

	//when thread not start,this would request any time;interactive interface
	int 	GetDepartListOfHospital(const std::string& hos_name, void *out);

	//when thread not start,this would request any time;interactive interface
	int 	GetDoctorListOfDepart(const std::string&   depart_name,void *out);

	//set argument
	void 	SetWeekday(const std::string& day);
	
	void 	SetLogfile(const std::string& run_log,const std::string& result_log);

	//start threads
	int 	Run(const std::string& hos,const std::string& dep,const std::string &doc);

	//stop  threads and exit program
	int 	Stop();

private:
	//forbidden copy and assign
	CControlManager(const CControlManager& rhs);

	CControlManager& operator=(const CControlManager& rhs);

private:
	//private function 

	CHttpProcesser 		first_connection;
private:
	std::string 		server_address;

	int 				server_port;

	std::string 		hospital_name;

	std::string 		department_name;

	std::string	 		doctor_name;

	std::string 		run_log_file;

	std::string 		result_log_file;

	std::string 		day;

//初步使用boost thread启动线程,之后改成为多种模式
#ifdef USE_BOOST_THREAD
//	std::vector<boost::thread> thread_pool;
#endif

	static unsigned int	THREAD_NUMBER;//线程数
};

#endif //GUAHAO_CONTROLMANAGER_H_
