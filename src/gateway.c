/*
This file is part of makima.

Makima is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published
by the Free Software Foundation, version 3.

Makima is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with makima. If not, see <https://www.gnu.org/licenses/>.
*/

#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include <curl/curl.h>
#include <json-c/json.h>

/* Type definitions */

enum status
{
    STATUS_OK,
    STATUS_ERROR,
    STATUS_FATAL
};

enum message
{
    MESSAGE_WARN,
    MESSAGE_ERROR,
    MESSAGE_FATAL
};

struct gateway
{
    CURL *curl;

    char *token;
    char *url;
    char *agent;

    char *session;

    int64_t seq;
    bool ack;

    int interval;

    char *inbuf;
    size_t inbuf_c;
    size_t inbuf_s;
    bool partial;

    bool killed;
    bool reconnect;
    int close_code;

    pthread_t hb;
    bool hb_active;

    enum status status;
    char tag[64];

    bool (*on_message)(const char *, const char *);
};

/* Auxiliar functions */

static void
sleep_ms(int ms)
{
    #if _POSIX_C_SOURCE >= 199309L
    struct timespec ts = {.tv_sec = ms / 1000,
                          .tv_nsec = (ms % 1000) * 1000000};
    nanosleep(&ts, NULL);
    #else
    if (ms >= 1000)
        sleep(ms / 1000);
    usleep((ms % 1000) * 1000);
    #endif
}

/* General gateway functions */

static void
message(struct gateway *g, enum message type, char *string)
{
    fprintf(stderr, "makima_gateway [%s]: ", (g->tag[0] != '\0') ?
                                              g->tag : "?");
    switch (type)
    {
        case MESSAGE_WARN:
            fprintf(stderr, "warning: ");
            break;
        case MESSAGE_ERROR:
            fprintf(stderr, "error: ");
            break;
        case MESSAGE_FATAL:
            fprintf(stderr, "fatal: ");
            break;
    }
    fprintf(stderr, "%s\n", string);
}

static bool
reconnect(struct gateway *g)
{
    if (g->hb_active)
    {
        pthread_cancel(g->hb);
        g->hb_active = false;
    }
    g->killed = true;

    return false;
}

static bool
die(struct gateway *g, enum status status)
{
    if (status > g->status)
        g->status = status;

    g->reconnect = false;
    return reconnect(g);
}

static bool
event(struct gateway *g, int op, struct json_object *d)
{
    bool ret = true;

    struct json_object *data = json_object_new_object();
    json_object_object_add(data, "op", json_object_new_int(op));
    json_object_object_add(data, "d", d);

    size_t sent = 0;
    const char *str = json_object_to_json_string(data);
    size_t len = strlen(str);
    if (curl_ws_send(g->curl, str, len, &sent, 0, CURLWS_TEXT) != CURLE_OK)
    {
        message(g, MESSAGE_WARN, "An event was not sent, reconnecting");
        ret = reconnect(g);
    }

    json_object_put(data);

    return ret;
}

/* Parsing and specific gateway functions */

static void
ready(struct gateway *g, struct json_object *d)
{
    struct json_object *user = NULL;
    json_object_object_foreach(d, key, val)
    {
        if (strcmp(key, "resume_gateway_url") == 0)
        {
            free(g->url);
            g->url = strdup(json_object_get_string(val));
        }
        else if (strcmp(key, "session_id") == 0)
        {
            free(g->session);
            g->session = strdup(json_object_get_string(val));
        }
        else if (strcmp(key, "user") == 0)
        {
            user = val;
        }
    }

    bool tag_fail = true;
    if (user != NULL)
    {
        struct json_object *username = NULL;
        struct json_object *discriminator = NULL;
        if (json_object_object_get_ex(user, "username",
                                      &username) &&
            json_object_object_get_ex(user, "discriminator",
                                      &discriminator))
        {
            sprintf(g->tag, "%s#%s",
                    json_object_get_string(username),
                    json_object_get_string(discriminator));

            message(g, MESSAGE_WARN, "Ready");
            tag_fail = false;
        }
    }

    if (tag_fail)
        message(g, MESSAGE_WARN, "Ready, but failed to get account tag");
}

static bool
heartbeat(struct gateway *g)
{
    bool ret = true;

    struct json_object *d = (g->seq > 0) ? json_object_new_int(g->seq) : NULL;
    ret = event(g, 1, d);

    return ret;
}

