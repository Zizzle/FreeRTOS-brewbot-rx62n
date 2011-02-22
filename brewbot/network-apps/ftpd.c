
/**
 * \addtogroup exampleapps
 * @{
 */

/**
 * \defgroup ftp server
 * @{
 *
 * The uIP ftp server is a very simplistic implementation of an FTP
 * server. It can serve only one file in binary mode ed only the commands
 * to download a file are implemented.
 */


/**
 * \file
 * FTP server.
 * \author Fabio Giovagnini <[EMAIL PROTECTED]>
 */

/*
 * Copyright (c) 2006, Aurion s.r.l.
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.  
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * $Id: arnproto.cpp,v 1.1.2.1 2006/06/08 21:21:01 fabio Exp $
 *
 */
//#include "arnUtilFunctions.h"
#include "net/uip.h"
#include "ftpd.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//#define DEBUG
#ifdef DEBUG

#define PRINT(x) arnDEBUG("%s",x) /*printf("%s", x)*/
#define PRINTLN(x) arnDEBUG("%s\n",x) /*printf("%s\n", x)*/
#define PRINTNUM(x) arnDEBUG("%d",x) 
#define PRINTNUMLN(x) arnDEBUG("%d\n",x)
#else /* DEBUG */
#define PRINT(x)
#define PRINTLN(x)
#define PRINTNUMLN(x)
#define PRINTNUM(x)
#endif /* NOT DEBUG */

static void _abort(void);
static void _timeout(void);
static void _close(void);
static void _connect(void);
static void _newdata(void);
static void _ack(void);
static void _poll(void);
static void _senddata(void);
static void _retrasmit(void);

static void SplitCmdArg(char * line, char ** cmd, char ** args);

static const char cmd_cwd_P[]  = "CWD";
static const char cmd_dele_P[] = "DELE";
static const char cmd_list_P[] = "LIST";
static const char cmd_mkd_P[]  = "MKD";
static const char cmd_xmkd_P[] = "XMKD";
static const char cmd_nlst_P[] = "NLST";
static const char cmd_noop_P[] = "NOOP";
static const char cmd_pass_P[] = "PASS";
static const char cmd_pasv_P[] = "PASV";
static const char cmd_port_P[] = "PORT";
static const char cmd_pwd_P[]  = "PWD";
static const char cmd_xpwd_P[] = "XPWD";
static const char cmd_quit_P[] = "QUIT";
static const char cmd_retr_P[] = "RETR";
static const char cmd_rmd_P[]  = "RMD";
static const char cmd_xrmd_P[] = "XRMD";
static const char cmd_size_P[] = "SIZE";
static const char cmd_stor_P[] = "STOR";
static const char cmd_syst_P[] = "SYST";
static const char cmd_type_P[] = "TYPE";
static const char cmd_user_P[] = "USER";

static const char rep_banner[] = "220 uIP/arnSys FTP server ready\r\n";

static const char okCode200[] = "200 OK\r\n";
static const char okCode221[] = "221 OK\r\n";
static const char okCode225[] = "225 OK\r\n";
static const char okCode226[] = "226 OK\r\n";
static const char okCode230[] = "230 OK\r\n";

static const char FailCode500[] =  "500 sintax error; command unrecognized.\r\n";
static const char FailCode501[] =  "501 sintax error; arg unrecognized.\r\n";
static const char FailCode502[] =  "502 command not implemented.\r\n";
static const char FailCode504[] =  "504 command not implemented for that parameter.\r\n";
static const char FailCode421[] =  "421 servivece not available; closing connection.\r\n";

// this is the section defining the the allowed command
#define FTPDREC_NONE			0
#define FTPDREC_CONNECT			(1 + FTPDREC_NONE)
#define FTPDREC_NOOP			(1 + FTPDREC_CONNECT)
#define FTPDREC_USER			(1 + FTPDREC_NOOP)
#define FTPDREC_PWD				(1 + FTPDREC_USER)
#define FTPDREC_CWD				(1 + FTPDREC_PWD)
#define FTPDREC_PASV			(1 + FTPDREC_CWD)
#define FTPDREC_LIST			(1 + FTPDREC_PASV)
#define FTPDREC_RETR			(1 + FTPDREC_LIST)
#define FTPDREC_SYST			(1 + FTPDREC_RETR)
#define FTPDREC_SIZE			(1 + FTPDREC_SYST)
#define FTPDREC_QUIT			(1 + FTPDREC_SIZE)
#define FTPDREC_TYPE			(1 + FTPDREC_QUIT)
#define FTPDREC_UNK				(1 + FTPDREC_TYPE)
//END this is the section defining the the allowed command

