/*-
 * Copyright (c) 2011 Varnish Software AS
 * All rights reserved.
 *
 * Author: Kristian Lyngstol <kristian@bohemians.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdlib.h>

#include "vrt.h"
#include "bin/varnishd/cache.h"

#include "vcc_if.h"

int
init_function(struct vmod_priv *priv, const struct VCL_conf *conf)
{
	return (0);
}

/*
 * Stolen bluntly from cache_vrt.c (should be fixed)
 */
static struct http *
vrt_selecthttp(const struct sess *sp, enum gethdr_e where)
{
        struct http *hp;

        CHECK_OBJ_NOTNULL(sp, SESS_MAGIC);
        switch (where) {
        case HDR_REQ:
                hp = sp->http;
                break;
        case HDR_BEREQ:
                hp = sp->wrk->bereq;
                break;
        case HDR_BERESP:
                hp = sp->wrk->beresp;
                break;
        case HDR_RESP:
                hp = sp->wrk->resp;
                break;
        case HDR_OBJ:
                CHECK_OBJ_NOTNULL(sp->obj, OBJECT_MAGIC);
                hp = sp->obj->http;
                break;
        default:
                assert("ops");
        }
        CHECK_OBJ_NOTNULL(hp, HTTP_MAGIC);
        return (hp);
}


void __match_proto__()
vmod_append(struct sess *sp, enum gethdr_e e, const char *h, const char *fmt, ...)
{
	va_list ap;
	struct http *hp;
	char *b;

	CHECK_OBJ_NOTNULL(sp, SESS_MAGIC);
	assert(fmt != NULL);
	
	hp = vrt_selecthttp(sp, e);

	va_start(ap, fmt);
	b = VRT_String(hp->ws, h + 1, fmt, ap);
	if (b == NULL) {
		WSP(sp, SLT_LostHeader, "vmod_header: %s", h+1);
	} else {
		http_SetHeader(sp->wrk,sp->fd,hp,b);
	}
	va_end(ap);
}

/*
void __match_proto__()
vmod_remove(struct sess *sp, enum gethdr_e e, const char *h, const char *s)
{
	(void)e;
	(void)sp;
	(void)h;
	return (1);
}

void __match_proto__()
vmod_get(struct sess *sp, enum gethdr_e e, const char *h, const char *s)
{
	(void)e;
	(void)sp;
	(void)h;
	return (1);
}
*/
