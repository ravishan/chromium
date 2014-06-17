// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/environment/environment.h"

#include "mojo/public/cpp/environment/lib/default_logger_internal.h"
#include "mojo/public/cpp/utility/run_loop.h"

namespace mojo {

Environment::Environment() {
  SetMinimumLogLevel(MOJO_LOG_LEVEL_INFO);
  RunLoop::SetUp();
}

Environment::~Environment() {
  RunLoop::TearDown();
}

// static
void Environment::SetMinimumLogLevel(MojoLogLevel minimum_log_level) {
  internal::SetMinimumLogLevel(minimum_log_level);
}

}  // namespace mojo