// this is the section defining the answer to command
#define FTPDANS_NONE		0
#define FTPDANS_BANNER		(1 + FTPDANS_NONE)
#define FTPDANS_OK200		(1 + FTPDANS_BANNER) // command ok
#define FTPDANS_OK221		(1 + FTPDANS_OK200) // user name ok; no password
#define FTPDANS_OK225		(1 + FTPDANS_OK221) // user name ok; no password
#define FTPDANS_OK226		(1 + FTPDANS_OK225) // user name ok; no password
#define FTPDANS_OK230		(1 + FTPDANS_OK226) // user name ok; no password
#define FTPDANS_SYST		(1 + FTPDANS_OK230) // risponde al comando che identifica il sistema target
#define FTPDANS_PWD			(1 + FTPDANS_SYST) // risponde al comando che chiede la stampa della working directory
#define FTPDANS_TYPE		(1 + FTPDANS_PWD) // risponde al comando TYPE per stabilire se ASCII oppure BINARY
#define FTPDANS_PASV		(1 + FTPDANS_TYPE) // risponde al comando PASV
#define FTPDANS_LIST		(1 + FTPDANS_PASV) // risponde al LIST NLST
#define FTPDANS_FAIL502		(1 + FTPDANS_LIST) // risponde a comandi non conosciuti
#define FTPDANS_SIZE		(1 + FTPDANS_FAIL502) // risponde al SIZE
#define FTPDANS_RETR		(1 + FTPDANS_SIZE) // risponde al RETR
#define FTPDANS_CWD			(1 + FTPDANS_RETR) // risponde al CWD
#define FTPDANS_FAIL500		(1 + FTPDANS_CWD) // errore sintattico
#define FTPDANS_FAIL501		(1 + FTPDANS_FAIL500) // errore sintattico sui parametri
#define FTPDANS_FAIL504		(1 + FTPDANS_FAIL501) // comando non implementati per quel parametro
#define FTPDANS_FAIL421		(1 + FTPDANS_FAIL504) // servizio non disponibile; chiudo connesisone
#define FTPDANS_NOOP		(1 + FTPDANS_FAIL421) // no operation
//END this is the section defining the answer to command

// this is the section defining the internal status of the server
#define FTPDSTS_NONE					0
#define FTPDSTS_WAITFORCMD				(1 + FTPDSTS_NONE)
#define FTPDSTS_PREP_FTPDATA			(1 + FTPDSTS_WAITFORCMD)
#define FTPDSTS_SENDING_FTPDATA			(1 + FTPDSTS_PREP_FTPDATA)
#define FTPDSTS_SENT_FTPDATA			(1 + FTPDSTS_SENDING_FTPDATA)
#define FTPDSTS_SENDING_CTLANS			(1 + FTPDSTS_SENT_FTPDATA)
#define FTPDSTS_SENT_CTLANS				(1 + FTPDSTS_SENDING_CTLANS)
#define FTPDSTS_SENDING_ENDDATACTLANS	(1 + FTPDSTS_SENT_CTLANS)
#define FTPDSTS_SENT_ENDDATACTLANS		(1 + FTPDSTS_SENDING_ENDDATACTLANS)
//#define FTPDSTS_CLOSING_DATACONN		(1 + FTPDSTS_SENT_ENDDATACTLANS)
//#define FTPDSTS_CLOSED_DATACONN			(1 + FTPDSTS_CLOSING_DATACONN)
//END this is the section defining the internal status of the server

// this is the section defining the TYPE
#define FTPDTYP_NONE				0
#define FTPDTYP_ASCII				(1 + FTPDTYP_NONE)
#define FTPDTYP_BINARY				(1 + FTPDTYP_ASCII)
//END this is the section defining the TYPE

// this is the section defining the MODE
#define FTPDMOD_NONE				0
#define FTPDMOD_STREAM				(1 + FTPDMOD_NONE)
//END this is the section defining the MODE