static void *
heartbeat_thread(void *arg)
{
    struct gateway *g = arg;

    srand(time(NULL));
    sleep_ms(g->interval * rand() / RAND_MAX);
    while (true)
    {
        g->ack = false;
        heartbeat(g);
        sleep_ms(5000);
        if (!g->ack)
        {
            message(g, MESSAGE_ERROR, "Heartbeat ack not received");
            die(g, STATUS_ERROR);
        }
        sleep_ms(g->interval - 5000);
    }
    g->hb_active = false;

    pthread_exit(0);
}

static bool
identify(struct gateway *g, struct json_object *d)
{
    bool ret = true;

    if (g->session != NULL)
    {
        struct json_object *data = json_object_new_object();
        json_object_object_add(data, "seq", json_object_new_int(g->seq));
        json_object_object_add(data, "token",
                               json_object_new_string(g->token));
        json_object_object_add(data, "session_id",
                               json_object_new_string(g->session));

        ret = event(g, 6, data);

        json_object_put(data);
    }
    else
    {
        struct json_object *shards = json_object_new_array();
        json_object_array_add(shards, json_object_new_int(0));
        json_object_array_add(shards, json_object_new_int(1));

        struct json_object *prop = json_object_new_object();
        json_object_object_add(prop, "os", json_object_new_string("unix"));
        json_object_object_add(prop, "browser",
                               json_object_new_string("makima"));
        json_object_object_add(prop, "device",
                               json_object_new_string("makima"));

        struct json_object *data = json_object_new_object();
        json_object_object_add(data, "properties", prop);
        json_object_object_add(data, "shards", shards);
        json_object_object_add(data, "token", json_object_new_string(g->token));

        uint32_t intents = (1 << 0) | (1 << 9) | (1 << 15);
        json_object_object_add(data, "intents", json_object_new_int(intents));

        ret = event(g, 2, data);
    }

    if (ret)
    {
        json_object_object_foreach(d, key, val)
        {
            if (strcmp(key, "heartbeat_interval") == 0)
                g->interval = json_object_get_int(val);
        }

        if (g->interval > 0)
        {
            g->hb_active = true;
            pthread_create(&g->hb, NULL, heartbeat_thread, (void*)g);
        }
        else
        {
            message(g, MESSAGE_ERROR,
                    "Couldn't determine heartbeat interval");
            ret = die(g, STATUS_ERROR);
        }
    }

    return ret;
}

static bool
parse_message(struct gateway *g, struct json_object *d)
{
    bool ret = false;

    if (d != NULL)
    {
        struct json_object *author = NULL;
        const char *content = NULL;
        json_object_object_foreach(d, key, val)
        {
            if (strcmp(key, "author") == 0)
                author = val;
            else if (strcmp(key, "content") == 0)
                content = json_object_get_string(val);
        }

        const char *name = NULL;
        json_object_object_foreach(author, key2, val2)
        {
            if ((!name && strcmp(key2, "username")    == 0) ||
                          strcmp(key2, "global_name") == 0)
                name = json_object_get_string(val2);
        }

        ret = g->on_message(name, content);
    }

    return ret;
}

static bool
parse(struct gateway *g)
{
    bool ret = true;
    char *buf = g->inbuf;

    struct json_object *obj = json_tokener_parse(buf);
    if (obj != NULL)
    {
        int op = 0;
        int64_t s = 0;
        struct json_object *d = NULL;
        const char *t = NULL;
        json_object_object_foreach(obj, key, val)
        {
            if (strcmp(key, "op") == 0)
                op = json_object_get_int(val);
            else if (strcmp(key, "s") == 0)
                s = json_object_get_int64(val);
            else if (strcmp(key, "d") == 0)
                d = val;
            else if (strcmp(key, "t") == 0)
                t = json_object_get_string(val);
        }

        if (s != 0)
            g->seq = s;

        switch (op)
        {
            case 0:
                if (strcmp(t, "READY") == 0)
                    ready(g, d);
                else if (strcmp(t, "RESUMED") == 0)
                    message(g, MESSAGE_WARN, "Resumed");
                else if (strcmp(t, "MESSAGE_CREATE") == 0)
                    ret = parse_message(g, d);
                break;
            case 1:
                ret = heartbeat(g);
                break;
            case 7:
                message(g, MESSAGE_WARN,
                        "Received reconnect request, reconnecting");
                ret = reconnect(g);
                break;
            case 9:
                message(g, MESSAGE_WARN, "Invalid session");
                if (!json_object_get_boolean(d))
                {
                    free(g->session);
                    g->session = NULL;
                }
                ret = reconnect(g);
                break;
            case 10:
                ret = identify(g, d);
                break;
            case 11:
                g->ack = true;
                break;
        }

        json_object_put(obj);
    }
    else
        g->partial = true;

    return ret;
}

