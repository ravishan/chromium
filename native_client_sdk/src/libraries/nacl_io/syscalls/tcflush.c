/* Copyright 2013 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. */

#include <sys/types.h>
/*
 * Include something that will define __BIONIC__, then wrap the entire file
 * in this #if, so this file will be compiled on a non-bionic build.
 */

#if !defined(__BIONIC__)

#include "nacl_io/kernel_intercept.h"
#include "nacl_io/ostermios.h"

int tcflush(int fd, int queue_selector) {
  return ki_tcflush(fd, queue_selector);
}

#endif /* #if !defined(__BIONIC_) */