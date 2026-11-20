# Menu music source

Track: `Happy Clappy Ukulele`
Artist: Shane Ivers / Silverman Sound
Source page: https://freemusicarchive.org/music/shane-ivers/single/happy-clappy-ukulele/
Direct artist page: https://www.silvermansound.com/free-music/happy-clappy-ukulele
Licence: CC BY 4.0
Licence URL: https://creativecommons.org/licenses/by/4.0/
Required credit: `Music: Happy Clappy Ukulele by Shane Ivers - https://www.silvermansound.com`

Feature/instrumentation summary from the source page:
- ukulele
- bass guitar
- glockenspiel
- bells
- claps
- whistle
- snare brushes / drums

Repository assets:
- `happy-clappy-ukulele.mp3` — downloaded source track
- `../../runtime/music/menu_theme.wav` — 24 kHz mono ADPCM IMA WAV derived for SDL playback

Derivation command:
```sh
ffmpeg -y -i assets/source/music/happy-clappy-ukulele.mp3 \
  -af "afade=t=in:st=0:d=0.04,afade=t=out:st=188.7:d=0.04" \
  -ar 24000 -ac 1 -c:a adpcm_ima_wav assets/runtime/music/menu_theme.wav
```
