
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
#include "fatfs/ff.h"
#include "lcd.h"

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
#define FTPDREC_PWD			(1 + FTPDREC_USER)
#define FTPDREC_CWD			(1 + FTPDREC_PWD)
#define FTPDREC_PASV			(1 + FTPDREC_CWD)
#define FTPDREC_LIST			(1 + FTPDREC_PASV)
#define FTPDREC_RETR			(1 + FTPDREC_LIST)
#define FTPDREC_STOR                    (1 + FTPDREC_RETR)
#define FTPDREC_SYST			(1 + FTPDREC_STOR)
#define FTPDREC_SIZE			(1 + FTPDREC_SYST)
#define FTPDREC_QUIT			(1 + FTPDREC_SIZE)
#define FTPDREC_TYPE			(1 + FTPDREC_QUIT)
#define FTPDREC_UNK			(1 + FTPDREC_TYPE)
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
#define FTPDANS_PWD		(1 + FTPDANS_SYST) // risponde al comando che chiede la stampa della working directory
#define FTPDANS_TYPE		(1 + FTPDANS_PWD) // risponde al comando TYPE per stabilire se ASCII oppure BINARY
#define FTPDANS_PASV		(1 + FTPDANS_TYPE) // risponde al comando PASV
#define FTPDANS_LIST		(1 + FTPDANS_PASV) // risponde al LIST NLST
#define FTPDANS_FAIL502		(1 + FTPDANS_LIST) // risponde a comandi non conosciuti
#define FTPDANS_SIZE		(1 + FTPDANS_FAIL502) // risponde al SIZE
#define FTPDANS_RETR		(1 + FTPDANS_SIZE) // risponde al RETR
#define FTPDANS_STOR            (1 + FTPDANS_RETR)
#define FTPDANS_CWD		(1 + FTPDANS_STOR) // risponde al CWD
#define FTPDANS_FAIL500		(1 + FTPDANS_CWD) // errore sintattico
#define FTPDANS_FAIL501		(1 + FTPDANS_FAIL500) // errore sintattico sui parametri
#define FTPDANS_FAIL504		(1 + FTPDANS_FAIL501) // comando non implementati per quel parametro
#define FTPDANS_FAIL421		(1 + FTPDANS_FAIL504) // servizio non disponibile; chiudo connesisone
#define FTPDANS_NOOP		(1 + FTPDANS_FAIL421) // no operation
//END this is the section defining the answer to command

// this is the section defining the internal status of the server
#define FTPDSTS_NONE				0
#define FTPDSTS_WAITFORCMD			(1 + FTPDSTS_NONE)
#define FTPDSTS_PREP_FTPDATA			(1 + FTPDSTS_WAITFORCMD)
#define FTPDSTS_SENDING_FTPDATA			(1 + FTPDSTS_PREP_FTPDATA)
#define FTPDSTS_SENT_FTPDATA			(1 + FTPDSTS_SENDING_FTPDATA)
#define FTPDSTS_SENDING_CTLANS			(1 + FTPDSTS_SENT_FTPDATA)
#define FTPDSTS_SENT_CTLANS			(1 + FTPDSTS_SENDING_CTLANS)
#define FTPDSTS_SENDING_ENDDATACTLANS    	(1 + FTPDSTS_SENT_CTLANS)
#define FTPDSTS_SENT_ENDDATACTLANS		(1 + FTPDSTS_SENDING_ENDDATACTLANS)
#define FTPDSTS_RECV_DATA                       (1 + FTPDSTS_SENT_ENDDATACTLANS)
#define FTPDSTS_RECV_DONE                       (1 + FTPDSTS_RECV_DATA)
//#define FTPDSTS_CLOSING_DATACONN		(1 + FTPDSTS_SENT_ENDDATACTLANS)
//#define FTPDSTS_CLOSED_DATACONN		(1 + FTPDSTS_CLOSING_DATACONN)
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
    int pos;
    char IsCmdWD;
    
    
    unsigned char Status;
    unsigned char RecvCmd;
    unsigned char AnsToCmd;
    unsigned char ftpMode;
    unsigned char ftpType;
    unsigned char ftpStru;
};

FIL file;

// test test test
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

    ftps->count = 0;
    ftps->IsCmdWD = FALSE;
    ftps->Status = FTPDSTS_WAITFORCMD;
    exchgParams.Status = FTPDSTS_NONE;
    exchgParams.ftpMode = FTPDMOD_STREAM;
    exchgParams.ftpType = FTPDTYP_ASCII;
    exchgParams.ftpStru = FTPDSTRU_FILE;
    //PRINT("Connect_Handler:\n");
}

