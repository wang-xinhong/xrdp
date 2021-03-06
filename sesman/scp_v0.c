/**
 * xrdp: A Remote Desktop Protocol server.
 *
 * Copyright (C) Jay Sorg 2004-2013
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 *
 * @file scp_v0.c
 * @brief scp version 0 implementation
 * @author Jay Sorg, Simone Fedele
 *
 */

#include "sesman.h"

extern struct config_sesman *g_cfg; /* in sesman.c */

/******************************************************************************/
void DEFAULT_CC
scp_v0_process(struct SCP_CONNECTION *c, struct SCP_SESSION *s)
{
    int display = 0;
    tbus data;
    struct session_item *s_item;
    int errorcode = 0 ;

    data = auth_userpass(s->username, s->password,&errorcode);

    if (s->type == SCP_GW_AUTHENTICATION)
    {
        /* this is just authentication in a gateway situation */
        /* g_writeln("SCP_GW_AUTHENTICATION message received"); */
        if (data)
        {
            if (1 == access_login_allowed(s->username))
            {
                /* the user is member of the correct groups. */
                scp_v0s_replyauthentication(c, errorcode);
                log_message(LOG_LEVEL_INFO, "Access permitted for user: %s",
                            s->username);
                /* g_writeln("Connection allowed"); */
            }
            else
            {
                scp_v0s_replyauthentication(c, 32+3); /* all first 32 are reserved for PAM errors */
                log_message(LOG_LEVEL_INFO, "Username okey but group problem for "
                            "user: %s", s->username);
                /* g_writeln("user password ok, but group problem"); */
            }
        }
        else
        {
            /* g_writeln("username or password error"); */
            log_message(LOG_LEVEL_INFO, "Username or password error for user: %s",
                        s->username);
            scp_v0s_replyauthentication(c, errorcode);
        }

        auth_end(data);
    }
    else if (data)
    {
        s_item = session_get_bydata(s->username, s->width, s->height,
                                    s->bpp, s->type, s->client_ip);

        if (s_item != 0)
        {
            display = s_item->display;

            if (0 != s->client_ip)
            {
                log_message( LOG_LEVEL_INFO, "++ reconnected session: username %s, "
                             "display :%d.0, session_pid %d, ip %s",
                             s->username, display, s_item->pid, s->client_ip);
            }
            else
            {
                log_message(LOG_LEVEL_INFO, "++ reconnected session: username %s, "
                            "display :%d.0, session_pid %d", s->username, display,
                            s_item->pid);
            }

            session_reconnect(display, s->username);
            auth_end(data);
            /* don't set data to null here */
        }
        else
        {
            LOG_DBG("pre auth");

            if (1 == access_login_allowed(s->username))
            {
                if (0 != s->client_ip)
                {
                    log_message(LOG_LEVEL_INFO, "++ created session (access granted): "
                                "username %s, ip %s", s->username, s->client_ip);
                }
                else
                {
                    log_message(LOG_LEVEL_INFO, "++ created session (access granted): "
                                "username %s", s->username);
                }

                if (SCP_SESSION_TYPE_XVNC == s->type)
                {
                    log_message( LOG_LEVEL_INFO, "starting Xvnc session...");
                    display = session_start(s->width, s->height, s->bpp, s->username,
                                            s->password, data, SESMAN_SESSION_TYPE_XVNC,
                                            s->domain, s->program, s->directory,
                                            s->client_ip);
                }
                else if (SCP_SESSION_TYPE_XRDP == s->type)
                {
                    log_message(LOG_LEVEL_INFO, "starting X11rdp session...");
                    display = session_start(s->width, s->height, s->bpp, s->username,
                                            s->password, data, SESMAN_SESSION_TYPE_XRDP,
                                            s->domain, s->program, s->directory,
                                            s->client_ip);					
				}
                else if (SCP_SESSION_TYPE_XORG == s->type)
                {
					/* type is SCP_SESSION_TYPE_XORG */
                    log_message(LOG_LEVEL_INFO, "starting Xorg session...");
                    display = session_start(s->width, s->height, s->bpp, s->username,
                                            s->password, data, SESMAN_SESSION_TYPE_XORG,
                                            s->domain, s->program, s->directory,
                                            s->client_ip);
                }
            }
            else
            {
                display = 0;
            }
        }

        if (display == 0)
        {
            auth_end(data);
            scp_v0s_deny_connection(c);
        }
        else
        {
            scp_v0s_allow_connection(c, display);
        }
    }
    else
    {
        scp_v0s_deny_connection(c);
    }
}
