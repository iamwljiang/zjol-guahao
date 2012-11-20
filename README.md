guhao
===========
针对guahao.zjol.com.cn进行自动挂号,初步实现命令行自动挂号,可以用于封装库够图形界面程序使用

usage:
============
./guahao -u user -p password [-a hospital [-b department [-c doctor] ] ] [-t weekday]

./guahao -h|-?|-v

-u user			用户名,需要在guahao.zjol.com.cn中注册过

-p passwd 		用户密码

-a hospital 		指定需要挂号的医院,可选

-b department   	指定需要挂号的科室,不为空的时候,必须-a也不为空

-c doctor  		指定需要挂号的医生,不为空的时候,必须-b也不为空

-t weekday 		用户指定需要预约的时间,从当天开始的7天,0-6代表礼拜天到礼拜礼拜六

-o logfile		用于指定运行log输出到目标文件,默认执行目录下的guahao_info.log

-r result		用于指定挂号成功信息的目标文件,默认执行目录下的result.txt

-v              	显示版本号,并提示当前版本支持的功能

-h,-?			显示帮助,有问题可联系iamwljiang@gmail


future
================
1.多线程交互支持

2.非阻塞支持,数据分段接受

3.注释规范化

4.日志记录

5.windows支持

6.验证码自动识别

7.网路调整,抽取带外部


dependence
================
apache apr