// this is the section defining the STRUCTURE
#define FTPDSTRU_NONE				0
#define FTPDSTRU_FILE				(1 + FTPDSTRU_NONE)
//END this is the section defining the STRUCTURE


#define TRUE  1
#define FALSE 0


#define PACK_DATA_SIZE	512

static void _abort_data(void);
static void _timeout_data(void);
static void _close_data(void);
static void _connect_data(void);
static void _newdata_data(void);
static void _ack_data(void);
static void _poll_data(void);
static void _senddata_data(void);
static void _retrasmit_data(void);

struct {
    unsigned char Status;
    unsigned char RecvCmd;
    unsigned char AnsToCmd;
    unsigned char ftpMode;
    unsigned char ftpType;
    unsigned char ftpStru;
} exchgParams;

struct ftpd_state {
    int count;
    char *dataptr;
    char IsCmdWD;
    
    
    unsigned char Status;
    unsigned char RecvCmd;
    unsigned char AnsToCmd;
    unsigned char ftpMode;
    unsigned char ftpType;
    unsigned char ftpStru;
};


// test test test
unsigned long ftpd_size = 0x000000;
unsigned long ftpd_sadd = 0x00420000;
char* ftpd_name = "DDF.ddf";
//END


void ftpd_init(void)
{
    /* Listen to port 21. */
    uip_listen(HTONS(21));
}

void ftpd_init_data(void)
{
    /* Listen to port 20. */
    uip_listen(HTONS(20));
}

void ftpd_appcall_data(void)
{
    if(uip_aborted()) {
	_abort_data();
    }
    if(uip_timedout()) {
	_timeout_data();
    }
    if(uip_closed()) {
	_close_data();
    }
    if(uip_connected()) {
	_connect_data();
    }
    if(uip_acked()) {
	_ack_data();
    }
    if(uip_newdata()) {
	_newdata_data();
    }
    if(uip_poll()) {
	_poll_data();
    }
    if(uip_rexmit()) {
	_retrasmit_data();
    }
    if(uip_rexmit() ||
       uip_newdata() ||
       uip_acked() ||
       uip_connected() ||
       uip_poll()) {
	_senddata_data();
    }
}

void ftpd_appcall(void)
{
    if(uip_aborted()) {
	_abort();
    }
    if(uip_timedout()) {
	_timeout();
    }
    if(uip_closed()) {
	_close();
    }
    if(uip_connected()) {
	_connect();
    }
    if(uip_acked()) {
	_ack();
    }
    if(uip_newdata()) {
	_newdata();
    }
    if(uip_poll()) {
	_poll();
    }
    if(uip_rexmit()) {
	_retrasmit();
    }
    if(uip_rexmit() ||
       uip_newdata() ||
       uip_acked() ||
       uip_connected() ||
       uip_poll()) {
	_senddata();
    }
}

void _abort(void)
{
//	struct arnftpd_state *ftps = (struct arnftpd_state *)(uip_conn->appstate);
    PRINT("Abort_Handler:\n");
    uip_abort();
}

void _timeout(void)
{
//	struct arnftpd_state *ftps = (struct arnftpd_state *)(uip_conn->appstate);
    PRINT("Timeout_Handler:\n");
    uip_close();
}

void _close(void)
{
//	struct arnftpd_state *ftps = (struct arnftpd_state *)(uip_conn->appstate);
    PRINT("Close_Handler:\n");
//	exchgParams.Status = FTPDSTS_CLOSING_DATACONN;
}


void _connect(void)
{
    struct ftpd_state *ftps = (struct ftpd_state *)(&uip_conn->appstate);
    ftps->RecvCmd = FTPDREC_CONNECT;
    ftps->count = strlen(rep_banner);
    uip_len = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
    strncpy((char*) (uip_appdata),(char*)(rep_banner),uip_len);
    ftps->AnsToCmd = FTPDANS_BANNER;

    ftps->dataptr = 0;
    ftps->count = 0;
    ftps->IsCmdWD = FALSE;
    ftps->Status = FTPDSTS_WAITFORCMD;
    exchgParams.Status = FTPDSTS_NONE;
    exchgParams.ftpMode = FTPDMOD_STREAM;
    exchgParams.ftpType = FTPDTYP_ASCII;
    exchgParams.ftpStru = FTPDSTRU_FILE;
    //PRINT("Connect_Handler:\n");
}

