/****************************************************************************
 |  (C) Copyright 2008 Novell, Inc. All Rights Reserved.
 |
 |  GPLv2: This program is free software; you can redistribute it
 |  and/or modify it under the terms of version 2 of the GNU General
 |  Public License as published by the Free Software Foundation.
 |
 |  This program is distributed in the hope that it will be useful,
 |  but WITHOUT ANY WARRANTY; without even the implied warranty of
 |  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 |  GNU General Public License for more details.
 +-------------------------------------------------------------------------*/

/* Copyright (C) 1999 Lucent Technologies */
/* Excerpted from 'The Practice of Programming' */
/* by Brian W. Kernighan and Rob Pike */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <eprintf.h>

#include <debug.h>
#include <style.h>

bool Stacktrace = TRUE;
static Cleanup_f Cleanup = NULL;

static void call_cleanup (void)
{
	Cleanup_f cleanup = Cleanup;

	Cleanup = NULL;		/* Prevent recursive loop of cleanup */
	if (cleanup) cleanup();
}

/* pr_display: print debug message */
void pr_display (const char *file, const char *func, int line, const char *fmt, ...)
{
	va_list args;

	fflush(stdout);
	if (getprogname() != NULL) {
		fprintf(stderr, "%s ", getprogname());
	}
	fprintf(stderr, "%s:%s<%d> ", file, func, line);
	if (fmt) {
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);

		if (fmt[0] != '\0' && fmt[strlen(fmt)-1] == ':') {
			fprintf(stderr, " %s<%d>", strerror(errno), errno);
		}
	}
	fprintf(stderr, "\n");
}

/* pr_fatal: print error message and exit */
void pr_fatal (const char *file, const char *func, int line, const char *fmt, ...)
{
	va_list args;

	fflush(stdout);
	fprintf(stderr, "Fatal ");
	if (getprogname() != NULL) {
		fprintf(stderr, "%s ", getprogname());
	}
	fprintf(stderr, "%s:%s<%d> ", file, func, line);
	if (fmt) {
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);

		if (fmt[0] != '\0' && fmt[strlen(fmt)-1] == ':') {
			fprintf(stderr, " %s<%d>", strerror(errno), errno);
		}
	}
	fprintf(stderr, "\n");
	if (Stacktrace) stacktrace_err();
	call_cleanup();
	exit(2); /* conventional value for failed execution */
}

/* pr_warn: print warning message */
void pr_warn (const char *file, const char *func, int line, const char *fmt, ...)
{
	va_list args;

	fflush(stdout);
	fprintf(stderr, "Warn ");
	if (getprogname() != NULL) {
		fprintf(stderr, "%s ", getprogname());
	}
	fprintf(stderr, "%s:%s<%d> ", file, func, line);
	if (fmt) {
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);

		if (fmt[0] != '\0' && fmt[strlen(fmt)-1] == ':') {
			fprintf(stderr, " %s<%d>", strerror(errno), errno);
		}
	}
	fprintf(stderr, "\n");
}

/* pr_usage: print usage message */
void pr_usage (const char *fmt, ...)
{
	va_list args;

	fflush(stdout);
	fprintf(stderr, "Usage: ");
	if (getprogname() != NULL) {
		fprintf(stderr, "%s ", getprogname());
	}
	if (fmt) {
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);
	}
	fprintf(stderr, "\n");
	exit(2);
}

/* eprintf: print error message and exit */
void eprintf (const char *fmt, ...)
{
	va_list args;

	fflush(stdout);
	if (getprogname() != NULL)
		fprintf(stderr, "%s: ", getprogname());

	if (fmt) {
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);

		if (fmt[0] != '\0' && fmt[strlen(fmt)-1] == ':')
			fprintf(stderr, " %s<%d>", strerror(errno), errno);
	}
	fprintf(stderr, "\n");
	call_cleanup();
	exit(2); /* conventional value for failed execution */
}

/* weprintf: print warning message */
void weprintf (const char *fmt, ...)
{
	va_list args;

	fflush(stdout);
	fprintf(stderr, "warning: ");
	if (getprogname() != NULL)
		fprintf(stderr, "%s: ", getprogname());
	if (fmt) {
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);
		if (fmt[0] != '\0' && fmt[strlen(fmt)-1] == ':')
			fprintf(stderr, " %s<%d>", strerror(errno), errno);
	}
	fprintf(stderr, "\n");
}

/* emalloc: malloc and report if error */
void *emalloc (size_t n)
{
	void *p;

	p = malloc(n);
	if (p == NULL)
		eprintf("malloc of %u bytes failed:", n);
	return p;
}

/* ezalloc: malloc, zero, and report if error */
void *ezalloc (size_t n)
{
	void *p;

	p = malloc(n);
	if (p == NULL)
		eprintf("malloc of %u bytes failed:", n);
	memset(p, 0, n);
	return p;
}

/* erealloc: realloc and report if error */
void *erealloc (void *vp, size_t n)
{
	void *p;

	p = realloc(vp, n);
	if (p == NULL)
		eprintf("realloc of %u bytes failed:", n);
	return p;
}

/* eallocpages: allocates n pages of specific size */
void *eallocpages (size_t npages, size_t size)
{
	void *p;
	int rc;

	rc = posix_memalign( &p, size, npages * size);
	if (rc) {
		eprintf("eallocpages failed %d", rc);
		return NULL;
	}
	return p;
}

/* estrdup: duplicate a string, report if error */
char *estrdup (const char *s)
{
	char *t;

	t = (char *) malloc(strlen(s)+1);
	if (t == NULL) {
		eprintf("estrdup(\"%.20s\") failed:", s);
	}
	strcpy(t, s);
	return t;
}

/* esystem: execute a shell command, exit on error */
void esystem(const char *command)
{
	int rc = system(command);

	if (rc == -1) {
		fatal("system: %s:", command);
	}
	if (rc != 0) {
		fatal("system: %s exit=%d", command, rc);
	}
}

#if __linux__ || __WINDOWS__
static char *name = NULL;  /* program name for messages */

// Defined in stdlib.h getprogname instead of progname
/* progname: return stored name of program */
const char *getprogname (void)
{
	return name;
}

/* setprogname: set stored name of program */
void setprogname (const char *str)
{
	name = estrdup(str);
}
#endif

static void caught_signal (int sig)
{
	call_cleanup();
	exit(2);
}

static void catch_signals (void)
{
	signal(SIGHUP,	caught_signal);
	signal(SIGINT,	caught_signal);
	signal(SIGQUIT,	caught_signal);
	signal(SIGILL,	caught_signal);
	signal(SIGTRAP,	caught_signal);
	signal(SIGABRT,	caught_signal);
	signal(SIGBUS,	caught_signal);
	signal(SIGFPE,	caught_signal);
	signal(SIGKILL,	caught_signal);
	signal(SIGSEGV,	caught_signal);
	signal(SIGPIPE,	caught_signal);
	signal(SIGSTOP,	caught_signal);
	signal(SIGTSTP,	caught_signal);
}

void set_cleanup (Cleanup_f cleanup)
{
	Cleanup = cleanup;
	catch_signals();
}

void clear_cleanup (void)
{
	Cleanup = NULL;
}
