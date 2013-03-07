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
 
#include "HttpProcesser.h"
#include "ControlManager.h"
#include "ssignal.h"
#include <iostream>
#include <sstream>
//#include "DataType.h"
#ifdef WIN32
#include "gnu_getopt.h"
#endif

#define V_0_1_1 "当前版本0.1.2 支持并发线程连接控制,支持数据压缩传输,不支持自动识别验证码,log记录"  

static std::string user;
static std::string passwd;
static std::string want_hospital;
static std::string want_department;
static std::string want_doctor;
static std::string want_week="-1";

static std::string thread_number;
static std::string connection_number;

static std::string log_file="guahao_info.txt";
static std::string result_file="result.txt";

extern boost::function0<void> console_ctrl_function;
void display(const std::string &rhs)
{
	std::cout << rhs << "\n";
}

void Usage(char **argv)
{
	std::ostringstream oss;
	oss << "usage:\n" << argv[0] << " -u user -p password "
		<< "[-a hospital [-b department [-c doctor] ] ] [-t weekday]\n"
		<< argv[0] << " -h|-?|-v\n"
		<< "-u user	        用户名,需要在guahao.zjol.com.cn中注册过\n"
		<< "-p passwd       用户密码\n"
		<< "-a hospital     指定需要挂号的医院,可选\n"
		<< "-b department   指定需要挂号的科室,不为空的时候,必须-a也不为空\n"
		<< "-c doctor       指定需要挂号的医生,不为空的时候,必须-b也不为空\n"
		<< "-m thread number     程序需要启动的线程数\n"
		<< "-n connection number 每个线程的连接数\n"
		<< "-t weekday      用户指定需要一周内的预约时间,,0-6代表礼拜天到礼拜礼拜六,默认从最近有号的一天\n"
		<< "-o logfile      用于指定运行log输出到目标文件,默认执行目录下的guahao_info.log\n"
		<< "-r result       用于指定挂号成功信息的目标文件,默认执行目录下的result.txt\n"
		<< "-v              显示版本号,并提示当前版本支持的功能\n"
		<< "-h,-?           显示帮助,有问题可联系iamwljiang@gmail\n";

	std::cout << oss.str();	
}