void _newdata(void)
{
    struct ftpd_state *ftps = (struct ftpd_state *)(&uip_conn->appstate);
    char* cmd;
    char* arg;
    //PRINT("Newdata_Handler:\n");
    if (ftps->Status != FTPDSTS_WAITFORCMD) return;
    SplitCmdArg((char*)uip_appdata,&cmd,&arg);
    PRINTLN(cmd);
    PRINTLN(arg);
    if (!strcmp(cmd,cmd_user_P)) {
	ftps->RecvCmd = FTPDREC_USER;
	ftps->count = strlen(okCode230);
	uip_len = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
	strncpy((char*) (uip_appdata),(char*)(okCode230),uip_len);
	ftps->AnsToCmd = FTPDANS_OK230;
	ftps->IsCmdWD = FALSE;
	ftps->Status = FTPDSTS_SENDING_CTLANS;
    }
    else if (!strcmp(cmd,cmd_syst_P)) {
	char *msg = "215 UNIX Type: L8\r\n";
	ftps->RecvCmd = FTPDREC_SYST;
	ftps->count = strlen(msg);
	uip_len = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
	strncpy((char*) (uip_appdata),(char*)(msg),uip_len);
	ftps->AnsToCmd = FTPDANS_SYST;
	ftps->IsCmdWD = FALSE;
	ftps->Status = FTPDSTS_SENDING_CTLANS;
    }
    else if (!strcmp(cmd,cmd_pwd_P) || (!strcmp(cmd,cmd_xpwd_P))) {
	char *msg = "257 \"/\"\r\n";
	ftps->RecvCmd = FTPDREC_PWD;
	ftps->count = strlen(msg);
	uip_len = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
	strncpy((char*) (uip_appdata),(char*)(msg),uip_len);
	ftps->AnsToCmd = FTPDANS_PWD;
	ftps->IsCmdWD = FALSE;
	ftps->Status = FTPDSTS_SENDING_CTLANS;
    }
    else if (!strcmp(cmd,cmd_type_P)) {
	ftps->RecvCmd = FTPDREC_TYPE;
	if (!strcmp(arg,"A")) exchgParams.ftpType = FTPDTYP_ASCII;
	else exchgParams.ftpType = FTPDTYP_BINARY;
	ftps->count = strlen(okCode200);
	uip_len = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
	strncpy((char*) (uip_appdata),(char*)(okCode200),uip_len);
	ftps->AnsToCmd = FTPDANS_OK200;
	ftps->IsCmdWD = FALSE;
	ftps->Status = FTPDSTS_WAITFORCMD;
    }
    else if (!strcmp(cmd,cmd_quit_P)) {
	ftps->RecvCmd = FTPDREC_QUIT;
	ftps->count = strlen(okCode221);
	uip_len = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
	strncpy((char*) (uip_appdata),(char*)(okCode221),uip_len);
	ftps->AnsToCmd = FTPDANS_OK221;
	ftps->IsCmdWD = FALSE;
	ftps->Status = FTPDSTS_SENDING_CTLANS;
    }
    else if (!strcmp(cmd,cmd_pasv_P)) {
	uip_ipaddr_t hostaddr;
	uip_gethostaddr(&hostaddr);
	ftps->RecvCmd = FTPDREC_PASV;
	sprintf((char*)uip_appdata,"227 Passive (%u,%u,%u,%u,%u,%u).\r\n",
		uip_ipaddr_to_quad(&hostaddr), 0, 20); // port
	ftps->count = strlen((char*)uip_appdata);
	uip_len = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
	ftps->AnsToCmd = FTPDANS_PASV;
	ftps->IsCmdWD = FALSE;
	ftps->Status = FTPDSTS_SENDING_CTLANS;
    }
    else if (!strcmp(cmd,cmd_size_P)) {
	ftps->RecvCmd = FTPDREC_SIZE;
	char tmpBff[32];tmpBff[0] = '/';
	strcpy(tmpBff + 1,ftpd_name);
	if (!strcmp(arg,tmpBff) || !strcmp(arg,ftpd_name)) {
	    sprintf((char*)uip_appdata,"213 %6lu\r\n",
		    (unsigned long) ftpd_size); 
	    ftps->count = strlen((char*)uip_appdata);
	    uip_len = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
	    ftps->AnsToCmd = FTPDANS_SIZE;
	}
	else {
	    ftps->count = strlen(FailCode502);
	    uip_len = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
	    strncpy((char*) (uip_appdata),(char*)(FailCode502),uip_len);
	    ftps->AnsToCmd = FTPDANS_FAIL502;
	}
	ftps->IsCmdWD = FALSE;
	ftps->Status = FTPDSTS_SENDING_CTLANS;
    }
    else if (!strcmp(cmd,cmd_list_P) || !strcmp(cmd,cmd_nlst_P)) {
	char* msg = "150 opening data connection list answer.\r\n";
	ftps->RecvCmd = FTPDREC_LIST;
	ftps->count = strlen(msg);
	uip_len = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
	strncpy((char*) (uip_appdata),(char*)(msg),uip_len);
	ftps->AnsToCmd = FTPDANS_LIST;
	ftps->IsCmdWD = TRUE;
	ftps->Status = FTPDSTS_SENDING_CTLANS;
    }
    else if (!strcmp(cmd,cmd_retr_P)) {
	if (!strcmp(arg,"/")) {
	    ftps->count = strlen(FailCode504);
	    uip_len = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
	    strncpy((char*) (uip_appdata),(char*)(FailCode504),uip_len);
	    ftps->AnsToCmd = FTPDANS_FAIL504;
	    ftps->IsCmdWD = FALSE;
	}
	else {
	    char* msg = "150 opening data connection for retr answer.\r\n";
	    ftps->RecvCmd = FTPDREC_RETR;
	    ftps->count = strlen(msg);
	    uip_len = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
	    strncpy((char*) (uip_appdata),(char*)(msg),uip_len);
	    ftps->AnsToCmd = FTPDANS_RETR;
	    ftps->IsCmdWD = TRUE;
	}
	ftps->Status = FTPDSTS_SENDING_CTLANS;
    }
    else if (!strcmp(cmd,cmd_cwd_P)) {
	ftps->RecvCmd = FTPDREC_CWD;
	if (!strcmp(arg,"/")) {
	    char* msg = "200 directory changed to /.\r\n";
	    ftps->count = strlen(msg);
	    uip_len = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
	    strncpy((char*) (uip_appdata),(char*)(msg),uip_len);
	    ftps->AnsToCmd = FTPDANS_CWD;
	}
	else {
	    ftps->count = strlen(FailCode504);
	    uip_len = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
	    strncpy((char*) (uip_appdata),(char*)(FailCode504),uip_len);
	    ftps->AnsToCmd = FTPDANS_FAIL504;
	}
	ftps->IsCmdWD = FALSE;
	ftps->Status = FTPDSTS_SENDING_CTLANS;
    }
    else if (!strcmp(cmd,cmd_noop_P)) {
	ftps->RecvCmd = FTPDREC_NOOP;
	ftps->count = strlen(okCode200);
	uip_len = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
	strncpy((char*) (uip_appdata),(char*)(okCode200),uip_len);
	ftps->AnsToCmd = FTPDANS_NOOP;
	ftps->IsCmdWD = FALSE;
	ftps->Status = FTPDSTS_SENDING_CTLANS;
    }
    else {
	ftps->RecvCmd = FTPDREC_UNK;
	ftps->count = strlen(FailCode502);
	uip_len = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
	strncpy((char*) (uip_appdata),(char*)(FailCode502),uip_len);
	ftps->AnsToCmd = FTPDANS_FAIL502;
	ftps->IsCmdWD = FALSE;
	ftps->Status = FTPDSTS_SENDING_CTLANS;
    }
}

