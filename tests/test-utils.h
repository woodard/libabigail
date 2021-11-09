// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.

#ifndef __TEST_UTILS_H__
#define __TEST_UTILS_H__

#include "config.h"
#include <string>

#define BRIGHT_YELLOW_COLOR "\e[1;33m"
#define BRIGHT_RED_COLOR "\e[1;31m"
#define DEFAULT_TERMINAL_COLOR "\033[0m"

namespace abigail
{
namespace tests
{

const char* get_src_dir();
const char* get_build_dir();

}//end namespace tests
}//end namespace abigail
#endif //__TEST_UTILS_H__
