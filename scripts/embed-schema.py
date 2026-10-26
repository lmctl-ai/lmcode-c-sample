#!/usr/bin/env python3
"""Embed exact schema bytes: no runtime path discovery or JSON dependency."""
from pathlib import Path
import sys
text=Path(sys.argv[1]).read_bytes().decode('utf-8')
assert ')SAMPLE_SCHEMA"' not in text
Path(sys.argv[2]).write_text('#pragma once\nnamespace sample { inline constexpr char schema[] = R"SAMPLE_SCHEMA('+text+')SAMPLE_SCHEMA"; }\n',encoding='utf-8',newline='\n')
