/*
 * testcode/unitdname.c - unit test for dname routines.
 *
 * Copyright (c) 2007, NLnet Labs. All rights reserved.
 *
 * This software is open source.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of the NLNET LABS nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
/**
 * \file
 * Calls dname unit tests. Exits with code 1 on a failure. 
 */

#include "config.h"
#include "util/log.h"
#include "testcode/unitmain.h"
#include "util/data/dname.h"

/** put dname into buffer */
static ldns_buffer*
dname_to_buf(ldns_buffer* b, const char* str)
{
	ldns_rdf* rdf;
	ldns_status status;
	ldns_buffer_clear(b);
	rdf = ldns_dname_new_frm_str(str);	
	status = ldns_dname2buffer_wire(b, rdf);
	if(status != LDNS_STATUS_OK)
		fatal_exit("%s ldns: %s", __func__, 
			ldns_get_errorstr_by_id(status));
	ldns_rdf_deep_free(rdf);
	ldns_buffer_flip(b);
	return b;
}

/** test query_dname_len function */
static void 
dname_test_qdl(ldns_buffer* buff)
{
	unit_assert( query_dname_len(buff) == 0);
	unit_assert( query_dname_len(dname_to_buf(buff, ".")) == 1 );
	unit_assert( query_dname_len(dname_to_buf(buff, "bla.foo.")) == 9 );
	unit_assert( query_dname_len(dname_to_buf(buff, "x.y.z.example.com."
		)) == 19 );
}

/** test query_dname_tolower */
static void
dname_test_qdtl(ldns_buffer* buff)
{
	ldns_buffer_write_at(buff, 0, "\012abCDeaBCde\003cOm\000", 16);
	query_dname_tolower(ldns_buffer_begin(buff), 16);
	unit_assert( memcmp(ldns_buffer_begin(buff), 
		"\012abcdeabcde\003com\000", 16) == 0);

	ldns_buffer_write_at(buff, 0, "\001+\012abC{e-ZYXe\003NET\000", 18);
	query_dname_tolower(ldns_buffer_begin(buff), 18);
	unit_assert( memcmp(ldns_buffer_begin(buff), 
		"\001+\012abc{e-zyxe\003net\000", 18) == 0);

	ldns_buffer_write_at(buff, 0, "\000", 1);
	query_dname_tolower(ldns_buffer_begin(buff), 1);
	unit_assert( memcmp(ldns_buffer_begin(buff), "\000", 1) == 0);

	ldns_buffer_write_at(buff, 0, "\002NL\000", 4);
	query_dname_tolower(ldns_buffer_begin(buff), 4);
	unit_assert( memcmp(ldns_buffer_begin(buff), "\002nl\000", 4) == 0);
}

/** test query_dname_compare */
static void
dname_test_query_dname_compare()
{
	unit_assert(query_dname_compare((uint8_t*)"", (uint8_t*)"") == 0);
	unit_assert(query_dname_compare((uint8_t*)"\001a", 
					(uint8_t*)"\001a") == 0);
	unit_assert(query_dname_compare((uint8_t*)"\003abc\001a", 
					(uint8_t*)"\003abc\001a") == 0);
	unit_assert(query_dname_compare((uint8_t*)"\003aBc\001a", 
					(uint8_t*)"\003AbC\001A") == 0);
	unit_assert(query_dname_compare((uint8_t*)"\003abc", 
					(uint8_t*)"\003abc\001a") == -1);
	unit_assert(query_dname_compare((uint8_t*)"\003abc\001a", 
					(uint8_t*)"\003abc") == +1);
	unit_assert(query_dname_compare((uint8_t*)"\003abc\001a", 
					(uint8_t*)"") == +1);
	unit_assert(query_dname_compare((uint8_t*)"", 
					(uint8_t*)"\003abc\001a") == -1);
	unit_assert(query_dname_compare((uint8_t*)"\003abc\001a", 
					(uint8_t*)"\003xxx\001a") == -1);
	unit_assert(query_dname_compare((uint8_t*)"\003axx\001a", 
					(uint8_t*)"\003abc\001a") == 1);
	unit_assert(query_dname_compare((uint8_t*)"\003abc\001a", 
					(uint8_t*)"\003abc\001Z") == -1);
	unit_assert(query_dname_compare((uint8_t*)"\003abc\001Z", 
					(uint8_t*)"\003abc\001a") == 1);
}

