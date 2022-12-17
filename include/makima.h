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

int makima_gateway(char *token, int intents,
                   int shard_i, int shard_c, int in, int out);

#endif
