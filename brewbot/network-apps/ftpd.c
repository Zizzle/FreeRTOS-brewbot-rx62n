
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

#include "serial.h"

#define PRINT(x)
#define PRINTLN(x)
#define PRINTNUMLN(x)
#define PRINTNUM(x)

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

static const char rep_banner[] = "220 uIP/brewbot FTP server ready\r\n";

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

enum FtpCmd
{
    FTPDREC_NONE,
    FTPDREC_CONNECT,
    FTPDREC_NOOP,
    FTPDREC_USER,
    FTPDREC_PWD,
    FTPDREC_CWD,
    FTPDREC_PASV,
    FTPDREC_LIST,
    FTPDREC_RETR,
    FTPDREC_STOR,
    FTPDREC_SYST,
    FTPDREC_SIZE,
    FTPDREC_MKD,
    FTPDREC_RMD,
    FTPDREC_DEL,
    FTPDREC_QUIT,
    FTPDREC_TYPE,
    FTPDREC_UNK
};

enum FtpAnswer
{
    FTPDANS_NONE,
    FTPDANS_BANNER,
    FTPDANS_OK200,
    FTPDANS_OK221,
    FTPDANS_OK225,
    FTPDANS_OK226,
    FTPDANS_OK230,
    FTPDANS_SYST,
    FTPDANS_PWD,
    FTPDANS_TYPE,
    FTPDANS_PASV,
    FTPDANS_LIST,
    FTPDANS_FAIL502,
    FTPDANS_SIZE,
    FTPDANS_MKD,
    FTPDANS_RMD,
    FTPDANS_DEL,
    FTPDANS_RETR,
    FTPDANS_STOR,
    FTPDANS_CWD,
    FTPDANS_FAIL500,
    FTPDANS_FAIL501,
    FTPDANS_FAIL504,
    FTPDANS_FAIL421,
    FTPDANS_NOOP
};

enum FrpStatus
{
    FTPDSTS_NONE,
    FTPDSTS_WAITFORCMD,
    FTPDSTS_PREP_FTPDATA,
    FTPDSTS_SENDING_FTPDATA,
    FTPDSTS_SENT_FTPDATA,
    FTPDSTS_SENDING_CTLANS,
    FTPDSTS_SENT_CTLANS,
    FTPDSTS_SENDING_ENDDATACTLANS,
    FTPDSTS_SENT_ENDDATACTLANS,
    FTPDSTS_RECV_DATA
};

enum FtpDataType
{
    FTPDTYP_NONE,
    FTPDTYP_ASCII,
    FTPDTYP_BINARY
};

#define FTPDMOD_NONE				0
#define FTPDMOD_STREAM				(1 + FTPDMOD_NONE)

#define FTPDSTRU_NONE				0
#define FTPDSTRU_FILE				(1 + FTPDSTRU_NONE)

#define TRUE  1
#define FALSE 0

static FIL file;
static struct ftpd_state *waiting_for_data_con;

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

static void _abort(void)
{
//	struct arnftpd_state *ftps = (struct arnftpd_state *)(uip_conn->appstate);
    PRINT("Abort_Handler:\n");
    uip_abort();
}

static void _timeout(void)
{
//	struct arnftpd_state *ftps = (struct arnftpd_state *)(uip_conn->appstate);
    PRINT("Timeout_Handler:\n");
    uip_close();
}

static void _close(void)
{
//	struct arnftpd_state *ftps = (struct arnftpd_state *)(uip_conn->appstate);
    PRINT("Close_Handler:\n");
//	exchgParams.Status = FTPDSTS_CLOSING_DATACONN;
}


static void _connect(void)
{
    struct ftpd_state *ftps = (struct ftpd_state *)(&uip_conn->appstate);
    ftps->RecvCmd = FTPDREC_CONNECT;
    ftps->count   = strlen(rep_banner);
    uip_len = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
    strncpy((char*) (uip_appdata),(char*)(rep_banner),uip_len);
    ftps->AnsToCmd = FTPDANS_BANNER;
    ftps->count    = 0;
    ftps->IsCmdWD  = FALSE;
    ftps->Status   = FTPDSTS_WAITFORCMD;
    ftps->other    = NULL;
    ftps->uip_conn = uip_conn;
    strcpy(ftps->cwd, "/");
}

