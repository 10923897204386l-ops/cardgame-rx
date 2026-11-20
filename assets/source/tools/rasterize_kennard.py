#!/usr/bin/env python3
"""Rasterise locally downloaded Kennard SVGs. Requires cairosvg (pip install cairosvg)."""
from pathlib import Path
import argparse
p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('--output',type=Path,default=Path('faces'));p.add_argument('--scale',type=int,default=1,choices=(1,2));a=p.parse_args()
try: import cairosvg
except ImportError: raise SystemExit('Install cairosvg in your build toolbox first')
a.output.mkdir(parents=True,exist_ok=True)
svgs=list(a.source.rglob('*.svg'))
if not svgs: raise SystemExit('No SVGs found. Download the Kennard SVG ZIP and extract it first.')
for src in svgs:
 dst=a.output/(src.stem+'.png')
 cairosvg.svg2png(url=str(src),write_to=str(dst),output_width=72*a.scale,output_height=100*a.scale)
 print(src.name,'->',dst)
print('Rendered',len(svgs),'SVG files; verify all 52 faces and suit/rank mapping manually.')
