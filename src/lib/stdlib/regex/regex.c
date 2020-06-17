#include <cube/cubeext.h>
#include <re.h>

typedef struct pattern_st
{
    re_t pattern;
    int id;
    struct pattern_st *next;
} pattern_t;

typedef struct match_st
{
    char *match;
    int idx;
    int length;
    struct match_st *next;
} match_t;

match_t *matchs = NULL;
pattern_t *patterns = NULL;

EXPORTED void cube_init()
{
    matchs = NULL;
    patterns = NULL;
}

EXPORTED void cube_release()
{
    pattern_t *curP = NULL;
    while (patterns != NULL)
    {
        curP = patterns;
        patterns = curP->next;
        free(curP);
    }

    match_t *curM = NULL;
    while (matchs != NULL)
    {
        curM = matchs;
        matchs = curM->next;
        free(curM->match);
        free(curM);
    }
}

EXPORTED int compile(const char *pattern)
{
    pattern_t *p = malloc(sizeof(pattern_t));
    p->next = patterns;
    if (p->next == NULL)
        p->id = 0;
    else
        p->id = p->next->id + 1;
    p->pattern = re_compile(pattern);
    patterns = p;
    return p->id;
}

EXPORTED char *match(const char *pattern, const char *text)
{
    int match_length;
    int match_idx = re_match(pattern, text, &match_length);
    if (match_idx == -1)
        return NULL;

    match_t *m = malloc(sizeof(match_t));
    m->next = matchs;
    m->idx = match_idx;
    m->length = match_length;
    m->match = malloc(sizeof(char) * (match_length + 1));
    memcpy(m->match, &text[match_idx], match_length);
    m->match[match_length] = '\0';

    matchs = m;
    return m->match;
}

EXPORTED char *matchp(int id, const char *text)
{
    pattern_t *p = patterns;
    while (p != NULL && p->id != id)
    {
        p = p->next;
    }

    if (p == NULL)
        return NULL;

    int match_length;
    int match_idx = re_matchp(p->pattern, text, &match_length);
    if (match_idx == -1)
        return NULL;

    match_t *m = malloc(sizeof(match_t));
    m->next = matchs;
    m->idx = match_idx;
    m->length = match_length;
    m->match = malloc(sizeof(char) * (match_length + 1));
    memcpy(m->match, &text[match_idx], match_length);
    m->match[match_length] = '\0';

    matchs = m;
    return m->match;
}

EXPORTED cube_native_var *matchAll(char *pattern, const char *text)
{
    cube_native_var *list = NATIVE_LIST();

    int id = compile(pattern);

    char *txt = (char *)text;
    char *res = matchp(id, txt);
    while (res != NULL)
    {
        ADD_NATIVE_LIST(list, NATIVE_STRING_COPY(res));
        txt = txt + (matchs->idx + matchs->length);
        res = matchp(id, txt);
    }
    return list;
}

EXPORTED cube_native_var *matchpAll(int id, const char *text)
{
    cube_native_var *list = NATIVE_LIST();
    char *txt = (char *)text;
    char *res = matchp(id, txt);
    while (res != NULL)
    {
        ADD_NATIVE_LIST(list, NATIVE_STRING_COPY(res));
        txt = txt + (matchs->idx + matchs->length);
        res = matchp(id, txt);
    }
    return list;
}