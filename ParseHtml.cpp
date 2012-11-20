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
#include "ParseHtml.h"
#include <algorithm>
#include <string.h>

#ifdef DEBUG_PARSEHTML
#include <iostream>
#include <fstream>
#endif

#include "Strutil.h"

#define TRIM_ALL_BLANK(p,r) int i =0;while(){f+=1;}  

	CParseHtml::CParseHtml()
	{

	}

	CParseHtml::~CParseHtml()
	{

	}

	HOSMAP CParseHtml::ParseIndexHtml(char * data)
	{
		HOSMAP hospital_map;
		char *save_ptr_line = NULL;

		char tabg[] = "TabbedPanelsTabGroup";
		char tab[]  = "TabbedPanelsTab";
		char cont[] = "TabbedPanelsContent\"";
		int is_block_end = false;

		std::string sep_line = "\r\n";
		std::vector<std::string> lines;
		//分割行并去除空行
		split(data,lines,sep_line);
		int lines_len = lines.size();
		
		int level_number = 0;
		std::vector<std::string> hos_level_name;
		for(int i = 0; i < lines_len; ++i){
			//查找TabbedPanelsTabGroup的块
			if(std::string::npos != lines[i].find(tabg)){
				hos_level_name.clear();
				size_t b = 0;
				size_t e = 0;
				//在块中查找title
				while(1){
					size_t start_pos = lines[++i].find(tab);
					if(start_pos != std::string::npos){
						b=lines[i].find_last_of('<');
						e=lines[i].find_last_of('>',b-1);
						if(b != std::string::npos && e != std::string::npos){
							hos_level_name.push_back(lines[i].substr(e+1,b-e-1));	
						}
						
					}else{
						break;
					}
				}
				//在块中查找content
				for(size_t inx = 0 ; inx < hos_level_name.size(); ++inx){
					while(1){
						if(std::string::npos != lines[i++].find(cont)){
							//如果标识连着，则index不要前进一行,否则+1
							if(std::string::npos == lines[i-1].find("<ul")){
								i += 1;
							}
							while(1){
								std::vector<std::string>items;
								
								split((char*)lines[i++].c_str(),items,"\"");
								if(items.size() != 9){
									break;
								}
								HOS_INFO hi ;

								hi.uri   = items.at(3);
								hi.tel   = items.at(5);
								b = items.at(8).find('>');
								e = items.at(8).find('<');
								hi.name = items.at(8).substr(b+1,e-b-1);
								e=hi.name.find('(');
								if(e != std::string::npos){
									hi.name = hi.name.substr(0,e);
								}
								e=hi.name.find("（");
								if(e != std::string::npos){
									hi.name = hi.name.substr(0,e);
								}
#ifdef DEBUG_PARSEHTML
								//std::cout << hi.name << "\t" << hi.tel<< "\t" << hos_level_name.at(inx) << "\n";
#endif
								HOSMAP::iterator iter = hospital_map.find(hi.name);
								if(iter == hospital_map.end()){
									if(hos_level_name.at(inx).size() == 12){
										hi.level = hos_level_name.at(inx);
									}else{
										hi.addr = hos_level_name.at(inx);
									}
									hospital_map.insert(std::make_pair(hi.name,hi));
								}else{
									if(hos_level_name.at(inx).size() == 12){
										iter->second.level = hos_level_name.at(inx);
									}else{
										iter->second.addr = hos_level_name.at(inx);
									}
								}
							}
							break;
						}
					}
				}

			}else{
				continue;
			}
		}

#ifdef DEBUG_PARSEHTML
		HOSMAP::iterator iter = hospital_map.begin();
		for(; iter != hospital_map.end(); ++iter){
			/*
			std::cout << "hos:" ;
			std::cout.width(40);
			std::cout << std::left << iter->first ;
			std::cout << "\t tel:" << iter->second.tel
			<< "\t addr:" << iter->second.addr
			<< "\t level:"<< iter->second.level<< "\n";
			*/
			printf("hos:%-32s\ttel:%-20s \taddr:%-10s \tlevel:%-10s\n",iter->first.c_str(),
					iter->second.tel.c_str(),iter->second.addr.c_str(),iter->second.level.c_str());
		}
#endif
		return hospital_map;
	}

	DEPMAP CParseHtml::ParseHospitalHtml(char * data)
	{
		DEPMAP department_map;
		char sep[] = "\r\n";
		char top[] = "科室列表";
		char header[] = "<li>";

		std::vector<std::string> lines;
		split(data,lines,sep);

		int line_len = lines.size();
		int i = 0;
		for(; i < lines.size() ;++i){
			if(std::string::npos != lines.at(i).find(top)){
				i += 1;
				if(std::string::npos != lines.at(i).find("<div")){
					i+=1;
				}
				if(std::string::npos != lines.at(i).find("<ul")){
					i+=1;
				}

				size_t b = 0;
				size_t e = 0;
				while(1){
					if(std::string::npos != lines.at(i).find(header)){
						std::vector<std::string> items;
						split((char*)lines[i].c_str(),items,"\"");
						if(items.size() != 5){
							break;
						}

						DEPART_INFO di;
						di.uri  = items.at(1);

						b = items.at(4).find('>');
						e = items.at(4).find('<');
						di.name = items.at(4).substr(b+1,e-b-1);

						department_map.insert(std::make_pair(di.name,di));

					}else{
						break;
					}
					++i;
				}
			}
		}
#ifdef DEBUG_PARSEHTML
		DEPMAP::iterator iter = department_map.begin();
		for(; iter != department_map.end(); ++iter){
			std::cout << "department:" << iter->first << " uri:" << iter->second.uri << std::endl;
		}
#endif
		return department_map;
	}

	DETMAP CParseHtml::ParseDeparmentHtml(char * data)
	{
		DETMAP detail_map;

		char top[] = "排名不分先后";
		char sep[] = "\r\n";
		char tr[]  = "<tr";
		char etr[] = "</tr";
		char td[]  = "<td";

		std::vector<std::string> date_vec;


		std::vector<std::string> lines;
		split(data,lines,sep);

		int line_len = lines.size();
		int i = 0;

		size_t b = 0;
		size_t e = 0;
		for(; i < line_len; ++i){
			if(std::string::npos != lines.at(i).find(top)){
				i += 1;
				//查找日期
				char date_sep[] = "strong>";
				while(1){
					if(std::string::npos != lines.at(i).find(etr)){
						break;
					}

					b = lines.at(i).find(date_sep);
					if(b == std::string::npos){
						//html代码变更会造成判断不到
						break;
					}

					e = lines.at(i).find('<',b+strlen(date_sep));
					date_vec.push_back(lines.at(i).substr(b+strlen(date_sep),e-b-strlen(date_sep)));

					++i;
				}	
				
				i += 1;

				//跳过上午下午
				if(std::string::npos != lines.at(i++).find(tr)){
					while(1){
						if(std::string::npos != lines.at(i).find(etr)){
							break;
						}
						++i;
					}
				}else{
					break;
				}

				i += 1;
				
				//抽取医生信息
				while(1){
					//每个医生
					if(std::string::npos != lines.at(i++).find(tr)){
						std::string doc_line = lines.at(i++);
						std::string doc_uri;
						std::string doc_name ;
						std::string doc_level;
						std::vector<std::string> doc_items;
						split((char*)doc_line.c_str(),doc_items,"\"");

						if(doc_items.size() == 7){
							doc_uri = doc_items.at(3);
							//">徐斌</b></a> <br />副主任医师</td>"
							find_item(doc_items.at(6),">","<",0,doc_name);
							find_item(doc_items.at(6),">","<",-1,doc_level);
							//std::cout << "doc:" << doc_name << std::endl; 
						}else{
							find_item(doc_items.at(4),">","<",0,doc_name);
						}
						//TODO:考虑在这里去除名字中任意位置的空格
						int inx = 0;
						while(1){
							if(std::string::npos != lines.at(i).find(etr)){
								i+=1;
								break;
							}

							//存在号源
							if(lines.at(i).size() > 60){
								std::vector<std::string> items;
								split((char*)lines.at(i).c_str(),items,",':");

								DETAIL_INFO dei;
								dei.content = items.at(2);
								dei.ampm    = inx % 2;
								find_item((char*)items.at(9).c_str(),NULL,"(",0,dei.cost);

								std::vector<std::string> date_items;
								int date_inx = inx / 2;
								std::string date = "";
								date.assign(  date_vec.at(date_inx).c_str());
								split((char*)date.c_str(),date_items," ");
								dei.date    = date_items.at(0);
								dei.weekday = date_items.at(1);
								
								DETMAP::iterator iter = detail_map.find(doc_name);	
								if(iter == detail_map.end()){
									std::vector<DETAIL_INFO> de;

									de.push_back(dei);
									detail_map.insert(std::make_pair(doc_name,de));
								}else{
									iter->second.push_back(dei);
								}
							}
							++i;
							++inx;

						}
					}else{
						break;
					}
				}
			}
		}
#ifdef DEBUG_PARSEHTML
		DETMAP::iterator iter = detail_map.begin();
		for(; iter != detail_map.end(); ++iter){
			std::cout << "doc:"<< iter->first << "\n";
			std::vector<DETAIL_INFO>::iterator viter = iter->second.begin();
			for(; viter != iter->second.end(); ++viter){
				std::cout << "date:" << viter->date 
					<< " cost:" << viter->cost
					<< " sg:" << viter->content <<"\n";
			}
		}
#endif

		return detail_map;
	}

