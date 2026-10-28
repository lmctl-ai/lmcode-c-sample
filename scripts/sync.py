#!/usr/bin/env python3
"""Check pinned SDK/suite bytes, or sync an immutable SDK commit and suite."""
import hashlib, json, shutil, subprocess, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent

def digest(paths): return {p.relative_to(ROOT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(paths)}
def sdk_files():
    paths=[]
    for folder in (ROOT/'include', ROOT/'std', ROOT/'stdlib'):
        paths += [p for p in folder.rglob('*') if p.is_file()]
    return paths + [ROOT/'LICENSE']
def check():
    lock=json.loads((ROOT/'sdk.lock').read_text())
    if digest(sdk_files()) != lock['sha256']: raise SystemExit('SDK drift: restore pinned archive or sync a reviewed commit')
    suite=json.loads((ROOT/'suite.lock').read_text())
    files=[p for p in (ROOT/'sample-suite').rglob('*') if p.is_file() and '__pycache__' not in p.parts and p.suffix != '.pyc']
    if {p.relative_to(ROOT/'sample-suite').as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(files)} != suite['sha256']: raise SystemExit('suite drift: restore canonical bytes')
    print('PASS SDK and suite locks')
def sync_sdk(source):
    commit=subprocess.check_output(['git','-C',str(source),'rev-parse','HEAD'],text=True).strip()
    staging=ROOT/'sync-sdk.tmp'; shutil.rmtree(staging,ignore_errors=True); staging.mkdir()
    subprocess.run(['git','-C',str(source),'archive',commit,'include','std','stdlib','LICENSE'],check=True,stdout=open(staging/'archive.tar','wb'))
    shutil.unpack_archive(staging/'archive.tar',staging/'tree');
    for folder in ('include','std','stdlib'):
        shutil.rmtree(ROOT/folder,ignore_errors=True); shutil.copytree(staging/'tree'/folder,ROOT/folder,copy_function=shutil.copyfile)
    shutil.copy2(staging/'tree/LICENSE',ROOT/'LICENSE')
    shutil.rmtree(staging); lock={'repo':'lmctl-ai/lmcode-cpp','commit':commit,'sha256':digest(sdk_files())}; (ROOT/'sdk.lock').write_text(json.dumps(lock,indent=2)+'\n')
def sync_suite(source):
    commit=subprocess.check_output(['git','-C',str(source),'rev-parse','HEAD'],text=True).strip()
    if subprocess.check_output(['git','-C',str(source),'status','--porcelain','--','sample-suite'],text=True):
        raise SystemExit('commit the canonical suite before syncing')
    destination=ROOT/'sample-suite'
    shutil.rmtree(destination)
    shutil.copytree(source/'sample-suite',destination,ignore=shutil.ignore_patterns('__pycache__','*.pyc'))
    files=sorted(p for p in destination.rglob('*') if p.is_file())
    lock={'repo':'lmctl-ai/lmcode-cpp','commit':commit,'sha256':{p.relative_to(destination).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in files}}
    (ROOT/'suite.lock').write_text(json.dumps(lock,indent=2)+'\n')
def main():
    mode=sys.argv[1] if len(sys.argv)>1 else 'check'
    if mode=='check': check()
    elif mode=='sdk': sync_sdk(Path(sys.argv[2] if len(sys.argv)>2 else ROOT.parent/'lmcode-cpp'))
    elif mode=='suite': sync_suite(Path(sys.argv[2] if len(sys.argv)>2 else ROOT.parent/'lmcode-cpp'))
    else: raise SystemExit('usage: sync.py check|sdk|suite [SDK_DIR]')
if __name__=='__main__': main()
