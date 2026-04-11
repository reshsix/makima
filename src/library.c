#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

#include <curl/curl.h>
#include <json-c/json.h>
#include <makima/makima.h>

struct makima
{
    void *ctx;
    struct makima_cb cb;

    char token[512];

    CURL *curl;
    char *inbuf;
    size_t inbuf_c;
    size_t inbuf_s;
    bool partial;
};

/* Initialization and clean-up */

struct makima *
makima_new(void *ctx, struct makima_cb cb)
{
    struct makima *ret = calloc(1, sizeof(struct makima));

    if (ret)
    {
        ret->ctx = ctx;
        ret->cb  = cb;

        ret->curl = curl_easy_init();
    }

    return ret;
}

struct makima *
makima_del(struct makima *m)
{
    if (m)
    {
        curl_easy_cleanup(m->curl);
        free(m->inbuf);
    }
    free(m);

    return NULL;
}

/* REST calls and main loop */

static size_t
callback(void *content, size_t size, size_t nmemb, void *userp)
{
    bool ret = true;

    struct makima *m = userp;

    if (m->inbuf == NULL)
    {
        m->inbuf_s = 1024;
        m->inbuf = malloc(m->inbuf_s);
    }

    size_t bytes = (size * nmemb) + 1;
    if (m->partial)
        bytes += m->inbuf_c - 1;

    if (bytes > m->inbuf_s)
    {
        uint8_t bits = 0;
        for (size_t need = bytes; need > 0; need >>= 1)
            bits++;
        m->inbuf_s = 1 << bits;
        m->inbuf = realloc(m->inbuf, m->inbuf_s);
    }

    if (m->inbuf != NULL)
    {
        if (m->partial)
            memcpy(&(m->inbuf[m->inbuf_c - 1]), content,
                   bytes - m->inbuf_c + 1);
        else
            memcpy(m->inbuf, content, bytes - 1);
        m->inbuf[bytes - 1] = '\0';
        m->inbuf_c = bytes;
    }
    else
        ret = false;

    m->partial = true;
    return (ret) ? size * nmemb : 0;
}

