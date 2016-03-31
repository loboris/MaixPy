/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>

#include "py/nlr.h"
#include "py/compile.h"
#include "py/runtime0.h"
#include "py/runtime.h"
#include "py/stackctrl.h"
#include "py/mphal.h"
#include "py/gc.h"
#include "lib/utils/pyexec.h"
#include "gccollect.h"
#include "user_interface.h"

STATIC char heap[15360];

STATIC void mp_reset(void) {
    mp_stack_set_top((void*)0x40000000);
    mp_stack_set_limit(8192);
    mp_hal_init();
    gc_init(heap, heap + sizeof(heap));
    mp_init();
    mp_obj_list_init(mp_sys_path, 0);
    mp_obj_list_init(mp_sys_argv, 0);
    #if MICROPY_VFS_FAT
    memset(MP_STATE_PORT(fs_user_mount), 0, sizeof(MP_STATE_PORT(fs_user_mount)));
    #endif
    MP_STATE_PORT(mp_kbd_exception) = mp_obj_new_exception(&mp_type_KeyboardInterrupt);
    MP_STATE_PORT(term_obj) = MP_OBJ_NULL;
#if MICROPY_MODULE_FROZEN
    pyexec_frozen_module("boot");
#endif
}

void soft_reset(void) {
    mp_hal_stdout_tx_str("PYB: soft reboot\r\n");
    mp_hal_delay_us(10000); // allow UART to flush output
    mp_reset();
    pyexec_event_repl_init();
}

void init_done(void) {
    uart_task_init();
    mp_reset();
    mp_hal_stdout_tx_str("\r\n");
    pyexec_event_repl_init();
    dupterm_task_init();
}

void user_init(void) {
    system_init_done_cb(init_done);
}

mp_lexer_t *fat_vfs_lexer_new_from_file(const char *filename);
mp_import_stat_t fat_vfs_import_stat(const char *path);

mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
    #if MICROPY_VFS_FAT
    return fat_vfs_lexer_new_from_file(filename);
    #else
    (void)filename;
    return NULL;
    #endif
}

mp_import_stat_t mp_import_stat(const char *path) {
    #if MICROPY_VFS_FAT
    return fat_vfs_import_stat(path);
    #else
    (void)path;
    return MP_IMPORT_STAT_NO_EXIST;
    #endif
}

mp_obj_t mp_builtin_open(uint n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);

void mp_keyboard_interrupt(void) {
    MP_STATE_VM(mp_pending_exception) = MP_STATE_PORT(mp_kbd_exception);
}

void nlr_jump_fail(void *val) {
    printf("NLR jump failed\n");
    for (;;) {
    }
}

//void __assert(const char *file, int line, const char *func, const char *expr) {
void __assert(const char *file, int line, const char *expr) {
    printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    for (;;) {
    }
}
