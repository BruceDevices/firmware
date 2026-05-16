/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef _STORAGE_PORT_H
#define _STORAGE_PORT_H

#include <stdio.h>

typedef enum {
    READONLY,
    READWRITE
} open_mode_t;

typedef uint32_t cache_handle_t;
typedef int (*open_cache)(const char *namespace_name, open_mode_t open_mode, cache_handle_t *out_handle);
typedef void (*close_cache)(cache_handle_t handle);
typedef int (*erase_all_cache)(cache_handle_t handle);
typedef int (*write_cache)(cache_handle_t handle, const char *key, const void* value, size_t length);
typedef int (*read_cache)(cache_handle_t handle, const char *key, void* out_value, size_t *length);

struct cache_fn_mapping {
    open_cache open;
    close_cache close;
    erase_all_cache erase_all;
    write_cache write;
    read_cache read;
};

struct cache_fn_mapping link_storage_fn(void *storage_cb);

#endif