static bool
rest(struct makima *m, const char *mode, const char *point, const char *body)
{
    curl_easy_reset(m->curl);

    const char *agent = "DiscordBot (https://github.com/reshsix/makima, 1.0)";
    curl_easy_setopt(m->curl, CURLOPT_CUSTOMREQUEST, mode);
    curl_easy_setopt(m->curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(m->curl, CURLOPT_WRITEDATA, m);
    curl_easy_setopt(m->curl, CURLOPT_USERAGENT, agent);
    curl_easy_setopt(m->curl, CURLOPT_VERBOSE, 0);
    if (body)
        curl_easy_setopt(m->curl, CURLOPT_POSTFIELDS, body);

    char url[256] = {0};
    snprintf(url, sizeof(url), "https://discord.com/api/v10/%s", point);
    curl_easy_setopt(m->curl, CURLOPT_URL, url);

    struct curl_slist *h = NULL;

    char auth[512] = {0};
    snprintf(auth, sizeof(auth), "Authorization: Bot %s", m->token);
    h = curl_slist_append(h, auth);
    if (body)
        h = curl_slist_append(h, "Content-Type: application/json");
    curl_easy_setopt(m->curl, CURLOPT_HTTPHEADER, h);

    CURLcode res = curl_easy_perform(m->curl);
    m->partial = false;

    curl_slist_free_all(h);

    return (res == CURLE_OK);
}

extern bool
makima_author(struct makima *m,
              uint64_t author, uint64_t server, char *name, char *avatar)
{
    bool ret = true;

    char point[128] = {0};
    snprintf(point, sizeof(point), "users/%" SCNu64, author);

    ret = rest(m, "GET", point, NULL);
    if (ret)
    {
        const char *name2   = NULL;
        const char *avatar2 = NULL;

        struct json_object *obj = json_tokener_parse(m->inbuf);
        if (obj)
        {
            json_object_object_foreach(obj, key, val)
            {
                if (strcmp(key, "global_name") == 0)
                    name2 = json_object_get_string(val);
                else if (!name2 && strcmp(key, "username") == 0)
                    name2 = json_object_get_string(val);
                else if (strcmp(key, "avatar") == 0)
                    avatar2 = json_object_get_string(val);
            }

            if (name && name2)
                strncpy(name, name2, 32);
            if (avatar && avatar2)
                sprintf(avatar,
                        "https://cdn.discordapp.com/avatars/%" SCNu64 "/%s.png",
                        author, avatar2);
        }
        else
            ret = false;
        json_object_put(obj);
    }

    return ret;
}

extern bool
makima_channel(struct makima *m, uint64_t channel, char *name, bool *direct)
{
    bool ret = true;

    char point[128] = {0};
    snprintf(point, sizeof(point), "channels/%" SCNu64, channel);

    ret = rest(m, "GET", point, NULL);
    if (ret)
    {
        const char *name2 = NULL;
        int type = 0;

        struct json_object *obj = json_tokener_parse(m->inbuf);
        if (obj)
        {
            json_object_object_foreach(obj, key, val)
            {
                if (strcmp(key, "name") == 0)
                    name2 = json_object_get_string(val);
                else if (strcmp(key, "type") == 0)
                    type  = json_object_get_int(val);
            }

            if (name && name2)
                strncpy(name, name2, 100);
            if (direct)
                *direct = (type == 1 || type == 3);
        }
        else
            ret = false;
        json_object_put(obj);
    }

    return ret;
}

extern bool
makima_server(struct makima *m, uint64_t server, char *name, char *icon)
{
    bool ret = true;

    char point[128] = {0};
    snprintf(point, sizeof(point), "guilds/%" SCNu64 "/preview", server);

    ret = rest(m, "GET", point, NULL);
    if (ret)
    {
        const char *name2 = NULL;
        const char *icon2 = NULL;

        struct json_object *obj = json_tokener_parse(m->inbuf);
        if (obj)
        {
            json_object_object_foreach(obj, key, val)
            {
                if (strcmp(key, "name") == 0)
                    name2 = json_object_get_string(val);
                else if (strcmp(key, "icon") == 0)
                    icon2 = json_object_get_string(val);
            }

            if (name && name2)
                strncpy(name, name2, 100);
            if (icon && icon2)
                sprintf(icon,
                        "https://cdn.discordapp.com/icons/%" SCNu64 "/%s.png",
                        server, icon2);
        }
        else
            ret = false;
        json_object_put(obj);
    }

    return ret;
}

/* Event scheduler */

static bool
on_token(struct makima *m, const char *line)
{
    strcpy(m->token, line + 6);
    return true;
}

static bool
on_message(struct makima *m, const char *line)
{
    bool ret = false;

    uint64_t author, channel, server;
    int consumed = 0;
    if (sscanf(line, "MESSAGE %" SCNu64 " %" SCNu64 " %" SCNu64 " %n",
               &author, &channel, &server, &consumed) == 3)
        ret = m->cb.on_message(m, author, channel, server, line + consumed);

    return ret;
}

extern bool
makima_next(struct makima *m)
{
    bool ret = true;

    char line[512] = {0};
    uint16_t line_c = 0;
    for (int c = getc(stdin); c != EOF && c != '\n'; c = getc(stdin))
        line[line_c++] = c;
    line[line_c] = '\0';

    char command[32] = {0};
    sscanf(line, "%32s ", command);
    if (command[0] == '\0' || command[0] == ' ')
        ret = false;

    if (ret)
    {
        if (strcmp(command, "TOKEN") == 0)
            ret = on_token(m, line);
        else if (strcmp(command, "MESSAGE") == 0)
            ret = on_message(m, line);
    }

    return ret;
}