/* Main input and output functions */

static size_t
callback(void *content, size_t size, size_t nmemb, void *userp)
{
    bool ret = true;
    struct gateway *g = userp;

    if (g->inbuf == NULL)
    {
        g->inbuf_s = 1024;
        g->inbuf = malloc(g->inbuf_s);
    }

    size_t bytes = (size * nmemb) + 1;
    if (g->partial)
        bytes += g->inbuf_c - 1;

    if (bytes > g->inbuf_s)
    {
        uint8_t bits = 0;
        for (size_t need = bytes; need > 0; need >>= 1)
            bits++;
        g->inbuf_s = 1 << bits;
        g->inbuf = realloc(g->inbuf, g->inbuf_s);
    }

    if (g->inbuf != NULL)
    {
        if (g->partial)
            memcpy(&(g->inbuf[g->inbuf_c - 1]), content,
                   bytes - g->inbuf_c + 1);
        else
            memcpy(g->inbuf, content, bytes - 1);
        g->inbuf[bytes - 1] = '\0';
        g->inbuf_c = bytes;

        if (g->partial || (bytes > 1 && g->inbuf[0] == '{'))
        {
            g->partial = false;
            ret = parse(g);
        }
        else if (bytes >= 3)
        {
            uint8_t *code = (uint8_t*)g->inbuf;
            g->close_code = code[1] + (code[0] << 8);
            ret = false;

            g->reconnect = false;
            switch (g->close_code)
            {
                case 1000: case 1007: case 1008: case 1009:
                case 1011: case 1012: case 1013: case 1014:
                case 4000: case 4001: case 4002: case 4003:
                case 4005: case 4007: case 4008: case 4009:
                    g->reconnect = true;
            }

            char err[128] = {0};
            sprintf(err, "Websocket error %d\n", g->close_code);
            message(g, (g->reconnect) ? MESSAGE_WARN : MESSAGE_ERROR, err);
        }
        else
        {
            message(g, MESSAGE_WARN, "Corrupted data");
            ret = false;
        }
    }
    else
    {
        message(g, MESSAGE_FATAL, "Allocation failed");
        ret = die(g, STATUS_FATAL);
    }

    return (ret) ? size * nmemb : 0;
}

/* Initialization and main loop */

extern bool
makima_run(char *token, bool (*on_message)(const char *, const char *))
{
    struct gateway g = {.token = token, .on_message = on_message};

    g.reconnect = true;
    while (g.reconnect)
    {
        curl_global_init(CURL_GLOBAL_ALL);
        g.curl = curl_easy_init();

        if (!g.url)
            g.url = strdup("wss://gateway.discord.gg/?v=10&encoding=json");

        curl_easy_setopt(g.curl, CURLOPT_URL, g.url);
        curl_easy_setopt(g.curl, CURLOPT_WRITEFUNCTION, callback);
        curl_easy_setopt(g.curl, CURLOPT_WRITEDATA, &g);
        curl_easy_setopt(g.curl, CURLOPT_USERAGENT, g.agent);
        curl_easy_setopt(g.curl, CURLOPT_VERBOSE, 0);
        g.killed = false;

        CURLcode res = curl_easy_perform(g.curl);
        if (!g.killed && res != CURLE_OK)
        {
            if (res != CURLE_WRITE_ERROR)
            {
                char err[256] = {0};
                sprintf(err, "curl: %s", curl_easy_strerror(res));
                message(&g, MESSAGE_ERROR, err);
            }
            die(&g, STATUS_ERROR);
        }
        curl_easy_cleanup(g.curl);
        curl_global_cleanup();

        if (g.reconnect)
            message(&g, MESSAGE_WARN, "Reconnecting");
    }

    message(&g, MESSAGE_WARN, "Shuting down");
    if (g.hb_active)
        pthread_cancel(g.hb);
    free(g.inbuf);
    free(g.url);

    return g.status == STATUS_OK;
}
