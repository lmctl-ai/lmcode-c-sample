#include "domain.h"
#include <assert.h>
#include <math.h>
#include <string.h>
int main(void){SampleDomain d;sample_domain_init(&d);assert(sample_wrap_add(INT64_MAX,1)==INT64_MIN);assert(sample_wrap_add(-2,1)==-1);int64_t cents=0;assert(sample_units_to_cents(1.25,&cents)==0&&cents==125);assert(sample_units_to_cents(NAN,&cents)!=0);assert(sample_units_to_cents(INFINITY,&cents)!=0);const char raw[]={'a','\0','b'};assert(sample_ledger_note(&d.ledger,raw,3)==0&&d.ledger.note_length==3&&!memcmp(d.ledger.note,raw,3));SampleMoney*m=sample_money_new(&d,9);assert(m&&sample_money_get(m)==m);sample_domain_destroy(&d);return 0;}
