import numpy as np
from PIL import Image, ImageDraw, ImageFilter
rng=np.random.default_rng(11)
S=2000; ink=Image.new('L',(S,S),0); d=ImageDraw.Draw(ink)
def stroke(pts, w0, w1, jitter=6, bristles=True):
    pts=np.array(pts,float); seg=[]
    for a,b in zip(pts[:-1],pts[1:]):
        n=int(np.linalg.norm(b-a)/3)+1
        for t in np.linspace(0,1,n,endpoint=False): seg.append(a+(b-a)*t)
    seg.append(pts[-1]); seg=np.array(seg); N=len(seg)
    for i,p in enumerate(seg):
        t=i/max(1,N-1); w=w0+(w1-w0)*t
        w*=1+0.18*np.sin(i*0.05)+rng.normal(0,0.05)
        q=p+rng.normal(0,jitter,2)
        d.ellipse([q[0]-w,q[1]-w,q[0]+w,q[1]+w],fill=255)
    if bristles:   # dry-brush hairs trailing off the stroke
        dirv=seg[-1]-seg[0]; dirv/=np.linalg.norm(dirv)+1e-9; nrm=np.array([-dirv[1],dirv[0]])
        for k in range(14):
            off=rng.uniform(-1,1)*max(w0,w1)*0.9; L=rng.uniform(40,160)
            a=seg[-1]+nrm*off; b=a+dirv*L+rng.normal(0,6,2)
            d.line([tuple(a),tuple(b)],fill=255,width=int(rng.uniform(2,6)))
def blob(c,r):
    for k in range(10):
        q=np.array(c)+rng.normal(0,r*0.35,2); rr=r*rng.uniform(.4,.8)
        d.ellipse([q[0]-rr,q[1]-rr,q[0]+rr,q[1]+rr],fill=255)
# triangle (three spells): brushed, with blobby corners like the mark
A=(1000,330); B=(1560,1300); C=(440,1300)
stroke([C,A],46,40); stroke([A,B],42,48); stroke([(380,1300),(1650,1300)],44,30)
for p in (A,B,C): blob(p,62)
# eye ring inside (echoes the mark's ring), open at the top-right like a brush circle
th=np.linspace(np.radians(-60),np.radians(250),120); cx,cy,R=1000,930,175
stroke([(cx+R*np.cos(t),cy+R*np.sin(t)) for t in th],30,14,jitter=3,bristles=False)
blob((cx,cy),38)
d.line([(cx,cy),(cx-6,cy+70)],fill=255,width=14)          # little drip from the eye
# drips off the base line
for x in (520,700,1120,1290,1480):
    L=rng.uniform(150,420); w=rng.uniform(10,22)
    d.polygon([(x-w,1310),(x+w,1310),(x+rng.normal(0,4),1310+L)],fill=255)
# splatter
for k in range(420):
    ang=rng.uniform(0,2*np.pi); rad=rng.uniform(350,900)**1.0
    x=1000+np.cos(ang)*rad*rng.uniform(.5,1.1); y=950+np.sin(ang)*rad*rng.uniform(.4,.9)
    r=rng.pareto(2.2)*3+1.2
    if r<40: d.ellipse([x-r,y-r,x+r,y+r],fill=255)
# organic ink edges: blur + threshold with grain
img=ink.filter(ImageFilter.GaussianBlur(5)); a=np.array(img).astype(float)
grain=np.array(Image.fromarray((rng.random((S//8,S//8))*255).astype(np.uint8)).resize((S,S),Image.BICUBIC)).astype(float)
a=np.clip((a+ (grain-128)*0.35 - 110)*6,0,255)
# the glitch: two horizontal slices knocked sideways
for (y0,y1,dx) in ((640,700,46),(1010,1050,-58),(1180,1205,30)):
    a[y0:y1]=np.roll(a[y0:y1],dx,axis=1)
a=a.astype(np.uint8)
ys,xs=np.where(a>20); pad=30
box=(xs.min()-pad,ys.min()-pad,xs.max()+pad,ys.max()+pad)
rgba=np.zeros((S,S,4),np.uint8); rgba[...,:3]=(237,244,255); rgba[...,3]=a
out=Image.fromarray(rgba).crop(box); out.thumbnail((900,900)); out.save('logos/spell_sigil_light.png',optimize=True)
# preview on dark + on white
bg=Image.new('RGBA',out.size,(7,9,17,255)); bg.alpha_composite(out); bg.convert('RGB').save('sigil_preview.png')
print(out.size)