/*
	void   CParseHtml::split(char * data,std::vector<std::string>& items,const std::string& sep)
	{
		char * save_ptr = NULL;
		char * str      = NULL;
		char * token    = NULL;
		int i = 0;
		for(str=data; ;++i,str=NULL){
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
#ifdef DEBUG_PARSEHTML
			//std::cout << token << "\n";
#endif 
			std::string value ;
			value.assign(token);	
			items.push_back(value);		
		}
	}
*/
	void  CParseHtml::find_item(const std::string& data,const char* bstr, const char* estr, int n,std::string &result)
	{
		if(data.empty())
			return;
		
		size_t b = 0;
		size_t e = 0;
		//">徐斌</b></a> <br />副主任医师</td>"

		if(n >= 0){
			size_t next_pos = 0;
			for(int i = 0 ;i <= n; ++i){
				if( bstr != NULL){
					b = data.find(bstr,next_pos);
					next_pos = b + strlen(bstr);
				}

				if( estr != NULL){
					e = data.find(estr,next_pos);
					next_pos = e + strlen(estr);
				}else{
					e = -1;
				}

				if(next_pos == std::string::npos)
					break;
				if( i == n){
					int bstr_len = 0;
					if(bstr != NULL){
						bstr_len = strlen(bstr);
					}
					result = data.substr(b+bstr_len,e-b-bstr_len);
					break;
				}
			}
		}else{
			size_t prev_pos = 0;
			e = data.find_last_of(estr);
			b = data.find_last_of(bstr,e-strlen(estr));
			if(b == std::string::npos)
				return;
			result = data.substr(b+strlen(bstr),e-b-strlen(bstr));
		}

	}