void _poll(void)
{
    struct ftpd_state *ftps = (struct ftpd_state *)(&uip_conn->appstate);
    if (ftps->IsCmdWD ) {
	if (exchgParams.Status == FTPDSTS_SENT_FTPDATA) {
	    ftps->Status = exchgParams.Status;
	    switch (ftps->AnsToCmd) {
	    case FTPDANS_LIST:
	    case FTPDANS_RETR:
		ftps->AnsToCmd = FTPDANS_OK226;
		ftps->count = strlen(okCode226);
		uip_len = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
		strncpy((char*) (uip_appdata),(char*)(okCode226),uip_len);
		ftps->IsCmdWD = FALSE;
		exchgParams.Status = FTPDSTS_NONE;
		ftps->Status = FTPDSTS_SENDING_ENDDATACTLANS;
		break;
	    default: break;
	    }
	}
    }
}

// test testt test
unsigned char TSTrtx;
//END
void _retrasmit(void)
{
    TSTrtx++;
}

void _ack(void)
{
    struct ftpd_state *ftps = (struct ftpd_state *)(&uip_conn->appstate);
    switch (ftps->Status) {
    case FTPDSTS_SENDING_CTLANS:
	switch (ftps->AnsToCmd) {
	case FTPDANS_BANNER:
	case FTPDANS_OK200: 
	case FTPDANS_OK221: 
	case FTPDANS_OK230: 
	case FTPDANS_SYST:
	case FTPDANS_PWD:
	case FTPDANS_PASV:
	case FTPDANS_FAIL502:
	case FTPDANS_FAIL504:
	case FTPDANS_SIZE:
	case FTPDANS_CWD:
	case FTPDANS_NOOP:
	    ftps->Status = FTPDSTS_WAITFORCMD;
	    ftps->RecvCmd = FTPDREC_NONE;
	    ftps->AnsToCmd = FTPDANS_NONE;
	    uip_len = 0;
	    break;
	case FTPDANS_LIST:
	case FTPDANS_RETR:
	    ftps->Status = FTPDSTS_PREP_FTPDATA;
	    uip_len = 0;
	    exchgParams.Status = ftps->Status;
	    exchgParams.RecvCmd = ftps->RecvCmd;
	    exchgParams.AnsToCmd = ftps->AnsToCmd;
	    break;
	default :
	    break;
	}
	break;
    case FTPDSTS_SENDING_ENDDATACTLANS:
	switch (ftps->AnsToCmd) {
	case FTPDANS_OK226:
	    ftps->IsCmdWD = FALSE;
	    ftps->Status = FTPDSTS_WAITFORCMD;
	    ftps->RecvCmd = FTPDREC_NONE;
	    ftps->AnsToCmd = FTPDANS_NONE;
	    break;
	default :
	    break;
	}
	break;
    default : break;
    }
/*	SplitCmdArg((char*)uip_appdata,&cmd,&arg);
	PRINTLN(cmd);
	PRINTLN(arg);
	if (!strcmp(cmd,cmd_user_P)) {
	ftps->Command = FTPDC_SENDOK200;
	ftps->count = strlen(okCode200);
	uip_len = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
	strncpy((char*) (uip_appdata),(char*)(okCode200),uip_len);
	}*/
}

