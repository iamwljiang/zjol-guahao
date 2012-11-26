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

#include "ssignal.h"
#include <signal.h>
#include <iostream>

boost::function0<void> console_ctrl_function;
unsigned int global_exit_flag = 0;
#ifndef WIN32
void init_signal() {
	struct sigaction act;
  	act.sa_handler = sig_handler;
  	sigemptyset(&act.sa_mask);
  	act.sa_flags = 0;
  	if (sigaction(SIGHUP, &act, NULL)
  	  || sigaction(SIGTERM, &act, NULL)
  	  || sigaction(SIGINT, &act, NULL)
  	  || sigaction(SIGABRT, &act, NULL))
  		std::cerr << "sigaction failed" << std::endl;
  	act.sa_handler = SIG_IGN;
  	if (sigaction(SIGPIPE, &act, NULL))
    	std::cerr << "sigaction failed" << std::endl;

  	for (int k = 0; k < NSIG; k++) {
    // 11--segmentation violation,
    // 18--child status change,
    // 20--window size change
    // 13--write on a pipe with no one to read it
    // other signals's meaning, please refer to "/usr/include/sys/signal.h"
    	if (k == 11 || k == 18 || k == 20 || k == 28) {
      		continue;
    	} else if (k == 13 || k == 6) {
      		continue;
    	} else {
    		signal(k, sig_handler);
    	}
  	}//end for

  	return;
}

void sig_handler(int v) {
	switch (v) {
		case SIGTERM:
		case SIGINT:
		case SIGABRT:
		  console_ctrl_function();
			break;
		default:
    		break;
  	}
}

#else
BOOL WINAPI console_ctrl_handler(DWORD ctrl_type) {
  switch (ctrl_type) {
  case CTRL_C_EVENT:
  case CTRL_BREAK_EVENT:
  case CTRL_CLOSE_EVENT:
  case CTRL_SHUTDOWN_EVENT:
    console_ctrl_function();
    return TRUE;
  default:
    return FALSE;
  }
}
#endif