static void prep_response(struct ftpd_state *ftps, const char *msg, int recvCmd, int ansCmd, int status, int isCmdWD)
{
    ftps->RecvCmd = recvCmd;
    ftps->count   = strlen(msg);
    uip_len       = ((ftps->count > uip_mss()) ? uip_mss() : ftps->count);
    strncpy((char*) (uip_appdata),(char*)(msg),uip_len);
    ftps->AnsToCmd = ansCmd;
    ftps->Status   = status;
    ftps->IsCmdWD  = isCmdWD;

    if (isCmdWD && ftps->other && ftps->other->uip_conn)
    {
	ftps->other->Status  = FTPDSTS_PREP_FTPDATA;
	ftps->other->RecvCmd = recvCmd;
    }
}

static void _newdata(void)
{
    struct ftpd_state *ftps = (struct ftpd_state *)(&uip_conn->appstate);
    char* cmd;
    char* arg;
    char msg[40];
    char path[128];
    //PRINT("Newdata_Handler:\n");
    if (ftps->Status != FTPDSTS_WAITFORCMD) return;

    SplitCmdArg((char*)uip_appdata,&cmd,&arg);
    PRINTLN(cmd);
    PRINTLN(arg);

    snprintf(path, sizeof(path), "%s/%s", ftps->cwd, arg);

    if (!strcmp(cmd,cmd_user_P)) {
	prep_response(ftps, okCode230, FTPDREC_USER, FTPDANS_OK230, FTPDSTS_SENDING_CTLANS, FALSE);
    }
    else if (!strcmp(cmd,cmd_syst_P)) {
	prep_response(ftps, "215 UNIX Type: L8\r\n", FTPDREC_SYST, FTPDANS_SYST, FTPDSTS_SENDING_CTLANS, FALSE);
    }
    else if (!strcmp(cmd,cmd_pwd_P) || (!strcmp(cmd,cmd_xpwd_P))) {
	snprintf(path, sizeof(path), "257 \"%s\"\r\n", ftps->cwd);
	prep_response(ftps, path, FTPDREC_PWD, FTPDANS_PWD, FTPDSTS_SENDING_CTLANS, FALSE);
    }
    else if (!strcmp(cmd,cmd_type_P)) {
	if (!strcmp(arg,"A")) ftps->ftpType = FTPDTYP_ASCII;
	else ftps->ftpType = FTPDTYP_BINARY;
	prep_response(ftps, okCode200, FTPDREC_TYPE, FTPDANS_OK200, FTPDSTS_WAITFORCMD, FALSE);
    }
    else if (!strcmp(cmd,cmd_quit_P)) {
	prep_response(ftps, okCode221, FTPDREC_QUIT, FTPDANS_OK221, FTPDSTS_SENDING_CTLANS, FALSE);
    }
    else if (!strcmp(cmd,cmd_pasv_P)) {
	uip_ipaddr_t hostaddr;
	uip_gethostaddr(&hostaddr);
	ftps->RecvCmd = FTPDREC_PASV;
	sprintf(msg,"227 Passive (%u,%u,%u,%u,%u,%u).\r\n", uip_ipaddr_to_quad(&hostaddr), 0, 20); // port
	prep_response(ftps, msg, FTPDREC_PASV, FTPDANS_PASV, FTPDSTS_SENDING_CTLANS, FALSE);
	waiting_for_data_con = ftps;
    }
    else if (!strcmp(cmd,cmd_size_P)) {
	FILINFO file_info;
	if (f_stat (path, &file_info) == FR_OK)
	{
	    sprintf(msg, "213 %6lu\r\n", file_info.fsize); 
	    prep_response(ftps, msg, FTPDREC_SIZE, FTPDANS_SIZE, FTPDSTS_SENDING_CTLANS, FALSE);
	}
	else
	{
	    prep_response(ftps, FailCode502, FTPDREC_SIZE, FTPDANS_FAIL502, FTPDSTS_SENDING_CTLANS, FALSE);
	}
    }
    else if (!strcmp(cmd,cmd_list_P) || !strcmp(cmd,cmd_nlst_P)) {
	prep_response(ftps, "150 opening data connection list answer.\r\n", FTPDREC_LIST, FTPDANS_LIST, FTPDSTS_SENDING_CTLANS, TRUE);
    }
    else if (!strcmp(cmd,cmd_retr_P)) {
	if (f_open(&file, path, FA_READ) == FR_OK)
	{
	    prep_response(ftps, "150 opening data connection for retr answer.\r\n", FTPDREC_RETR, FTPDANS_RETR, FTPDSTS_SENDING_CTLANS, TRUE);
	}
	else
	{
	    prep_response(ftps, FailCode504, FTPDREC_RETR, FTPDANS_FAIL504, FTPDSTS_SENDING_CTLANS, FALSE);
	}
    }
    else if (!strcmp(cmd,cmd_stor_P)) {
	if (f_open(&file, path, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK)
	{
	    prep_response(ftps, "150 opening data connection for stor.\r\n", FTPDREC_STOR, FTPDANS_STOR, FTPDSTS_SENDING_CTLANS, TRUE);
	}
	else
	{
	    prep_response(ftps, FailCode504, FTPDREC_STOR, FTPDANS_FAIL504, FTPDSTS_SENDING_CTLANS, FALSE);
	}
    }
    else if (!strcmp(cmd,cmd_cwd_P))
    {
	FILINFO file_info;
	if (f_stat (path, &file_info) == FR_OK && (file_info.fattrib & AM_DIR))
	{
	    prep_response(ftps, "200 directory changed.\r\n", FTPDREC_CWD, FTPDANS_CWD, FTPDSTS_SENDING_CTLANS, FALSE);
	    strncpy(ftps->cwd, path, sizeof(ftps->cwd));
	}
	else
	{
	    prep_response(ftps, FailCode504, FTPDREC_CWD, FTPDANS_FAIL504, FTPDSTS_SENDING_CTLANS, FALSE);
	}
    }
    else if (!strcmp(cmd,cmd_mkd_P)) {
	if (f_mkdir(path) == FR_OK)
	{
	    prep_response(ftps, "200 dir created.\r\n", FTPDREC_MKD, FTPDANS_MKD, FTPDSTS_SENDING_CTLANS, FALSE);
	}
	else
	{
	    prep_response(ftps, FailCode504, FTPDREC_MKD, FTPDANS_FAIL504, FTPDSTS_SENDING_CTLANS, FALSE);
	}
    }
    else if (!strcmp(cmd,cmd_rmd_P)) {
	if (f_unlink(path) == FR_OK)
	{
	    prep_response(ftps, "200 dir delted.\r\n", FTPDREC_RMD, FTPDANS_RMD, FTPDSTS_SENDING_CTLANS, FALSE);
	}
	else
	{
	    prep_response(ftps, FailCode504, FTPDREC_RMD, FTPDANS_FAIL504, FTPDSTS_SENDING_CTLANS, FALSE);
	}
    }
    else if (!strcmp(cmd,cmd_dele_P)) {
	if (f_unlink(path) == FR_OK)
	{
	    prep_response(ftps, "200 file delted.\r\n", FTPDREC_DEL, FTPDANS_DEL, FTPDSTS_SENDING_CTLANS, FALSE);
	}
	else
	{
	    prep_response(ftps, FailCode504, FTPDREC_DEL, FTPDANS_FAIL504, FTPDSTS_SENDING_CTLANS, FALSE);
	}
    }
    else if (!strcmp(cmd,cmd_noop_P)) {
	prep_response(ftps, okCode200,  FTPDREC_NOOP, FTPDANS_NOOP, FTPDSTS_SENDING_CTLANS, FALSE);
    }
    else {
	prep_response(ftps, FailCode502, FTPDREC_UNK, FTPDANS_FAIL502, FTPDSTS_SENDING_CTLANS, FALSE);
    }
}

static void _poll(void)
{
    struct ftpd_state *ftps = (struct ftpd_state *)(&uip_conn->appstate);
    if (ftps->IsCmdWD )
    {
	if (ftps->Status == FTPDSTS_SENT_FTPDATA)
	{
	    switch (ftps->AnsToCmd) {
	    case FTPDANS_LIST:
	    case FTPDANS_RETR:
	    case FTPDANS_STOR:
		ftps->AnsToCmd = FTPDANS_OK226;
		uip_len = ftps->count = strlen(okCode226);
		strncpy((char*) (uip_appdata),(char*)(okCode226),uip_len);
		ftps->IsCmdWD  = FALSE;
		ftps->Status   = FTPDSTS_SENDING_ENDDATACTLANS;
		ftps->other    = NULL;
		debugf("done\r\n");
		break;
	    default: break;
	    }
	}
    }
}

// test testt test
unsigned char TSTrtx;
//END
static void _retrasmit(void)
{
    TSTrtx++;
}

static void _ack(void)
{
    struct ftpd_state *ftps = (struct ftpd_state *)(&uip_conn->appstate);
    switch (ftps->Status) {
    case FTPDSTS_SENDING_CTLANS:
	switch (ftps->AnsToCmd) {
	case FTPDANS_LIST:
	case FTPDANS_RETR:
	case FTPDANS_STOR:
	    ftps->Status = FTPDSTS_PREP_FTPDATA;
	    if (ftps->other)
	    {
		ftps->other->Status  = FTPDSTS_PREP_FTPDATA;
		uIP_request_poll(ftps->other->uip_conn);
	    }
	    uip_len = 0;
	    break;
	default :
	    ftps->Status   = FTPDSTS_WAITFORCMD;
	    ftps->RecvCmd  = FTPDREC_NONE;
	    ftps->AnsToCmd = FTPDANS_NONE;
	    uip_len = 0;
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
    _poll();
}

static void _senddata(void)
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

//////////////////////////////////////////////////////////////////////////////////////////
// DATA CONNECTION //
//////////////////////////////////////////////////////////////////////////////////////////
static void _close_data(void);

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

static void _connect_data(void)
{
    struct ftpd_state *ftps = (struct ftpd_state *)(&uip_conn->appstate);
    ftps->count    = 0;
    ftps->Status   = FTPDSTS_NONE;
    ftps->RecvCmd  = FTPDREC_NONE;
    ftps->uip_conn = uip_conn;
    uip_len = 0;

    if (waiting_for_data_con != NULL)
    {
	waiting_for_data_con->other = ftps;
	ftps->other = waiting_for_data_con;
    }
}

static void generate_file_list(char *path)
{
    DIR dir;
    FILINFO fno;
    char *fn;
    FRESULT result;
#if _USE_LFN
    static char lfn[_MAX_LFN + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof(lfn);
#endif
    result = f_opendir (&dir, path);
    if (result != FR_OK)
    {
	uip_len = sprintf((char *)uip_appdata, "Failed opendir %d\r\n", result);
	return;
    }
    uip_len = 0;
    for (;;)
    {
	result = f_readdir(&dir, &fno);
	if (result != FR_OK || fno.fname[0] == 0) break;
	if (fno.fname[0] == '.') continue;
#if _USE_LFN
	fn = *fno.lfname ? fno.lfname : fno.fname;
#else
	fn = fno.fname;
#endif
	uip_len += snprintf(uip_appdata + uip_len, uip_mss() - uip_len,
			    "%crw-r--r--  1 0 0 %6lu Jan 1 2007 %s\r\n",
			    (fno.fattrib & AM_DIR) ? 'd' : '-',
			    (unsigned long)fno.fsize, fn);
    }
	
    if (uip_len == 0)
    {
	_close_data();
    }
}

static void _poll_data(void)
{
    struct ftpd_state *ftps = (struct ftpd_state *)(&uip_conn->appstate);

    switch (ftps->Status) {
    case FTPDSTS_PREP_FTPDATA:
	switch (ftps->RecvCmd) {
	case FTPDREC_LIST:
	    generate_file_list(ftps->other->cwd);
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

static void _retrasmit_data(void)
{
    struct ftpd_state *ftps = (struct ftpd_state *)(&uip_conn->appstate);
    switch (ftps->Status) {
    case FTPDSTS_SENDING_FTPDATA:
	switch (ftps->RecvCmd) {
	case FTPDREC_LIST:
	    generate_file_list(ftps->other->cwd);
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

static void _newdata_data(void)
{
    struct ftpd_state *ftps = (struct ftpd_state *)(&uip_conn->appstate);
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

static void _ack_data(void)
{
    struct ftpd_state *ftps = (struct ftpd_state *)(&uip_conn->appstate);
    switch (ftps->Status) {
    case FTPDSTS_SENDING_FTPDATA:
	switch (ftps->RecvCmd) {
	case FTPDREC_LIST:
	    _close_data();
	    break;
	case FTPDREC_RETR:
	    debugf("ack, left %d\r\n", ftps->count);
	    if (!ftps->count) {
		_close_data();
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

static void _senddata_data(void)
{
    if (uip_len > 0)
    {
	uip_send(uip_appdata,uip_len);
    }
}

static void _abort_data(void)
{

}

static void _timeout_data(void)
{
    uip_close();
}

static void _close_data(void)
{
    struct ftpd_state *ftps = (struct ftpd_state *)(&uip_conn->appstate);
    switch (ftps->Status)
    {
    case FTPDSTS_PREP_FTPDATA:
    case FTPDSTS_RECV_DATA:
    case FTPDSTS_SENDING_FTPDATA:
	debugf("close file\r\n");
	f_close(&file);
	break;
    }
    uip_close();
    ftps->Status = FTPDSTS_SENT_FTPDATA;
    if (ftps->other->Status == FTPDSTS_PREP_FTPDATA)
	ftps->other->Status  = ftps->Status;
    uIP_request_poll(ftps->other->uip_conn);
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
    if(uip_poll() || uip_connected()) {
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
