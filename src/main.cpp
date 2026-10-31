#include "bindings.hpp"
#include "dslvm.hpp"
#include "sample_schema.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
namespace {
std::string read_bytecode(const std::string& path){std::ifstream in(std::filesystem::u8path(path),std::ios::binary);if(!in)throw std::runtime_error("cannot read sample input");std::ostringstream out;out<<in.rdbuf();if(in.bad())throw std::runtime_error("cannot read sample input");return out.str();}
std::string value_text(const dslvm::Value& v){if(v.kind==dslvm::Value::INT)return std::to_string(v.i);if(v.kind==dslvm::Value::BOOL)return v.b?"true":"false";if(v.kind==dslvm::Value::FLOAT){char b[64];std::snprintf(b,sizeof b,"%.15g",v.f);return b;}return v.s;}
int usage(){return dslvm::failure_frame("cli",dslvm::Error("CLI_USAGE","usage: --version | --lmcode-schema [--json] | --lmcode-signatures | --lmcode FILE"));}
}
static int run_cli(int argc,char** argv){
#ifdef _WIN32
_setmode(_fileno(stdout),_O_BINARY);
#endif
using namespace dslvm;
try{
 sample::Host host;
 if(argc==2&&std::string(argv[1])=="--version"){std::cout<<"lmcode-c-sample "<<SAMPLE_VERSION<<"\n";return 0;}
 if((argc==2||(argc==3&&std::string(argv[2])=="--json"))&&std::string(argv[1])=="--lmcode-schema"){std::cout.write(sample::schema,sizeof(sample::schema)-1);return 0;}
 if(argc==2&&std::string(argv[1])=="--lmcode-signatures"){for(const auto& s:host.registry.signatures())std::cout<<s<<'\n';return 0;}
 std::vector<std::string> files;std::vector<int> priorities;
 for(int i=1;i<argc;i+=2){std::string flag=argv[i];if(i+1>=argc||(flag!="--lmcode"&&flag!="--first"&&flag!="--last"))return usage();files.emplace_back(argv[i+1]);priorities.push_back(flag=="--first"?0:flag=="--last"?2:1);}
 if(files.empty())return usage();
 auto results=InvocationResults::from_command(files);
 std::vector<size_t> execution;for(size_t i=0;i<files.size();++i)execution.push_back(i);
 std::stable_sort(execution.begin(),execution.end(),[&](size_t a,size_t b){return priorities[a]<priorities[b];});
 const std::string schema(sample::schema,sizeof(sample::schema)-1);const auto contract=ResultContract::from_schema(schema);
 auto context=host.context();size_t entries_called=0;
 bool failed=false;
 for(size_t position:execution){const auto& part=results.registrations()[position];std::string phase="load";
  try{
   Program program;
   try{program=Program::load(read_bytecode(part.file),schema,host.registry);}catch(const Error& e){std::string message=e.code=="LOAD_SCHEMA_MISMATCH"?"sample schema mismatch":e.code=="LOAD_UNBOUND"?"sample import mismatch":"invalid sample bytecode";throw Error(e.code,message);}
   results.select(part.index,host.registry,contract);phase="run";program.run(context,host.registry);
   auto call_entry=[&](size_t i,Value argument){auto value=program.call(i,context,host.registry,{std::move(argument)});++entries_called;if(value.kind!=Value::VOID)std::cout<<"entry "<<program.entries()[i].name<<" = "<<value_text(value)<<'\n';};
   for(size_t i=0;i<program.entries().size();++i){const auto& annotation=program.entries()[i].annotation;if(annotation=="item"){for(size_t j=0;j<host.domain.shop.items.count;++j)call_entry(i,dslvm::Value::object("Money",host.domain.shop.items.values[j]));}else if(annotation=="total")call_entry(i,host.total_handle());}
   results.complete(part.index);
  }catch(const Error& e){results.fail(part.index,phase,e);failed=true;break;}
  catch(const std::exception&){results.fail(part.index,"app",Error("APP_FAIL","cannot read sample input"));failed=true;break;}
 }
 if(!failed&&host.domain.ledger.balance<0){results.fail(std::nullopt,"app",Error("APP_FAIL","ledger: negative balance"));failed=true;}
 if(!failed)std::cout<<"{\"host_summary\":{\"balance\": "<<host.domain.ledger.balance<<", \"entries\": "<<host.domain.ledger.entries<<", \"note\": "<<json_escape((host.domain.ledger.note?std::string(host.domain.ledger.note,host.domain.ledger.note_length):std::string()))<<", \"entries_called\": "<<entries_called<<"}}\n";
 return results.write_frame(std::cout);
}catch(const Error& e){return failure_frame("cli",e);}catch(const std::exception&){return failure_frame("app",Error("APP_FAIL","cannot read sample input"));}
}
#ifdef _WIN32
int wmain(int argc,wchar_t** wide){std::vector<std::string> values;for(int i=0;i<argc;++i){auto bytes=std::filesystem::path(wide[i]).u8string();values.emplace_back(bytes.begin(),bytes.end());}std::vector<char*> args;for(auto& v:values)args.push_back(v.data());return run_cli(argc,args.data());}
#else
int main(int argc,char** argv){return run_cli(argc,argv);}
#endif
