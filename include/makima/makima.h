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

#ifndef MAKIMA_H
#define MAKIMA_H

#include <stdint.h>
#include <stdbool.h>

typedef struct makima makima;

struct makima_cb
{
    bool (*on_message)(struct makima *m,
                       uint64_t, uint64_t, uint64_t, uint64_t, const char *);
    bool (*on_reaction)(struct makima *m, bool,
                        uint64_t, uint64_t, uint64_t, uint64_t, const char *);
};

makima *makima_new(void *ctx, struct makima_cb cb);
makima *makima_del(makima *m);

bool makima_author(makima *m, uint64_t author, uint64_t server,
                   char *name, char *avatar);
bool makima_message(makima *m, uint64_t channel, uint64_t message,
                    uint64_t *author, char *content);
bool makima_channel(makima *m, uint64_t channel, char *name, bool *direct);
bool makima_server(makima *m, uint64_t server, char *name, char *icon);

bool makima_say(makima *m, uint64_t channel, char *content);
bool makima_react(struct makima *m, uint64_t channel,
                  uint64_t message, const char *emoji);

bool makima_next(makima *m);

#endif