void _senddata(void)
{
//	struct arnftpd_state *ftps = (struct arnftpd_state *)(uip_conn->appstate);
    if (uip_len > 0) {
	uip_send(uip_appdata,uip_len);
	//PRINT("Senddata_Handler:\n");
    }
}

void SplitCmdArg(char * line, char ** cmd, char ** args)
{
    /* Skip leading spaces. */
    while (*line && *line <= ' ') {
        line++;
    }

    /* The first word is the command. Convert it to upper case. */
    *cmd = line;
    while (*line > ' ') {
        if (*line >= (unsigned char) 'a' && *line <= (unsigned char) 'z') {
            *line -= (unsigned char) 'a' - 'A';
        }
        line++;
    }

    /* Mark end of the command word. */
    if (*line) {
        *line++ = '\0';
    }

    /* Skip spaces. */
    while (*line && *line <= ' ') {
        ++line;
    }

    /* Arguments start here. */
    *args = line;
    while (*line && *line != '\r' && *line != '\n') {
        line++;
    }

    /* Mark end of arguments. */
    *line = 0;
}


void _abort_data(void)
{
}

void _timeout_data(void)
{
    uip_close();
}

void _close_data(void)
{
}

void _connect_data(void)
{
    struct ftpd_state *ftps = (struct ftpd_state *)(&uip_conn->appstate);
    ftps->Status = FTPDSTS_NONE;
    ftps->RecvCmd = FTPDREC_NONE;
    ftps->dataptr = 0;
    ftps->count = 0;
}

