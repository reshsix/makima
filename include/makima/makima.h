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

bool makima_run(char *token,
                bool (*on_message)(const char *, uint64_t, uint64_t, uint64_t));

#endif
