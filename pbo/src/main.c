/*
 * Copyright 2025 Aleksa Radomirovic
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <argp.h>

#include "mode/mode.h"
#include "pbo.h"

static enum mode {
    MODE_NULL,
    MODE_LIST,
} mode = MODE_NULL;

static const char *pbo_file_path = NULL;

static const struct argp_option args_opts[] = {
    { NULL, 0, NULL, 0, "Operating modes:", 1},
    { "list", 't', NULL, 0, "List contents of PBO", 0 },

    { NULL, 0, NULL, 0, "Common options:", 2},
    { "file", 'f', "PBO", 0, "Specify PBO file", 0 },
    { "pbo", 0, NULL, OPTION_ALIAS, NULL, 0 },

    { NULL, 0, NULL, 0, "General options:", -1 },
    { 0 }
};

static int args_parse(int key, char *arg, struct argp_state *state) {
    int status;

    switch(key) {
        case 't':
            if(mode != MODE_NULL) {
                argp_error(state, "mode already specified");
            }
            mode = MODE_LIST;
            break;

        case 'f':
            if(pbo_file_path != NULL) {
                argp_error(state, "pbo file already specified");
            }
            pbo_file_path = arg;
            break;
        
        case ARGP_KEY_ARG:
            switch(mode) {
                case MODE_NULL:
                    argp_error(state, "operating mode not defined");
                    break;

                case MODE_LIST:
                    argp_error(state, "extra arguments unused by list mode");
                    break;
            }

            break;
        case ARGP_KEY_SUCCESS:
            switch(mode) {
                case MODE_NULL:
                    argp_error(state, "operating mode not defined");
                    break;

                case MODE_LIST:
                    if(pbo_file_path == NULL) {
                        argp_error(state, "pbo file not specified");
                    }
                
                    status = pbo_mode_list(pbo_file_path);
                    if(status != 0) {
                        argp_failure(state, status, status, "failed to list contents of %s", pbo_file_path);
                    }
                    break;
            }

            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static const struct argp args_info = {
    .options = args_opts,
    .parser = args_parse,
};

int main(int argc, char **argv) {
    int status;

    status = argp_parse(&args_info, argc, argv, 0, NULL, NULL);
    if(status != 0) {
        return status;
    }

    return 0;
}
