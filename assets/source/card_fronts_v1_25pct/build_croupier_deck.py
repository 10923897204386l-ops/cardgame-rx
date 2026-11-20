from PIL import Image,ImageDraw,ImageFont,ImageOps
from pathlib import Path
import zipfile, hashlib, json, shutil
S=4; W,H=72*S,100*S
root=Path('/mnt/data/croupier_card_fronts_v1_25pct');root.mkdir(exist_ok=True)
source=Image.open('/mnt/data/a_clean_white_background_with_four_side_by_side_br.png').convert('RGB')
font='/usr/share/fonts/truetype/dejavu/DejaVuSerif-Bold.ttf'
suitfont='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
cream=(255,242,207); black=(15,15,18); red=(198,8,18)
# 4 separate illustrated portraits; first three are used for K,Q,J.
portraits={}
for key,i in [('K',0),('Q',1),('J',2)]:
    x0=[40,560,1080][i]; x1=[495,1005,1540][i]
    im=source.crop((x0,35,x1,670)).resize((210,270),Image.Resampling.LANCZOS)
    pix=im.load()
    for y in range(im.height):
      for x in range(im.width):
        r,g,b=pix[x,y]
        if min(r,g,b)>225:
          t=(min(r,g,b)-225)/30
          pix[x,y]=tuple(int(v*(1-t)+c*t) for v,c in zip((r,g,b),cream))
    portraits[key]=im

def center(d,xy,txt,font,fill):
  b=d.textbbox((0,0),txt,font=font);d.text((xy[0]-(b[2]-b[0])/2,xy[1]-(b[3]-b[1])/2-b[1]),txt,font=font,fill=fill,stroke_width=0)

def face(rank,suit):
  im=Image.new('RGBA',(W,H),(0,0,0,0));d=ImageDraw.Draw(im)
  d.rounded_rectangle((3,3,W-4,H-4),radius=23,fill=cream,outline=(18,19,19),width=7)
  color=red if suit in 'HD' else black
  sym={'S':'♠','H':'♥','D':'♦','C':'♣'}[suit]
  rf=ImageFont.truetype(font,82 if rank!='T' else 71)
  sf=ImageFont.truetype(suitfont,54)
  d.text((17,5),'10' if rank=='T' else rank,font=rf,fill=color,stroke_width=0)
  d.text((17,91),sym,font=sf,fill=color)
  # Small rotated corner index to balance full card.
  corner=Image.new('RGBA',(100,105));cd=ImageDraw.Draw(corner)
  cd.text((5,0),'10' if rank=='T' else rank,font=ImageFont.truetype(font,41),fill=color)
  cd.text((9,49),sym,font=ImageFont.truetype(suitfont,30),fill=color)
  im.alpha_composite(corner.rotate(180), (W-104,H-110))
  d=ImageDraw.Draw(im)
  if rank in 'JQK':
    # Single upright portrait, as in the user-provided reference; keep corners clear.
    p=portraits[rank].resize((199,252),Image.Resampling.LANCZOS)
    im.paste(p,(77,92))
    d=ImageDraw.Draw(im);d.rounded_rectangle((76,91,277,345),radius=5,outline=(32,27,24),width=3)
  else:
    pf=ImageFont.truetype(suitfont,69 if rank!='A' else 158)
    if rank=='A': pts=[(145,213)]
    else:
      patterns={
      '2':[(145,146),(145,294)],
      '3':[(145,139),(145,218),(145,296)],
      '4':[(102,149),(200,149),(102,287),(200,287)],
      '5':[(102,149),(200,149),(145,218),(102,287),(200,287)],
      '6':[(102,145),(200,145),(102,218),(200,218),(102,292),(200,292)],
      '7':[(102,145),(200,145),(145,181),(102,218),(200,218),(102,292),(200,292)],
      '8':[(102,142),(200,142),(145,178),(102,214),(200,214),(145,252),(102,292),(200,292)],
      '9':[(102,139),(200,139),(102,190),(200,190),(145,216),(102,243),(200,243),(102,296),(200,296)],
      'T':[(102,137),(200,137),(145,164),(102,190),(200,190),(145,216),(102,242),(200,242),(102,295),(200,295)]}
      pts=patterns[rank]
    for p in pts:center(d,p,sym,pf,color)
  return im
manifest={}; thumbs=[]
for suit in 'SHDC':
 for rank in 'A23456789TJQK':
  name=rank+suit
  im=face(rank,suit)
  for dims,folder in [((72,100),'faces_72x100'),((144,200),'faces_144x200')]:
   out=root/folder;out.mkdir(exist_ok=True)
   f=out/(name+'.png');im.resize(dims,Image.Resampling.LANCZOS).save(f,optimize=True)
   manifest[str(f.relative_to(root))]=hashlib.sha256(f.read_bytes()).hexdigest()
  thumbs.append((name,im.resize((72,100),Image.Resampling.LANCZOS)))
# contact sheet 13 cols x 4 rows
sheet=Image.new('RGB',(13*85+12,4*120+20),(238,236,226));sd=ImageDraw.Draw(sheet)
for i,(name,im) in enumerate(thumbs):
 x=12+(i%13)*85;y=8+(i//13)*120;sheet.paste(im,(x,y),im);sd.text((x+17,y+101),name,fill='black')
sheet.save(root/'contact_sheet_52.png')
# include exact reference and the full upstream illustrated court portrait art
shutil.copy('/mnt/data/image(20261120-055117).png',root/'user_reference.png')
shutil.copy('/mnt/data/a_clean_white_background_with_four_side_by_side_br.png',root/'court_portrait_source.png')
shutil.copy('/mnt/data/build_croupier_deck_25pct.py',root/'build_croupier_deck.py')
(root/'README.md').write_text('''# Croupier-RX complete card-front deck v1.1 (25% larger top-left indices)\n\n52 distinct PNGs in `faces_72x100/` and `faces_144x200/`, plus contact sheet, user reference, original illustrated court portrait sheet and deterministic generation script.\n\nTop-left rank and suit rendered at approximately 125% of v1 nominal font sizes; bottom-right index and all main pips remain unchanged.

Style: warm ivory faces, rounded charcoal border, bold traditional red/black pips and indices, illustrated single-upright court figures based on the user reference. Ranks A,2..9,T(10),J,Q,K; suits S,H,D,C. All pips generated deterministically; no AI-generated pip counts.\n\nCourt illustrations are newly generated artwork, not original Kennard. They share one illustration per rank (J/Q/K) across four suits; this is a **style prototype**, not a historically accurate four-suit court deck. Please review court styling and 72x100 legibility before integrating. The blue-and-gold back and approved felt are separate assets from the previous M0.1F package.\n\nThese sprites have transparent corners and no baked shadow; Warp should add a soft contact shadow at render time. Use `faces_72x100` for native gameplay and `faces_144x200` for higher-resolution display. Preserve original deck as a selectable fallback.\n''')
(root/'SHA256.json').write_text(json.dumps(manifest,indent=2))
zip_path='/mnt/data/croupier_card_fronts_v1_25pct.zip'
with zipfile.ZipFile(zip_path,'w',zipfile.ZIP_DEFLATED) as z:
 for f in root.rglob('*'):
  if f.is_file():z.write(f,f.relative_to(root))
print(zip_path,len(manifest),'files')
