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
#include <pthread.h>

#include "vrt.h"
#include "bin/varnishd/cache.h"
#include "include/vct.h"

#include "vcc_if.h"
#include "config.h"

pthread_mutex_t header_mutex;

/*
 * Stolen bluntly from cache_vrt.c (should be fixed)
 *
 * Returns the right struct http * to use for a given type of header.
 */
static struct http *
header_vrt_selecthttp(const struct sess *sp, enum gethdr_e where)
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

static int
header_http_IsHdr(const txt *hh, const char *hdr)
{
	unsigned l;

	Tcheck(*hh);
	AN(hdr);
	l = hdr[0];
	assert(l == strlen(hdr + 1));
	assert(hdr[l] == ':');
	hdr++;
	return (!strncasecmp(hdr, hh->b, l));
}

/*
 * Return true if the hp->hd[u] header matches *hdr and the regex *re
 * matches the content.
 *
 * If re is NULL, content is not tested and as long as it's the right
 * header, a match is returned.
 */
static int
header_http_match(const struct http *hp, unsigned u, void *re, const char *hdr)
{
	char *start;
	unsigned l;
	
	assert(hdr);
	assert(hp);
	
	Tcheck(hp->hd[u]);
	if (hp->hd[u].b == NULL)
		return 0;
	
	l = hdr[0];
	
	if (!header_http_IsHdr(&hp->hd[u], hdr))
		return 0;

	if (re == NULL)
		return 1;
			
	start = hp->hd[u].b + l;
	while (*start != '\0' && *start == ' ')
		start++;
	
	if (!*start)
		return 0;
	
	if (VRT_re_match(start,re))
		return 1;
	
	return 0;
}

/*
 * Returns the header named as *hdr that also matches the regular
 * expression *re. Blatant copy of http_findhdr() in varnishd, with the
 * re-stuff added.
 */
static unsigned
header_http_findhdr(const struct http *hp, const char *hdr, void *re)
{
        unsigned u;

        for (u = HTTP_HDR_FIRST; u < hp->nhd; u++) {
		if (header_http_match(hp, u, re, hdr))
			return (u);
        }
        return (0);
}

/*
 * Same as http_Unset(), pluss regex: It removes the header *hdr, assuming
 * it matches the regular expression *re.
 */
static void
header_http_Unset(struct http *hp, const char *hdr, void *re)
{
	unsigned u, v;

	for (v = u = HTTP_HDR_FIRST; u < hp->nhd; u++) {
		if (header_http_match(hp, u, re, hdr))
			continue;
		if (v != u) {
			memcpy(&hp->hd[v], &hp->hd[u], sizeof *hp->hd);
			memcpy(&hp->hdf[v], &hp->hdf[u], sizeof *hp->hdf);
		}
		v++;
	}
	hp->nhd = v;
}

int
init_function(struct vmod_priv *priv, const struct VCL_conf *conf)
{
	int ret;
	ret = pthread_mutex_init(&header_mutex, NULL);
	assert (ret == 0);
	return (0);
}


void __match_proto__()
vmod_append(struct sess *sp, enum gethdr_e e, const char *h, const char *fmt, ...)
{
	va_list ap;
	struct http *hp;
	char *b;
	CHECK_OBJ_NOTNULL(sp, SESS_MAGIC);
	assert(fmt != NULL);
	
	hp = header_vrt_selecthttp(sp, e);
	va_start(ap, fmt);
	b = VRT_String(hp->ws, h + 1, fmt, ap);
	if (b == NULL)
		WSP(sp, SLT_LostHeader, "vmod_header: %s", h+1);
	else
		http_SetHeader(sp->wrk,sp->fd,hp,b);
	va_end(ap);
}

/*
 * XXX: Needs to be cleaned up a bit
 */
const char * __match_proto__()
vmod_get(struct sess *sp, struct vmod_priv *priv, enum gethdr_e e, const char *h, const char *s)
{
	struct http *hp;
	unsigned u;
	char *p;
	int ret;

	if (priv->priv == NULL) {
		ret = pthread_mutex_lock(&header_mutex);
		assert (ret == 0);
		/*
		 * Need to re-test to avoid race
		 */
		if (priv->priv == NULL) {
			VRT_re_init(&priv->priv,s);
			priv->free = VRT_re_fini;
		}
		pthread_mutex_unlock(&header_mutex);
	}
	
	hp = header_vrt_selecthttp(sp, e);
	u = header_http_findhdr(hp,h,priv->priv);
	if (u == 0) {
		return NULL;
	}
	p = hp->hd[u].b + h[0];
	while (vct_issp(*p))
		p++;
	return p;
}

/*
 * Returns the header named as *hdr that also matches the regular
 * expression *re.
 */
static unsigned
header_http_cphdr(struct sess *sp, const struct http *hp, const char *hdr, enum gethdr_e dst_e, const char *dst_h)
{
        unsigned u;
	char *p;
        for (u = HTTP_HDR_FIRST; u < hp->nhd; u++) {
		if (!header_http_match(hp, u, NULL, hdr))
			continue;
		
		p = hp->hd[u].b + hdr[0];
		while (vct_issp(*p))
			p++;
                vmod_append(sp, dst_e, dst_h, p,vrt_magic_string_end);
        }
        return (0);
}

/*
 * XXX: Needs to be cleaned up a bit
 */
void __match_proto__()
vmod_copy(struct sess *sp, enum gethdr_e src_e, const char *src_h, enum gethdr_e dst_e, const char *dst_h)
{
	struct http *src_hp;
	unsigned u;
	char *p;
	int ret;

	src_hp = header_vrt_selecthttp(sp, src_e);
	u = header_http_cphdr(sp,src_hp,src_h,dst_e,dst_h);
}

/*
 * XXX: Needs to be cleaned up a bit
 */
void __match_proto__()
vmod_remove(struct sess *sp, struct vmod_priv *priv, enum gethdr_e e, const char *h, const char *s)
{
	struct http *hp;
	unsigned u;
	char *p;
	int ret;

	if (priv->priv == NULL) {
		ret = pthread_mutex_lock(&header_mutex);
		assert (ret == 0);
		/*
		 * Need to re-test to avoid race
		 */
		if (priv->priv == NULL) {
			VRT_re_init(&priv->priv,s);
			priv->free = VRT_re_fini;
		}
		pthread_mutex_unlock(&header_mutex);
	}
	
	hp = header_vrt_selecthttp(sp, e);
	header_http_Unset(hp,h,priv->priv);
}

const char * __match_proto__()
vmod_version(struct sess *sp)
{
	return VERSION;
}
