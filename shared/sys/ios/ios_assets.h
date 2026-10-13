#pragma once

#include <stddef.h>

int IOS_ResolveBaseDir( const char *documents, char *out, size_t outSize );
int IOS_ZipHasEntry( const char *zipPath, const char *name );
int IOS_ValidateAssets( const char *baseDir, const char *cfgName );
