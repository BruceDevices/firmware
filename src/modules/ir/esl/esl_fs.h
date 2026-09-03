#pragma once

#include "esl_store.h"

bool esl_fs_load_session(EslSession *s);
bool esl_fs_save_settings(const EslSession *s);
bool esl_fs_save_targets(const EslSession *s);
bool esl_fs_save_recents(const EslSession *s);