int Start()
{
	//check argument
	if(user.empty() || passwd.empty()){
		return -1;
	}

	if(want_hospital.empty() && (!want_department.empty() || !want_doctor.empty())){
		return -1;
	}

	if(want_department.empty() && !want_doctor.empty()){
		return -1;
	}

	CControlManager control_manager;
	console_ctrl_function = boost::bind(&CControlManager::Stop,&control_manager);
#ifdef WIN32
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
#else
    init_signal();
#endif
	//set argument
	control_manager.SetWeekday(want_week);
	control_manager.SetLogfile(log_file,result_file);
	control_manager.SetConcurrent(thread_number,connection_number);
	if(control_manager.Init("guahao.zjol.com.cn",80) < 0){
		return -3;
	}

	//interactive check

	int error_count = 0;
	//check hospital name 
	HOSMAP hos_map;
	HOSMAP::iterator hos_iter;
	if(control_manager.GetHositalList(&hos_map) < 0){
		return -4;
	}

	if(control_manager.TestLogin(user,passwd) != 0){
		return -5;
	}

	do{	
		if(want_hospital.empty()){
			
			hos_iter = hos_map.begin();
			for(; hos_iter != hos_map.end(); ++hos_iter){
				std::cout << "医院:" << hos_iter->first << "\n";
			}

			std::cout << "Input you want to select hospital name:\n";
			std::cin >> want_hospital;
		}

		hos_iter = hos_map.find(want_hospital);
		if(hos_iter == hos_map.end()){
			error_count += 1;
			std::cout << "Hospital name invalid,select one name from follow\n";
			
			hos_iter = hos_map.begin();
			for(; hos_iter != hos_map.end(); ++hos_iter){
				std::cout << "医院:" << hos_iter->first << "\n";
			}

			want_hospital.clear();
			std::cout << "Input:";
			std::cin >> want_hospital;
		}else{
			std::cout << "Hospital name valid\n";
		}

		if(error_count > 3){
			return -6;
		}

	}while(hos_iter == hos_map.end());
	error_count = 0;

	//check department name
	DEPMAP depart_map;
	DEPMAP::iterator depart_iter;
	if(control_manager.GetDepartListOfHospital(want_hospital,&depart_map) < 0){
		return -7;
	}

	do{
		if(want_department.empty()){
	
			depart_iter = depart_map.begin();
			for(; depart_iter != depart_map.end(); ++depart_iter){
				std::cout << "科室:" << depart_iter->first << "\n";
			}

			std::cout << "Input you want to select department name:\n";
			std::cin >> want_department;
		}

		depart_iter = depart_map.find(want_department);
		if(depart_iter == depart_map.end()){
			error_count += 1;
			std::cout << "Department name invalid,select one name from follow\n";
			
			depart_iter = depart_map.begin();
			for(; depart_iter != depart_map.end(); ++depart_iter){
				std::cout << "科室:" << depart_iter->first << "\n";
			}

			want_department.clear();
			std::cout << "Input:";
			std::cin >> want_department;
		}else{
			std::cout << "department name valid\n";
		}

		if(error_count > 3){
			return -6;
		}
	}while(depart_iter == depart_map.end());
	error_count = 0;

	//check doctor
	DETMAP detail_map;
	DETMAP::iterator det_iter;
	if(control_manager.GetDoctorListOfDepart(want_department,&detail_map) < 0){
		return -8;
	}

	do{
		if(want_doctor.empty()){
			det_iter = detail_map.begin();
			for(; det_iter != detail_map.end(); ++det_iter){
				std::cout << "医生:" << det_iter->first << "\n";
			}
			std::cout << "Input you want to select doctor name:\n";
			std::cin >> want_doctor;
		}

		det_iter = detail_map.find(want_doctor);
		if(det_iter == detail_map.end()){
			error_count += 1;
			std::cout << "Doctor name invalid,select one name from follow\n";
			
			det_iter = detail_map.begin();
			for(; det_iter != detail_map.end(); ++det_iter){
				std::cout << "医生:" << det_iter->first << "\n";
			}

			want_doctor.clear();
			std::cout << "Input:";
			std::cin >> want_doctor;
		}else{
			std::cout << "doctor name valid\n";
		}

		if(error_count > 3){
			return -6;
		}
	}while(det_iter == detail_map.end());

	//start threads
	control_manager.Run(want_hospital,want_department,want_doctor);
	return 0;
}

int main(int argc,char **argv)
{
	/*
	CHttpProcesser processer;
	processer.Init("guahao.zjol.com.cn",80);
	processer.Run();
	*/

	const char* optstring = "u:p:a:b:c:t:m:n:o:r:vh?";
	int ret = 0;
	if(argc > 1){
		while((ret = getopt(argc,argv,optstring)) != -1){
			switch((char)ret){
				case 'u':
					user.assign(optarg);
					break;
				case 'p':
					passwd.assign(optarg);
					break;
				case 'a':
					want_hospital.assign(optarg);
					break;
				case 'b':
					want_department.assign(optarg);
					break;
				case 'c':
					want_doctor.assign(optarg);
					break;
				case 't':
					want_week.assign(optarg);
					break;
				case 'o':
					log_file.assign(optarg);
					break;
				case 'r':
					result_file.assign(optarg);
					break;
				case 'm':
					thread_number.assign(optarg);
					break;
				case 'n':
					connection_number.assign(optarg);
					break;
				case 'v':
					std::cout << V_0_1_1 << std::endl; 
					return 0;
				case 'h':
				case '?':
					Usage(argv);
					return 0;
				default:
					Usage(argv);
					return 1;
			}
		}
	}else{
		std::cout << "argument invalid use \"" << argv[0] << " -h\" get help\n";
		return 1;
	}

	if((ret = Start()) < 0){
		std::cout << "start error:" << ret << " use \"" << argv[0] << " -h\" get help\n";
		return 2;
	}

	return 0;
}
