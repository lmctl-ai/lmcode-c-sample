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
    commit=subprocess.check_output(['git','-C',str(source),'rev-parse','0ea7cb9eb5806ee7f62cd17bb01b8580ec373adf'],text=True).strip()
    if commit != '0ea7cb9eb5806ee7f62cd17bb01b8580ec373adf': raise SystemExit('required SDK commit unavailable')
    staging=ROOT/'sync-sdk.tmp'; shutil.rmtree(staging,ignore_errors=True); staging.mkdir()
    subprocess.run(['git','-C',str(source),'archive',commit,'include','std','stdlib','LICENSE'],check=True,stdout=open(staging/'archive.tar','wb'))
    shutil.unpack_archive(staging/'archive.tar',staging/'tree');
    for folder in ('include','std','stdlib'):
        shutil.rmtree(ROOT/folder,ignore_errors=True); shutil.copytree(staging/'tree'/folder,ROOT/folder)
    shutil.copy2(staging/'tree/LICENSE',ROOT/'LICENSE')
    shutil.rmtree(staging); lock={'repo':'lmctl-ai/lmcode-cpp','commit':commit,'sha256':digest(sdk_files())}; (ROOT/'sdk.lock').write_text(json.dumps(lock,indent=2)+'\n')
def main():
    mode=sys.argv[1] if len(sys.argv)>1 else 'check'
    if mode=='check': check()
    elif mode=='sdk': sync_sdk(Path(sys.argv[2] if len(sys.argv)>2 else ROOT.parent/'lmcode-cpp'))
    else: raise SystemExit('usage: sync.py check|sdk [SDK_DIR]')
if __name__=='__main__': main()
