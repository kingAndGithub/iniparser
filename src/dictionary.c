/*-------------------------------------------------------------------------*/
/**
   @file    dictionary.c
   @author  N. Devillard
   @author  M. Brossard
   @date    Apr 2011
   @version 3.1
   @brief   Implements a dictionary for string variables.

   This module implements a simple dictionary object, i.e. a list
   of string/string associations. This object is useful to store e.g.
   informations retrieved from a configuration file (ini files).
*/
/*--------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
                                Includes
 ---------------------------------------------------------------------------*/
#include "dictionary.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

/** Maximum value size for integers and doubles. */
#define MAXVALSZ    1024

/** Minimal allocated number of entries in a dictionary */
#define DICTMINSZ   128

/** Invalid key token */
#define DICT_INVALID_KEY    ((char*)-1)

/*---------------------------------------------------------------------------
                            Private functions
 ---------------------------------------------------------------------------*/

/* Doubles the allocated size associated to a pointer */
/* 'size' is the current allocated size. */
static void * mem_double(void * ptr, int size)
{
    void * newptr ;
 
    newptr = calloc(2*size, 1);
    if (newptr==NULL) {
        return NULL ;
    }
    memcpy(newptr, ptr, size);
    free(ptr);
    return newptr ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Duplicate a string
  @param    s String to duplicate
  @return   Pointer to a newly allocated string, to be freed with free()

  This is a replacement for strdup(). This implementation is provided
  for systems that do not have it.
 */
/*--------------------------------------------------------------------------*/
static char * xstrdup(char * s)
{
    char * t ;
    if (!s)
        return NULL ;
    t = malloc(strlen(s)+1) ;
    if (t) {
        strcpy(t,s);
    }
    return t ;
}

#define hash_size(s) ((s) + ((s) >> 1))
#define hash_freed ((void *)1)

__inline__ static unsigned hash_first(dictionary *d, unsigned h)
{
       return h % hash_size(d->size);
}

__inline__ static unsigned hash_next(dictionary *d, unsigned i)
{
       return ((i + 1) == hash_size(d->size)) ? 0 : i + 1;
}

__inline__ static hash_t * hash_get(dictionary * d, char * key, unsigned hash)
{
    unsigned    i ;

    for(i = hash_first(d, hash); d->h[i].e; i = hash_next(d, i)) {
        if((d->h[i].e != hash_freed) && (d->h[i].h == hash) &&
           (strcmp(d->h[i].e->key, key) == 0)) {
            return &(d->h[i]);
        }
    }

    return NULL ;
}

__inline__ static void hash_set(dictionary * d, unsigned hash, entry_t * e)
{
    unsigned    i ;

    for(i = hash_first(d, hash); d->h[i].e && (d->h[i].e != hash_freed) ; ) {
        i = hash_next(d, i);
    }
    d->h[i].e = e;
    d->h[i].h = hash;
}

__inline__ static void free_val(dictionary *d, void *v)
{
    if(v && d ) {
        if(d->dict) {
            dictionary_del(v);
        } else {
            free(v);
        }
    }
}

__inline__ static void * dup_val(dictionary *d, void *v)
{
    return (v && d) ? (d->dict ? v : xstrdup(v)) : NULL;
}

/*---------------------------------------------------------------------------
                            Function codes
 ---------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/**
  @brief    Compute the hash key for a string.
  @param    key     Character string to use for key.
  @return   1 unsigned int on at least 32 bits.

  This hash function has been taken from an Article in Dr Dobbs Journal.
  This is normally a collision-free function, distributing keys evenly.
  The key is stored anyway in the struct so that collision can be avoided
  by comparing the key itself in last resort.
 */
/*--------------------------------------------------------------------------*/
uint32_t SuperFastHash(const char *k, int l);
unsigned dictionary_hash(char * key)
{
    return SuperFastHash(key, strlen(key));
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Create a new dictionary object.
  @param    size    Optional initial size of the dictionary.
  @return   1 newly allocated dictionary objet.

  This function allocates a new dictionary object of given size and returns
  it. If you do not know in advance (roughly) the number of entries in the
  dictionary, give size=0.
 */
/*--------------------------------------------------------------------------*/
dictionary * dictionary_new(int size)
{
    dictionary  *   d ;

    /* If no size was specified, allocate space for DICTMINSZ */
    if (size<DICTMINSZ) size=DICTMINSZ ;

    if (!(d = (dictionary *)calloc(1, sizeof(dictionary)))) {
        return NULL;
    }
    d->size = size ;
    d->e = (entry_t *)calloc(size, sizeof(entry_t));
    d->h = (hash_t *)calloc(hash_size(size), sizeof(hash_t));
    return d ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Defines storage policy for values.
  @param    d     dictionary object to modify.
  @param    dict  policy
  @return   void

  If pointer is set to 0 values are expected to be strings that will
  be duplicated when stored and freed by dictionary functions. If
  pointer is set to a non-zero value, values are treated as pointers
  to dictionary objects.
 */
/*--------------------------------------------------------------------------*/
void dictionary_policy(dictionary * d, int dict)
{
    if(d) {
        d->dict = dict ? 1 : 0;
    }
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Delete a dictionary object
  @param    d   dictionary object to deallocate.
  @return   void

  Deallocate a dictionary object and all memory associated to it.
 */
/*--------------------------------------------------------------------------*/
void dictionary_del(dictionary * d)
{
    unsigned     i ;

    if (d==NULL) return ;
    for (i=0 ; i<d->size ; i++) {
        if (d->e[i].key != NULL) {
            free(d->e[i].key);
            free_val(d, d->e[i].val);
        }
    }
    free(d->e);
    free(d->h);
    free(d);
    return ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Get a value from a dictionary.
  @param    d       dictionary object to search.
  @param    key     Key to look for in the dictionary.
  @param    def     Default value to return if key not found.
  @return   1 pointer to internally allocated character string.

  This function locates a key in a dictionary and returns a pointer to its
  value, or the passed 'def' pointer if no such key can be found in
  dictionary. The returned character pointer points to data internal to the
  dictionary object, you should not try to free it or modify it.
 */
/*--------------------------------------------------------------------------*/
void * dictionary_get(dictionary * d, char * key, void * def)
{
    unsigned    hash ;
    hash_t    * match ;

    if (d==NULL || key==NULL) return def ;

    hash = dictionary_hash(key);
    match = hash_get(d, key, hash);

    return (match && match->e && match->e->val) ? match->e->val : def ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Set a value in a dictionary.
  @param    d       dictionary object to modify.
  @param    key     Key to modify or add.
  @param    val     Value to add.
  @return   int     0 if Ok, anything else otherwise

  If the given key is found in the dictionary, the associated value is
  replaced by the provided one. If the key cannot be found in the
  dictionary, it is added to it.

  It is Ok to provide a NULL value for val, but NULL values for the dictionary
  or the key are considered as errors: the function will return immediately
  in such a case.

  Notice that if you dictionary_set a variable to NULL, a call to
  dictionary_get will return a NULL value: the variable will be found, and
  its value (NULL) is returned. In other words, setting the variable
  content to NULL is equivalent to deleting the variable from the
  dictionary. It is not possible (in this implementation) to have a key in
  the dictionary without value.

  This function returns non-zero in case of failure.
 */
/*--------------------------------------------------------------------------*/
int dictionary_set(dictionary * d, char * key, void * val)
{
    unsigned    hash, i ;
    hash_t    * h ;

    if (d==NULL || key==NULL) return -1 ;
    
    /* Compute hash for this key */
    hash = dictionary_hash(key);
    /* Find if value is already in dictionary */
    if((h = hash_get(d, key, hash)) != NULL) {
        free_val(d, h->e->val);
        h->e->val = dup_val(d, val);
        return 0;
    }

    /* Add a new value */
    /* See if dictionary needs to grow */
    if (d->n==d->size) {

        /* Reached maximum size: reallocate dictionary */
        d->e = (entry_t *)mem_double(d->e, d->size * sizeof(entry_t)) ;
        free(d->h);
        d->h = (hash_t *)calloc(hash_size(d->size * 2), sizeof(hash_t)) ;

        if ((d->e == NULL) || (d->h == NULL)) {
            /* Cannot grow dictionary */
            return -1 ;
        }

        /* Double size */
        d->size *= 2 ;

        /* Recompute all hashes */
        for(i = 0 ; i < d->size / 2 ; i++) {
            if(d->e[i].key) {
                hash_set(d, dictionary_hash(d->e[i].key), &(d->e[i]));
            }
        }
    }

    /* Insert key in the first empty slot. Start at d->n and wrap at
       d->size. Because d->n < d->size this will necessarily
       terminate. */
    for (i=d->n ; d->e[i].key ; ) {
        if(++i == d->size) i = 0;
    }

    /* Copy key */
    d->e[i].key = xstrdup(key);
    d->e[i].val = dup_val(d, val);
    hash_set(d, hash, &(d->e[i]));

    d->n ++ ;
    return 0 ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Delete a key in a dictionary
  @param    d       dictionary object to modify.
  @param    key     Key to remove.
  @return   void

  This function deletes a key in a dictionary. Nothing is done if the
  key cannot be found.
 */
/*--------------------------------------------------------------------------*/
void dictionary_unset(dictionary * d, char * key)
{
    unsigned    hash ;
    hash_t    * h ;

    if (d==NULL || key==NULL) return ;

    hash = dictionary_hash(key);
    if((h = hash_get(d, key, hash)) == NULL) {
        /* Key not found */
        return ;
    }
 
    if(h->e) {
        free(h->e->key);
        h->e->key = NULL;
        free_val(d, h->e->val);
        h->e->val = NULL;
    }

    /* Lazy deletion */
    h->e = hash_freed;
    h->h = 0;
    d->n -- ;
    return ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Dump a dictionary to an opened file pointer.
  @param    d   Dictionary to dump
  @param    f   Opened file pointer.
  @return   void

  Dumps a dictionary onto an opened file pointer. Key pairs are printed out
  as @c [Key]=[Value], one per line. It is Ok to provide stdout or stderr as
  output file pointers. Only works with dictionaries with string values.
 */
/*--------------------------------------------------------------------------*/
void dictionary_dump(dictionary * d, FILE * out)
{
    int     i ;

    if (d==NULL || out==NULL) return ;
    if (d->n<1) {
        fprintf(out, "empty dictionary\n");
        return ;
    }
    if (d->dict) {
        fprintf(out, "invalid dictionary\n");
        return ;
    }
    for (i=0 ; i<d->size ; i++) {
        if (d->e[i].key) {
            fprintf(out, "%20s\t[%s]\n",
                    d->e[i].key,
                    d->e[i].val ? (char *)d->e[i].val : "UNDEF");
        }
    }
    return ;
}


/* Test code */
#ifdef TESTDIC
#define NVALS 20000
int main(int argc, char *argv[])
{
    dictionary  *   d ;
    char    *   val ;
    int         i ;
    char        cval[90] ;

    /* Allocate dictionary */
    printf("allocating...\n");
    d = dictionary_new(0);
    
    /* Set values in dictionary */
    printf("setting %d values...\n", NVALS);
    for (i=0 ; i<NVALS ; i++) {
        sprintf(cval, "%04d", i);
        dictionary_set(d, cval, "salut");
    }
    printf("getting %d values...\n", NVALS);
    for (i=0 ; i<NVALS ; i++) {
        sprintf(cval, "%04d", i);
        val = dictionary_get(d, cval, DICT_INVALID_KEY);
        if (val==DICT_INVALID_KEY) {
            printf("cannot get value for key [%s]\n", cval);
        }
    }
    printf("unsetting %d values...\n", NVALS);
    for (i=0 ; i<NVALS ; i++) {
        sprintf(cval, "%04d", i);
        dictionary_unset(d, cval);
    }
    if (d->n != 0) {
        printf("error deleting values\n");
    }
    printf("deallocating...\n");
    dictionary_del(d);
    return 0 ;
}
#endif

/*
 * SuperFastHash by Paul Hsieh : http://www.azillionmonkeys.com/qed/hash.html
 * Public domain version http://burtleburtle.net/bob/hash/doobs.html
 */

#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((const uint8_t *)(d))[1] << UINT32_C(8))\
                      +((const uint8_t *)(d))[0])
#endif

uint32_t SuperFastHash (const char * data, int len) {
    uint32_t hash = len, tmp;
    int rem;

    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= data[sizeof (uint16_t)] << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += *data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}

/* vim: set ts=4 et sw=4 tw=75 */
