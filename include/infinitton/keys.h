/*
 * keys.h
 *
 * Created 2019-10-05 by James Magahern <james@magahern.com>
 */

#pragma once

/*
 * Layout of keys is the following on my device:
 * 
 *  --------------
 *  | 0 | 5 | 10 |
 *  --------------
 *  | 1 | 6 | 11 |
 *  --------------
 *  | 2 | 7 | 12 |
 *  --------------
 *  | 3 | 8 | 13 |
 *  --------------
 *  | 4 | 9 | 14 |
 *  --------------
 */

typedef enum {
    INF_KEY_CLEARED = 0,

    INF_KEY_0   = (1 << 0),
    INF_KEY_1   = (1 << 1),
    INF_KEY_2   = (1 << 2),
    INF_KEY_3   = (1 << 3),
    INF_KEY_4   = (1 << 4),
    INF_KEY_5   = (1 << 5),
    INF_KEY_6   = (1 << 6),
    INF_KEY_7   = (1 << 7),
    INF_KEY_8   = (1 << 8),
    INF_KEY_9   = (1 << 9),
    INF_KEY_10  = (1 << 10),
    INF_KEY_11  = (1 << 11),
    INF_KEY_12  = (1 << 12),
    INF_KEY_13  = (1 << 13),
    INF_KEY_14  = (1 << 14),
} infkey_t;

const unsigned int INF_NUM_KEYS = 15;

static inline infkey_t infkey_num_to_key (int num)
{
    infkey_t key = INF_KEY_CLEARED;
    if (num >= 0 && num < INF_NUM_KEYS) {
        key = (1 << num);
    }

    return key;
}

static inline int infkey_to_key_num (infkey_t key)
{
    if (key == INF_KEY_CLEARED) return -1;

    int num = 0;
    for (; key != 0; key >>= 1) { num++; };

    return num - 1;
}