/** test dname_count_labels */
static void
dname_test_count_labels()
{
	unit_assert(dname_count_labels((uint8_t*)"") == 1);
	unit_assert(dname_count_labels((uint8_t*)"\003com") == 2);
	unit_assert(dname_count_labels((uint8_t*)"\003org") == 2);
	unit_assert(dname_count_labels((uint8_t*)"\007example\003com") == 3);
	unit_assert(dname_count_labels((uint8_t*)"\003bla\007example\003com") 
		== 4);
}

/** test dname_count_size_labels */
static void
dname_test_count_size_labels()
{
	size_t sz = 0;
	unit_assert(dname_count_size_labels((uint8_t*)"", &sz) == 1);
	unit_assert(sz == 1);
	unit_assert(dname_count_size_labels((uint8_t*)"\003com", &sz) == 2);
	unit_assert(sz == 5);
	unit_assert(dname_count_size_labels((uint8_t*)"\003org", &sz) == 2);
	unit_assert(sz == 5);
	unit_assert(dname_count_size_labels((uint8_t*)"\007example\003com", 
		&sz) == 3);
	unit_assert(sz == 13);
	unit_assert(dname_count_size_labels((uint8_t*)"\003bla\007example"
		"\003com", &sz) == 4);
	unit_assert(sz == 17);
}


/** test pkt_dname_len */
static void
dname_test_pkt_dname_len(ldns_buffer* buff)
{
	ldns_buffer_clear(buff);
	ldns_buffer_write(buff, "\000", 1);
	ldns_buffer_flip(buff);
	unit_assert( pkt_dname_len(buff) == 1 );
	unit_assert( ldns_buffer_position(buff) == 1);

	ldns_buffer_clear(buff);
	ldns_buffer_write(buff, "\003org\000", 5);
	ldns_buffer_flip(buff);
	unit_assert( pkt_dname_len(buff) == 5 );
	unit_assert( ldns_buffer_position(buff) == 5);

	ldns_buffer_clear(buff);
	ldns_buffer_write(buff, "\002os\007example\003org\000", 16);
	ldns_buffer_flip(buff);
	unit_assert( pkt_dname_len(buff) == 16 );
	unit_assert( ldns_buffer_position(buff) == 16);

	/* invalid compression pointer: to self */
	ldns_buffer_clear(buff);
	ldns_buffer_write(buff, "\300\000os\007example\003org\000", 17);
	ldns_buffer_flip(buff);
	unit_assert( pkt_dname_len(buff) == 0 );

	/* valid compression pointer */
	ldns_buffer_clear(buff);
	ldns_buffer_write(buff, "\003com\000\040\300\000", 8);
	ldns_buffer_flip(buff);
	ldns_buffer_set_position(buff, 6);
	unit_assert( pkt_dname_len(buff) == 5 );
	unit_assert( ldns_buffer_position(buff) == 8);

	/* unknown label type */
	ldns_buffer_clear(buff);
	ldns_buffer_write(buff, "\002os\107example\003org\000", 16);
	ldns_buffer_flip(buff);
	unit_assert( pkt_dname_len(buff) == 0 );

	/* label too long */
	ldns_buffer_clear(buff);
	ldns_buffer_write(buff, "\002os\047example\003org\000", 16);
	ldns_buffer_flip(buff);
	unit_assert( pkt_dname_len(buff) == 0 );

	/* label exceeds packet */
	ldns_buffer_clear(buff);
	ldns_buffer_write(buff, "\002os\007example\007org\004", 16);
	ldns_buffer_flip(buff);
	unit_assert( pkt_dname_len(buff) == 0 );

	/* name very long */
	ldns_buffer_clear(buff);
	ldns_buffer_write(buff, 
		"\020a1cdef5555544444"
		"\020a2cdef5555544444"
		"\020a3cdef5555544444"
		"\020a4cdef5555544444"
		"\020a5cdef5555544444"
		"\020a6cdef5555544444"
		"\020a7cdef5555544444"
		"\020a8cdef5555544444"
		"\020a9cdef5555544444"
		"\020aAcdef5555544444"
		"\020aBcdef5555544444"
		"\020aCcdef5555544444"
		"\020aDcdef5555544444"
		"\020aEcdef5555544444"	/* 238 up to here */
		"\007aabbccd"		/* 246 up to here */
		"\007example\000"	/* 255 to here */
		, 255);
	ldns_buffer_flip(buff);
	unit_assert( pkt_dname_len(buff) == 255 );
	unit_assert( ldns_buffer_position(buff) == 255);

	/* name too long */
	ldns_buffer_clear(buff);
	ldns_buffer_write(buff, 
		"\020a1cdef5555544444"
		"\020a2cdef5555544444"
		"\020a3cdef5555544444"
		"\020a4cdef5555544444"
		"\020a5cdef5555544444"
		"\020a6cdef5555544444"
		"\020a7cdef5555544444"
		"\020a8cdef5555544444"
		"\020a9cdef5555544444"
		"\020aAcdef5555544444"
		"\020aBcdef5555544444"
		"\020aCcdef5555544444"
		"\020aXcdef5555544444"
		"\020aXcdef5555544444"
		"\020aXcdef5555544444"
		"\020aDcdef5555544444"
		"\020aEcdef5555544444"	/* 238 up to here */
		"\007aabbccd"		/* 246 up to here */
		"\007example\000"	/* 255 to here */
		, 255);
	ldns_buffer_flip(buff);
	unit_assert( pkt_dname_len(buff) == 0 );
}

