#include "shop.h"
#include "sample_schema.hpp"
#include "lmcode_c.h"
#include "dslvm.hpp"
#include <fstream>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>
#include <cstring>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
static std::string bytes(const char *path) { std::ifstream in(path,std::ios::binary); if(!in) throw std::runtime_error("cannot read sample input"); std::ostringstream out; out<<in.rdbuf(); return out.str(); }
static int usage(){std::cout<<"{\"ok\": false, \"phase\": \"cli\", \"code\": \"CLI_USAGE\", \"message\": \"usage: --version | --dsl-schema [--json] | --dsl-signatures | --dsl FILE\"}\n";return 2;}
int main(int argc,char**argv){
#ifdef _WIN32
 _setmode(_fileno(stdout),_O_BINARY);
#endif
 std::cout.imbue(std::locale::classic()); lmcode_error e={NULL,NULL,0,0,0,0}; lmcode_registry*r=lmcode_registry_create(); if(!r)return 4; SampleHost*h=sample_host_create(r,&e); if(!h){lmcode_registry_destroy(r);return 4;}
 if(argc==2&&!std::string(argv[1]).compare("--version")){std::cout<<"lmcode-c-sample 0.2.0\n";sample_host_destroy(h);lmcode_registry_destroy(r);return 0;}
 if((argc==2||(argc==3&&!std::string(argv[2]).compare("--json")))&&!std::string(argv[1]).compare("--dsl-schema")){std::cout.write(sample::schema,sizeof(sample::schema)-1);sample_host_destroy(h);lmcode_registry_destroy(r);return 0;}
 if(argc==2&&!std::string(argv[1]).compare("--dsl-signatures")){for(size_t i=0;i<lmcode_registry_signature_count(r);i++)std::cout<<lmcode_registry_signature(r,i)<<'\n';sample_host_destroy(h);lmcode_registry_destroy(r);return 0;}
 if(argc!=3||std::string(argv[1])!="--dsl"){sample_host_destroy(h);lmcode_registry_destroy(r);return usage();}
 std::string bytecode;try{bytecode=bytes(argv[2]);}catch(...){lmcode_error a={"APP_FAIL","cannot read sample input",0,0,0,0};int out=lmcode_report_error("app",&a);sample_host_destroy(h);lmcode_registry_destroy(r);return out;}
 lmcode_program*p=NULL;std::string schema(sample::schema,sizeof(sample::schema)-1);if(lmcode_program_load(bytecode.data(),bytecode.size(),schema.data(),schema.size(),r,&p,&e)){if(e.code&&!strcmp(e.code,"LOAD_SCHEMA_MISMATCH"))e.message="sample schema mismatch";else if(e.code&&!strcmp(e.code,"LOAD_UNBOUND"))e.message="sample import mismatch";else e.message="invalid sample bytecode";int out=lmcode_report_error("load",&e);sample_host_destroy(h);lmcode_registry_destroy(r);return out;}
 lmcode_context*c=lmcode_context_create();sample_host_context(h,c,&e);int rc=lmcode_program_run(p,c,&e);if(rc){int out=lmcode_report_error("run",&e);lmcode_program_destroy(p);lmcode_context_destroy(c);sample_host_destroy(h);lmcode_registry_destroy(r);return out;}
 if(h->ledger.balance<0){lmcode_error a={"APP_FAIL","ledger: negative balance",0,0,0,0};rc=lmcode_report_error("app",&a);}else{std::string note=h->ledger.note?std::string(h->ledger.note,h->ledger.note_length):std::string();std::string result="{\"balance\": "+std::to_string(h->ledger.balance)+", \"entries\": "+std::to_string(h->ledger.entries)+", \"note\": "+dslvm::json_escape(note)+"}";rc=lmcode_report_ok_json(result.c_str());}
 lmcode_program_destroy(p);lmcode_context_destroy(c);sample_host_destroy(h);lmcode_registry_destroy(r);return rc;
}