// test testt test
unsigned char TSTrtx_data;
//END
void _retrasmit_data(void)
{
    TSTrtx_data++;
    struct ftpd_state *ftps = (struct ftpd_state *)(&uip_conn->appstate);
    switch (ftps->Status) {
    case FTPDSTS_SENDING_FTPDATA:
	switch (ftps->RecvCmd) {
	case FTPDREC_LIST:
	    sprintf((char*)uip_appdata,"-rw-r--r--  1 0 0 %6lu Jan 1 2007 %s\r\n",ftpd_size,ftpd_name);
	    ftps->count = strlen((char*)uip_appdata);
	    uip_len = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
	    break;
	case FTPDREC_RETR:
	    ftps->count += PACK_DATA_SIZE;
	    ftps->dataptr -= PACK_DATA_SIZE;
	    //uip_len = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
	    uip_len = ((ftps->count > PACK_DATA_SIZE) ? PACK_DATA_SIZE : ftps->count);
	    memcpy((void*)uip_appdata,(void*)ftps->dataptr,uip_len);
	    if (ftps->count > PACK_DATA_SIZE) ftps->count-= PACK_DATA_SIZE;
	    else ftps->count = 0;
	    ftps->dataptr+= PACK_DATA_SIZE;
	    break;
	default: break;
	}
	break;
    default : break;
    }
}

void _newdata_data(void)
{
}

void _ack_data(void)
{
    struct ftpd_state *ftps = (struct ftpd_state *)(&uip_conn->appstate);
    switch (ftps->Status) {
    case FTPDSTS_SENDING_FTPDATA:
	switch (ftps->RecvCmd) {
	case FTPDREC_LIST:
	    uip_close();
	    ftps->Status = FTPDSTS_SENT_FTPDATA;
	    exchgParams.Status = ftps->Status;
	    break;
	case FTPDREC_RETR:
	    if (!ftps->count) {
		uip_close();
		ftps->Status = FTPDSTS_SENT_FTPDATA;
		exchgParams.Status = ftps->Status;
	    }
	    else {
		//uip_len = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
		uip_len = ((ftps->count > PACK_DATA_SIZE) ? PACK_DATA_SIZE : ftps->count);
		memcpy((void*)uip_appdata,(void*)ftps->dataptr,uip_len);
		if (ftps->count > PACK_DATA_SIZE) ftps->count-= PACK_DATA_SIZE;
		else ftps->count = 0;
		ftps->dataptr += PACK_DATA_SIZE;
	    }
	    break;
	default: break;
	}
	break;
    default: break;
    }
}

void _poll_data(void)
{
    struct ftpd_state *ftps = (struct ftpd_state *)(&uip_conn->appstate);
    if (ftps->Status == FTPDSTS_NONE) {
	if (exchgParams.Status == FTPDSTS_PREP_FTPDATA) {
	    ftps->Status = exchgParams.Status;
	    ftps->RecvCmd = exchgParams.RecvCmd;
	}
    }
/*	if (exchgParams.Status == FTPDSTS_CLOSING_DATACONN) {
	exchgParams.Status = FTPDSTS_CLOSED_DATACONN;
	uip_close();
	}*/
    switch (ftps->Status) {
    case FTPDSTS_PREP_FTPDATA:
	switch (ftps->RecvCmd) {
	case FTPDREC_LIST:
	    sprintf((char*)uip_appdata,"-rw-r--r--  1 0 0 %6lu Jan 1 2007 %s\r\n",ftpd_size,ftpd_name);
	    ftps->count = strlen((char*)uip_appdata);
	    uip_len = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
	    ftps->Status = FTPDSTS_SENDING_FTPDATA;
	    break;
	case FTPDREC_RETR:
	    ftps->dataptr = (char*)ftpd_sadd;
	    ftps->count = ftpd_size;
	    //uip_len = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
	    uip_len = ((ftps->count > PACK_DATA_SIZE) ? PACK_DATA_SIZE : ftps->count);
	    memcpy((void*)uip_appdata,(void*)ftps->dataptr,uip_len);
	    if (ftps->count > PACK_DATA_SIZE) ftps->count-= PACK_DATA_SIZE;
	    else ftps->count = 0;
	    ftps->dataptr += PACK_DATA_SIZE;
	    ftps->Status = FTPDSTS_SENDING_FTPDATA;
	    break;
	default: break;
	}
	break;
    default : break;
    }
}

void _senddata_data(void)
{
//	struct arnftpd_state *ftps = (struct arnftpd_state *)(uip_conn->appstate);
    if (uip_len > 0) {
	uip_send(uip_appdata,uip_len);
	//PRINT("Senddata_Handler:\n");
    }
}