void prep_response(struct ftpd_state *ftps, const char *msg, int recvCmd, int ansCmd, int status)
{
    ftps->RecvCmd = recvCmd;
    ftps->count   = strlen(msg);
    uip_len       = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
    strncpy((char*) (uip_appdata),(char*)(msg),uip_len);
    ftps->AnsToCmd = ansCmd;
    ftps->Status   = status;
    ftps->IsCmdWD  = FALSE;
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
	prep_response(ftps, okCode230, FTPDREC_USER, FTPDANS_OK230, FTPDSTS_SENDING_CTLANS);
    }
    else if (!strcmp(cmd,cmd_syst_P)) {
	prep_response(ftps, "215 UNIX Type: L8\r\n", FTPDREC_SYST, FTPDANS_SYST, FTPDSTS_SENDING_CTLANS);
    }
    else if (!strcmp(cmd,cmd_pwd_P) || (!strcmp(cmd,cmd_xpwd_P))) {
	prep_response(ftps, "257 \"/\"\r\n", FTPDREC_PWD, FTPDANS_PWD, FTPDSTS_SENDING_CTLANS);
    }
    else if (!strcmp(cmd,cmd_type_P)) {
	if (!strcmp(arg,"A")) exchgParams.ftpType = FTPDTYP_ASCII;
	else exchgParams.ftpType = FTPDTYP_BINARY;
	prep_response(ftps, okCode200, FTPDREC_TYPE, FTPDANS_OK200, FTPDSTS_WAITFORCMD);
    }
    else if (!strcmp(cmd,cmd_quit_P)) {
	prep_response(ftps, okCode221, FTPDREC_QUIT, FTPDANS_OK221, FTPDSTS_SENDING_CTLANS);
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
	FILINFO file_info;
	FRESULT result = f_stat (arg, &file_info); 

	if (result == FR_OK)
	{
	    char msg[30];
	    sprintf(msg, "213 %6lu\r\n", file_info.fsize); 
	    prep_response(ftps, msg, FTPDREC_SIZE, FTPDANS_SIZE, FTPDSTS_SENDING_CTLANS);
	}
	else
	{
	    prep_response(ftps, FailCode502, FTPDREC_SIZE, FTPDANS_FAIL502, FTPDSTS_SENDING_CTLANS);
	}
    }
    else if (!strcmp(cmd,cmd_list_P) || !strcmp(cmd,cmd_nlst_P)) {
	prep_response(ftps, "150 opening data connection list answer.\r\n", FTPDREC_LIST, FTPDANS_LIST, FTPDSTS_SENDING_CTLANS);
	ftps->IsCmdWD = TRUE;
    }
    else if (!strcmp(cmd,cmd_retr_P)) {
	FRESULT result = f_open(&file, arg, FA_READ);
	if (result == FR_OK)
	{
	    prep_response(ftps, "150 opening data connection for retr answer.\r\n", FTPDREC_RETR, FTPDANS_RETR, FTPDSTS_SENDING_CTLANS);
	    ftps->IsCmdWD = TRUE;
	}
	else
	{
	    char msg[40];
	    sprintf(msg, "%s %d", FailCode504, result);
	    prep_response(ftps, FailCode504, FTPDREC_RETR, FTPDANS_FAIL504, FTPDSTS_SENDING_CTLANS);
	}
    }
    else if (!strcmp(cmd,cmd_stor_P)) {
	FRESULT result = f_open(&file, arg, FA_WRITE | FA_CREATE_ALWAYS);
	if (result == FR_OK)
	{
	    prep_response(ftps, "150 opening data connection for stor.\r\n", FTPDREC_STOR, FTPDANS_STOR, FTPDSTS_SENDING_CTLANS);
	    ftps->IsCmdWD = TRUE;
	}
	else
	{
	    prep_response(ftps, FailCode504, FTPDREC_STOR, FTPDANS_FAIL504, FTPDSTS_SENDING_CTLANS);
	}
    }
    else if (!strcmp(cmd,cmd_cwd_P)) {
	FRESULT result = f_chdir(arg);
	if (result == FR_OK)
	{
	    prep_response(ftps, "200 directory changed to /.\r\n", FTPDREC_CWD, FTPDANS_CWD, FTPDSTS_SENDING_CTLANS);
	}
	else
	{
	    prep_response(ftps, FailCode504, FTPDREC_CWD, FTPDANS_FAIL504, FTPDSTS_SENDING_CTLANS);
	}
    }
    else if (!strcmp(cmd,cmd_noop_P)) {
	prep_response(ftps, okCode200,  FTPDREC_NOOP, FTPDANS_NOOP, FTPDSTS_SENDING_CTLANS);
    }
    else {
	prep_response(ftps, FailCode502, FTPDREC_UNK, FTPDANS_FAIL502, FTPDSTS_SENDING_CTLANS);
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
	    case FTPDANS_STOR:
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
	    ftps->Status   = FTPDSTS_WAITFORCMD;
	    ftps->RecvCmd  = FTPDREC_NONE;
	    ftps->AnsToCmd = FTPDANS_NONE;
	    uip_len = 0;
	    break;
	case FTPDANS_LIST:
	case FTPDANS_RETR:
	case FTPDANS_STOR:
	    ftps->Status = FTPDSTS_PREP_FTPDATA;
	    uip_len = 0;
	    exchgParams.Status   = ftps->Status;
	    exchgParams.RecvCmd  = ftps->RecvCmd;
	    exchgParams.AnsToCmd = ftps->AnsToCmd;
	    break;
	default :
	    break;
	}
	break;
    case FTPDSTS_SENDING_ENDDATACTLANS:
	switch (ftps->AnsToCmd) {
	case FTPDANS_OK226:
	    ftps->IsCmdWD  = FALSE;
	    ftps->Status   = FTPDSTS_WAITFORCMD;
	    ftps->RecvCmd  = FTPDREC_NONE;
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


void generate_file_list()
{
    DIR dir;
    FILINFO fno;
    char *fn;

#if _USE_LFN
    static char lfn[_MAX_LFN * (_DF1S ? 2 : 1) + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof(lfn);
#endif

    FRESULT result = f_opendir (&dir, "");
    if (result != FR_OK)
    {
	return;
    }

    uip_len = 0;

    for (;;)
    {
	char line[50];
	int line_len;

	result = f_readdir(&dir, &fno);
	if (result != FR_OK || fno.fname[0] == 0) break;
	if (fno.fname[0] == '.') continue;
#if _USE_LFN
	fn = *fno.lfname ? fno.lfname : fno.fname;
#else
	fn = fno.fname;
#endif

	if (fno.fattrib & AM_DIR)
	{
	    line_len = sprintf(line,"drw-r--r--  1 0 0 %6lu Jan 1 2007 %s\r\n", (unsigned long)0, fn);
	}
	else
	{
	    line_len = sprintf(line,"-rw-r--r--  1 0 0 %6lu Jan 1 2007 %s\r\n", (unsigned long)fno.fsize, fn);
	}

	if (uip_len + line_len < uip_mss())
	{
	    strcat((char*)uip_appdata + uip_len, line);
	    uip_len += line_len;
	}
    }
}

static void transmit_data(struct ftpd_state *ftps)
{
    UINT bytes_read = ftps->count;    

    if( bytes_read > uip_mss() )
    {
	bytes_read = uip_mss();
    }
    f_lseek(&file, ftps->pos);
    f_read ( &file, uip_appdata, bytes_read, &bytes_read);
    uip_len = bytes_read;
    ftps->count -= uip_len;    
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
    struct ftpd_state *ftps = (struct ftpd_state *)(&uip_conn->appstate);

    switch (ftps->Status) {
    case FTPDSTS_PREP_FTPDATA:
    case FTPDSTS_RECV_DATA:
	f_close(&file);
	ftps->Status = FTPDSTS_SENT_FTPDATA;
	exchgParams.Status = ftps->Status;
	break;
    }
}

void _connect_data(void)
{
    struct ftpd_state *ftps = (struct ftpd_state *)(&uip_conn->appstate);
    ftps->Status = FTPDSTS_NONE;
    ftps->RecvCmd = FTPDREC_NONE;
    ftps->count = 0;

    if (exchgParams.Status == FTPDSTS_PREP_FTPDATA) {
	ftps->Status = exchgParams.Status;
	ftps->RecvCmd = exchgParams.RecvCmd;
    }
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
	    generate_file_list();
	    break;
	case FTPDREC_RETR:
	    transmit_data(ftps);
	    break;
	default: break;
	}
	break;
    default : break;
    }
}
void _poll_data(void)
{
    struct ftpd_state *ftps = (struct ftpd_state *)(&uip_conn->appstate);

//    lcd_printf(0, 5, 19, "poll data");

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
	    generate_file_list();
	    ftps->Status = FTPDSTS_SENDING_FTPDATA;
	    break;
	case FTPDREC_RETR:
	    ftps->pos     = 0;
	    ftps->count   = file.fsize;
	    ftps->Status  = FTPDSTS_SENDING_FTPDATA;
	    transmit_data(ftps);
	    break;
	case FTPDREC_STOR:
	    break;
	default: break;
	}
	break;
    default : break;
    }
}

