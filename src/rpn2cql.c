/* This file is part of the YAZ toolkit.
 * Copyright (C) 1995-2008 Index Data
 * See the file LICENSE for details.
 */

/**
 * \file
 * \brief Implements RPN to CQL conversion
 *
 * Evaluation order of rules:
 *
 * always
 * relation
 * structure
 * position
 * truncation
 * index
 * relationModifier
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <yaz/rpn2cql.h>
#include <yaz/xmalloc.h>
#include <yaz/diagbib1.h>
#include <yaz/z-core.h>
#include <yaz/wrbuf.h>

static int rpn2cql_simple(cql_transform_t ct,
                          void (*pr)(const char *buf, void *client_data),
                          void *client_data,
                          Z_Operand *q)
{
    int ret = 0;
    if (q->which != Z_Operand_APT)
    {
        ret = -1;
        cql_transform_set_error(ct, YAZ_BIB1_RESULT_SET_UNSUPP_AS_A_SEARCH_TERM, 0);
    }
    else
    {
        Z_AttributesPlusTerm *apt = q->u.attributesPlusTerm;
        Z_Term *term = apt->term;
        Z_AttributeList *attributes = apt->attributes;
        WRBUF w = wrbuf_alloc();

        switch(term->which)
        {
        case Z_Term_general:
            wrbuf_write(w, (const char *) term->u.general->buf, term->u.general->len);
            break;
        case Z_Term_numeric:
            wrbuf_printf(w, "%d", *term->u.numeric);
            break;
        case Z_Term_characterString:
            wrbuf_puts(w, term->u.characterString);
            break;
        default:
            ret = -1;
            cql_transform_set_error(ct, YAZ_BIB1_TERM_TYPE_UNSUPP, 0);
        }
        if (ret == 0)
            pr(wrbuf_cstr(w), client_data);
        wrbuf_destroy(w);
    }
    return ret;
}

static int rpn2cql_structure(cql_transform_t ct,
                             void (*pr)(const char *buf, void *client_data),
                             void *client_data,
                             Z_RPNStructure *q, int nested)
{
    if (q->which == Z_RPNStructure_simple)
        return rpn2cql_simple(ct, pr, client_data, q->u.simple);
    else
    {
        Z_Operator *op = q->u.complex->roperator;
        int r;

        if (nested)
            pr("(", client_data);

        r = rpn2cql_structure(ct, pr, client_data, q->u.complex->s1, 1);
        if (r)
            return r;
        switch(op->which)
        {
        case  Z_Operator_and:
            pr(" and ", client_data);
            break;
        case  Z_Operator_or:
            pr(" or ", client_data);
            break;
        case  Z_Operator_and_not:
            pr(" not ", client_data);
            break;
        case  Z_Operator_prox:
            cql_transform_set_error(ct, YAZ_BIB1_UNSUPP_SEARCH, 0);
            return -1;
        }
        r = rpn2cql_structure(ct, pr, client_data, q->u.complex->s2, 1);
        if (nested)
            pr(")", client_data);
        return r;
    }
}

int cql_transform_rpn2cql(cql_transform_t ct,
                          void (*pr)(const char *buf, void *client_data),
                          void *client_data,
                          Z_RPNQuery *q)
{
    cql_transform_set_error(ct, 0, 0);
    return rpn2cql_structure(ct, pr, client_data, q->RPNStructure, 0);
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