/** test dname_lab_cmp */
static void
dname_test_dname_lab_cmp()
{
	int ml = 0; /* number of labels that matched exactly */

	/* test for equality succeeds */
	unit_assert(dname_lab_cmp((uint8_t*)"", 1, (uint8_t*)"", 1, &ml) == 0);
	unit_assert(ml == 1);
	unit_assert(dname_lab_cmp(
		(uint8_t*)"\003net", 2, 
		(uint8_t*)"\003net", 2, 
		&ml) == 0);
	unit_assert(ml == 2);
	unit_assert(dname_lab_cmp(
		(uint8_t*)"\007example\003net", 3, 
		(uint8_t*)"\007example\003net", 3, 
		&ml) == 0);
	unit_assert(ml == 3);
	unit_assert(dname_lab_cmp(
		(uint8_t*)"\004test\007example\003net", 4, 
		(uint8_t*)"\004test\007example\003net", 4, 
		&ml) == 0);
	unit_assert(ml == 4);

	/* root is smaller than anything else */
	unit_assert(dname_lab_cmp(
		(uint8_t*)"", 1, 
		(uint8_t*)"\003net", 2, 
		&ml) == -1);
	unit_assert(ml == 1);
	unit_assert(dname_lab_cmp(
		(uint8_t*)"\003net", 2, 
		(uint8_t*)"", 1, 
		&ml) == 1);
	unit_assert(ml == 1);
	unit_assert(dname_lab_cmp(
		(uint8_t*)"", 1, 
		(uint8_t*)"\007example\003net", 3, 
		&ml) == -1);
	unit_assert(ml == 1);
	unit_assert(dname_lab_cmp(
		(uint8_t*)"\007example\003net", 3, 
		(uint8_t*)"", 1, 
		&ml) == 1);
	unit_assert(ml == 1);

	/* label length makes a difference */
	unit_assert(dname_lab_cmp(
		(uint8_t*)"\004neta", 2, 
		(uint8_t*)"\003net", 2, 
		&ml) != 0);
	unit_assert(ml == 1);
	unit_assert(dname_lab_cmp(
		(uint8_t*)"\002ne", 2, 
		(uint8_t*)"\004neta", 2, 
		&ml) != 0);
	unit_assert(ml == 1);

	/* contents follow the zone apex */
	unit_assert(dname_lab_cmp(
		(uint8_t*)"\003bla\007example\003net", 4, 
		(uint8_t*)"\007example\003net", 3, 
		&ml) == 1);
	unit_assert(ml == 3);
	unit_assert(dname_lab_cmp(
		(uint8_t*)"\007example\003net", 3, 
		(uint8_t*)"\003bla\007example\003net", 4, 
		&ml) == -1);
	unit_assert(ml == 3);

	/* label content makes a difference */
	unit_assert(dname_lab_cmp(
		(uint8_t*)"\003aag\007example\003net", 4, 
		(uint8_t*)"\003bla\007example\003net", 4, 
		&ml) == -1);
	unit_assert(ml == 3);
	unit_assert(dname_lab_cmp(
		(uint8_t*)"\003aag\007example\003net", 4, 
		(uint8_t*)"\003bla\007example\003net", 4, 
		&ml) == -1);
	unit_assert(ml == 3);
	unit_assert(dname_lab_cmp(
		(uint8_t*)"\003bla\003aag\007example\003net", 5, 
		(uint8_t*)"\003aag\003bla\007example\003net", 5, 
		&ml) == -1);
	unit_assert(ml == 3);
	unit_assert(dname_lab_cmp(
		(uint8_t*)"\02sn\003opt\003aag\007example\003net", 6, 
		(uint8_t*)"\02sn\003opt\003bla\007example\003net", 6, 
		&ml) == -1);
	unit_assert(ml == 3);

	/* but lowercase/uppercase does not make a difference. */
	unit_assert(dname_lab_cmp(
		(uint8_t*)"\003bLa\007examPLe\003net", 4, 
		(uint8_t*)"\003bla\007eXAmple\003nET", 4, 
		&ml) == 0);
	unit_assert(ml == 4);
}