#ifdef DEBUG_PARSEHTML

std::string readfile(const char* filename)
{
	std::ifstream in(filename);
	std::string buffer;
	if(!in)
		return "";
	std::string line;
	while(getline(in,line)){
		buffer+= line;
		buffer+= "\r\n";
	}
	in.close();
	return buffer;
}

int test_parse_index_html()
{
	
	std::string buffer = readfile("index.html");
	if(buffer.empty())
		return -1;
	CParseHtml ph;
	ph.ParseIndexHtml((char*)buffer.c_str());
	return 0;
}

int test_parse_department_html()
{
	std::string buffer = readfile("department.html");
	if(buffer.empty())
		return -1;
	CParseHtml ph;
	ph.ParseDeparmentHtml((char*)buffer.c_str());
	return 0;
}


int test_parse_hospital_html()
{
	std::string buffer = readfile("hospital.html");
	if(buffer.empty())
		return -1;
	
	CParseHtml ph;
	ph.ParseHospitalHtml((char*)buffer.c_str());
	return 0;
}


void test_find_item()
{
	CParseHtml ph;

	std::string v;
	ph.find_item(">徐斌</b></a> <br />副主任医师</td>",">","<",0,v);
	std::cout << "v:" << v<< std::endl;
	ph.find_item(">徐斌</b></a> <br />副主任医师</td>",">","<",1,v);
	std::cout << "v:" << v<< std::endl;
	ph.find_item(">徐斌</b></a> <br />副主任医师</td>",">","<",2,v);
	std::cout << "v:" << v<< std::endl;
	ph.find_item(">徐斌</b></a> <br />副主任医师</td>",">","<",3,v);
	std::cout << "v:" << v<< std::endl;
	ph.find_item(">徐斌</b></a> <br />副主任医师</td>",">","<",-1,v);
	std::cout << "v:" << v<< std::endl;
	ph.find_item("1元(以医院为准",NULL,"(",0,v);
	std::cout << "v:" << v<< std::endl;
	ph.find_item("1元(以医院为准",NULL,"元",0,v);
	std::cout << "v:" << v<< std::endl;
}

int main()
{
	/*  
	if(test_parse_index_html() < 0){
		std::cout << "parse index html error";
	}
	*/
	
	/*
	if(test_parse_hospital_html() < 0){
		std::cout << "parse hospital html error";
	}
	*/
	
	if(test_parse_department_html() < 0){
		std::cout << "parse department html error";
	}
	
	//test_find_item();
	return 0;
}
#endif
