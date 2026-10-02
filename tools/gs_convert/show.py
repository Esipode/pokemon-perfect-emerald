"""Print a map's scripts without text and movement data. show.py MAP [MAP...] [--json]"""
import sys, re, os, json
from common import ROOT
for name in [a for a in sys.argv[1:] if not a.startswith('--')]:
    p = os.path.join(ROOT, 'data/maps', name, 'scripts.inc')
    print('#####', name)
    skip = False
    for raw in open(p, encoding='utf-8', errors='ignore'):
        l = raw.rstrip()
        if re.match(r'^\w+:{1,2}\s*$', l):
            skip = bool(re.search(r'(Text|Movement)', l.split(':')[0])) and not l.endswith('::')
            if skip or re.search(r'(_Text_|_Movement_)', l):
                skip = True; continue
        if skip:
            if l.startswith('\t.string') or l.strip().startswith(('walk_','face_','step_','jump_','delay_','emote','run_','slide_','lock_','set_','player_','levitate','init_','disable_','enable_','fly_','slow','faster','fast','nop','jump','acro','walk','.','#')) or not l.strip():
                continue
            skip = False
        if l.strip().startswith('.string') or not l.strip():
            continue
        print(l)
    if '--json' in sys.argv:
        d = json.load(open(os.path.join(ROOT, 'data/maps', name, 'map.json')))
        for o in d['object_events']:
            print('OBJ', o['script'], o['flag'], o['x'], o['y'], o['trainer_type'][13:])
        for c in d['coord_events']:
            print('COORD', c.get('var'), c.get('var_value'), c['script'], c['x'], c['y'])
        for b in d['bg_events']:
            print('BG', b.get('script'), b['x'], b['y'])