void _newdata_data(void)
{
    struct ftpd_state *ftps = (struct ftpd_state *)(&uip_conn->appstate);
    if (ftps->Status == FTPDSTS_NONE) {
	if (exchgParams.Status == FTPDSTS_PREP_FTPDATA) {
	    ftps->Status = exchgParams.Status;
	    ftps->RecvCmd = exchgParams.RecvCmd;
	}
    }

    UINT written;
    switch (ftps->Status) {
    case FTPDSTS_PREP_FTPDATA:
	    ftps->pos     = 0;
	    ftps->count   = 0;
	    ftps->Status  = FTPDSTS_RECV_DATA;
	    // fall through
    case FTPDSTS_RECV_DATA:
	f_write(&file, uip_appdata, uip_len, &written);
	break;
    }

    uip_len = 0;
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
		f_close(&file);
		uip_close();
		ftps->Status = FTPDSTS_SENT_FTPDATA;
		exchgParams.Status = ftps->Status;
	    }
	    else {
		ftps->pos = file.fsize - ftps->count;
		transmit_data(ftps);
	    }
	    break;
	default: break;
	}
	break;
    default: break;
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

void ftpd_appcall_data(void)
{
    if(uip_aborted()) {
	_abort_data();
    }
    if(uip_timedout()) {
	_timeout_data();
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
    if(uip_closed()) {
	_close_data();
    }
}