/** test dname_subdomain_c */
static void
dname_test_subdomain()
{
	unit_assert(dname_subdomain_c(
		(uint8_t*)"",
		(uint8_t*)""));
	unit_assert(dname_subdomain_c(
		(uint8_t*)"\003com",
		(uint8_t*)""));
	unit_assert(!dname_subdomain_c(
		(uint8_t*)"",
		(uint8_t*)"\003com"));
	unit_assert(dname_subdomain_c(
		(uint8_t*)"\007example\003com",
		(uint8_t*)"\003com"));
	unit_assert(!dname_subdomain_c(
		(uint8_t*)"\003com",
		(uint8_t*)"\007example\003com"));
	unit_assert(dname_subdomain_c(
		(uint8_t*)"\007example\003com",
		(uint8_t*)""));
	unit_assert(!dname_subdomain_c(
		(uint8_t*)"\003net",
		(uint8_t*)"\003com"));
	unit_assert(!dname_subdomain_c(
		(uint8_t*)"\003net",
		(uint8_t*)"\003org"));
	unit_assert(!dname_subdomain_c(
		(uint8_t*)"\007example\003net",
		(uint8_t*)"\003org"));
	unit_assert(!dname_subdomain_c(
		(uint8_t*)"\003net",
		(uint8_t*)"\007example\003org"));
}

/** test dname_strict_subdomain */
static void
dname_test_strict_subdomain()
{
	unit_assert(!dname_strict_subdomain(
		(uint8_t*)"", 1,
		(uint8_t*)"", 1));
	unit_assert(dname_strict_subdomain(
		(uint8_t*)"\003com", 2,
		(uint8_t*)"", 1));
	unit_assert(!dname_strict_subdomain(
		(uint8_t*)"", 1,
		(uint8_t*)"\003com", 2));
	unit_assert(dname_strict_subdomain(
		(uint8_t*)"\007example\003com", 3,
		(uint8_t*)"\003com", 2));
	unit_assert(!dname_strict_subdomain(
		(uint8_t*)"\003com", 2,
		(uint8_t*)"\007example\003com", 3));
	unit_assert(dname_strict_subdomain(
		(uint8_t*)"\007example\003com", 3,
		(uint8_t*)"", 1));
	unit_assert(!dname_strict_subdomain(
		(uint8_t*)"\003net", 2,
		(uint8_t*)"\003com", 2));
	unit_assert(!dname_strict_subdomain(
		(uint8_t*)"\003net", 2,
		(uint8_t*)"\003org", 2));
	unit_assert(!dname_strict_subdomain(
		(uint8_t*)"\007example\003net", 3,
		(uint8_t*)"\003org", 2));
	unit_assert(!dname_strict_subdomain(
		(uint8_t*)"\003net", 2,
		(uint8_t*)"\007example\003org", 3));
}

void dname_test()
{
	ldns_buffer* buff = ldns_buffer_new(65800);
	ldns_buffer_flip(buff);
	dname_test_qdl(buff);
	dname_test_qdtl(buff);
	dname_test_query_dname_compare();
	dname_test_count_labels();
	dname_test_count_size_labels();
	dname_test_dname_lab_cmp();
	dname_test_pkt_dname_len(buff);
	dname_test_strict_subdomain();
	dname_test_subdomain();
	ldns_buffer_free(buff);
}
