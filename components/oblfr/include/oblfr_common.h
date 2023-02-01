// Copyright 2023 Justin Hammond
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef OBLFR_COMMON_H
#define OBLFR_COMMON_H

#include "sdkconfig.h"
#include <assert.h>

typedef enum {
    OBLFR_OK = 0,
    OBLFR_ERR_ERROR = 1,
    OBLFR_ERR_TIMEOUT = 2,
    OBLFR_ERR_INVALID = 3, /* invalid arguments */
    OBLFR_ERR_NORESC = 4,   /* no resource or resource temperary unavailable */
    OBLFR_ERR_NOMEM = 5,    /* no memory */
    OBLFR_ERR_NOTSUPPORTED = 6
} OBLFR_Err;

typedef int oblfr_err_t;



#endif