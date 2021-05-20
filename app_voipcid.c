/*
 * Asterisk -- An open source telephony toolkit.
 *
 * Copyright (C) 1999 - 2005, Digium, Inc.
 *
 * Mark Spencer <markster@digium.com>
 *
 * See http://www.asterisk.org for more information about
 * the Asterisk project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

/*! \file app_voipcid.c
 *
 * \brief Post callerid information to api service
 *
 * \author Fabio Silvestri <fabio@rup.com.br>
 * 
 * \ingroup applications
 */

#include <asterisk.h>

ASTERISK_FILE_VERSION(__FILE__, "$Revision$")

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <ctype.h>
#include <curl/curl.h>

#include "asterisk/file.h"
#include <asterisk/logger.h>
#include <asterisk/channel.h>
#include <asterisk/pbx.h>
#include <asterisk/module.h>

#define SIZE 512
#define AST_MODULE "VOIPCId"
#define CONFIG_FILE_NAME "voipcid.conf"

static char *app="VOIPCId";

static char *http_url;
static char *domain;

static int httpPost(char *extension,char *callingnumber,char *callingname) {
    char json[SIZE];

    memset(&json,0,sizeof(json));
    sprintf(json, "{\"exten\":\"%s\",\"calleridnum\":\"%s\",\"calleridname\":\"%s\",\"domain\":\"%s\"}",extension,callingnumber,callingname,domain);

    ast_log(LOG_NOTICE,"Sending: '%s' to: %s\n",json,http_url);

    CURL *curl;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, (char *)http_url);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 5000);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "asterisk-libcurl-agent/1.0");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);

        /* SSL Options */
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER , 1);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST , 1);

        int res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            ast_log(LOG_ERROR, "Failed to execute CURL: %d\n", res);
        }
        curl_easy_cleanup(curl);
    } else {
        ast_log(LOG_ERROR, "Cannot allocate curl structure\n");
    }

    return 0;
}

static int fifoclient(struct ast_channel *chan, const char *data) {
    int res=0;
    char *extension,*callingnumber,*callingname;
    struct ast_module_user *u;
    
    u=ast_module_user_add(chan);
    extension = ast_strdupa(S_OR(ast_channel_macroexten(chan), ast_channel_exten(chan)));
    callingnumber=ast_strdupa(ast_channel_caller(chan)->id.number.str);
    callingname=ast_strdupa(ast_channel_caller(chan)->id.name.str);
    ast_log(LOG_NOTICE, "extension '%s <%s>' calling '%s@%s'\n",callingname,callingnumber,extension,domain);

    ast_autoservice_start(chan);

    /* Do our thing here */
    res=httpPost(extension,callingnumber,callingname);

    ast_autoservice_stop(chan);

    ast_module_user_remove(u);

    return res;
}

static int load_config(int reload) {
    int res = 0;
    char *cat = NULL;
    struct ast_variable *v;
    struct ast_config *cfg;
    struct ast_flags config_flags = { CONFIG_FLAG_NOCACHE };

    if (!(cfg = ast_config_load(CONFIG_FILE_NAME, config_flags))) {
        ast_log(LOG_ERROR, "config '%s' not found\n",CONFIG_FILE_NAME);
        return res;
    } else if (cfg == CONFIG_STATUS_FILEUNCHANGED) {
        ast_log(LOG_ERROR, "config '%s'unchanged. Aborting.\n",CONFIG_FILE_NAME);
        return res;

    } else if (cfg == CONFIG_STATUS_FILEINVALID) {
        ast_log(LOG_ERROR, "config '%s' is in an invalid format. Aborting.\n",CONFIG_FILE_NAME);
        return res;
    }

    while ((cat = ast_category_browse(cfg, cat))) {
        if (strcmp(cat,"VOIPCId")==0) {
            v = ast_variable_browse(cfg, cat);
            while (v) {
                if (!strcasecmp(v->name, "http_url")) {
                    http_url=ast_strdup(v->value);
                }
                if (!strcasecmp(v->name, "domain")) {
                    domain=ast_strdup(v->value);
                }
                v = v->next;
            }
        }
    }
    ast_config_destroy(cfg);

    return AST_MODULE_LOAD_SUCCESS;
}

static int load_module(void) {
    int res;

    res = load_config(0);
    res |= ast_register_application_xml(app, fifoclient);

    return res;
}

static int unload_module(void) {
    int res;

    res=ast_unregister_application(app);

    ast_module_user_hangup_all();

    return res;
}

static int reload(void) {
    if (load_config(1))
        return AST_MODULE_LOAD_DECLINE;

    return AST_MODULE_LOAD_SUCCESS;
}

AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_DEFAULT, "VOIPCId() application",
        .load = load_module,
        .unload = unload_module,
        .reload = reload,
        );
