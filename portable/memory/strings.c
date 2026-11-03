#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include <errors.h>
#include <memory.h>
#include <files.h>

addressCache_t stringCache;


// allocates a new string `s` with content `buf`
string_t newstring(c_string_t buf) {
	size_t len = strlen(buf);
	string_t s;
	s.len = len;
	s.buf = (char*)memalloc(len+1);
	memcpy(s.buf, buf, len+1);
	cacheAddress(&stringCache, s.buf);
	return s;
}
// allocates a new string `s` with content `buf`
string_t newstring_len(size_t len) {
	string_t s;
	s.len = len;
	s.buf = (char*)memalloc(len+1);
	memset(s.buf, 0, len+1);
	cacheAddress(&stringCache, s.buf);
	return s;
}
// allocates a new string `s` with content `buf`
string_t newstring_substr(c_string_t buf, size_t maxLen) {
	size_t len = __min(strlen(buf), maxLen);
	string_t s;
	s.len = len;
	s.buf = (char*)memalloc(len+1);
	memcpy(s.buf, buf, len+1);
	cacheAddress(&stringCache, s.buf);
	return s;
}

// delete `s` from the string cache and memfree its memory
void delstring(string_t s) {
	removeAddress(&stringCache, s.buf);
	memfree(s.buf);
}

// reallocates `s` and appends `buf`
void stringcat_buf(string_t *s, c_string_t source) {
	stringcat_str(s, newstring_t(source, strlen(source)));
}

// reallocates `s` and appends `b`
void stringcat_str(string_t* s, string_t source) {
	string_t old = *s;
	string_t new;
	new.len = old.len + source.len;
	new.buf = (char*)memalloc(new.len+1);
	cacheAddress(&stringCache, new.buf);

	memcpy(new.buf,              old.buf,    old.len);
	memcpy(new.buf + old.len, source.buf, source.len);
	
	new.buf[new.len] = 0;

	delstring(old);

	*s = new;
}

void stringprintf(string_t* s, c_string_t fmt, ...) {

}

// allocates a new string `data` and writes the file dat at `path` into it
string_t newstring_readFile(c_string_t path) {
	string_t s;
	error_t err = fsReadFile(path, &s.buf, &s.len);
	
	if (err) return null_string;

	cacheAddress(&stringCache, s.buf);
	return s;
}