"""Replay recorded missed route shots against actual 0.6-scaled collision radius."""
from pathlib import Path
from math import sqrt
root=Path(__file__).resolve().parents[1]
gun=(root/'Source/DirectorsCut/DCGunnery.cpp').read_text()
boss=(root/'Source/DirectorsCut/DCBossCombat.cpp').read_text()
assert 'Hostile->ActorHasTag(TEXT("DCMovingWeakPoint"))' in gun
assert 'Movement/WorldDT' in boss and 'DT>0.f && DT<.5f' in boss
assert 'A->GetRootComponent()->ComponentVelocity=FVector::ZeroVector' in boss
assert 'Point + Velocity * Flight' in gun and 'GetHostileRadius(I) + 15.f' in gun
muzzle=(-110.19,530.29,580.76)
recorded=[(9181.34,8462.15,1515.63),(6534.00,8589.72,1575.49),(3939.56,8644.84,1555.67)]
add=lambda a,b:tuple(x+y for x,y in zip(a,b))
sub=lambda a,b:tuple(x-y for x,y in zip(a,b))
mul=lambda a,s:tuple(x*s for x in a)
length=lambda a:sqrt(sum(x*x for x in a))
flight=lambda p:max(.5,min(3,length(sub(p,muzzle))/7000))
checks=0
for edge in recorded:
 # ResolveAim previously selected the near sphere surface; infer its centre
 # along the sight ray. Allow camera/muzzle parallax below in the replay.
 centre=add(edge,mul(sub(edge,muzzle),855/length(sub(edge,muzzle))))
 for fps in [15,30,60,120]:
  for speed in [1,2,3]:
   for parallax in [-150,0,150]:
    point=add(centre,(0,parallax,0))
    velocity=(-880*speed,42*speed,0)
    # Exact existing two-pass prediction used by ResolveAim.
    aim=add(point,mul(velocity,flight(point)))
    aim=add(point,mul(velocity,flight(aim)))
    def closest(target):
     duration=flight(target)
     delta=sub(target,muzzle)
     launch=add(mul(delta,1/duration),(0,0,700*duration))
     best=1e9
     for i in range(1,int((duration+.6)*fps)+1):
      t=i/fps
      ball=add(add(muzzle,mul(launch,t)),(0,0,-700*t*t))
      moving=add(point,mul(velocity,t))
      best=min(best,length(sub(ball,moving)))
     return best
    led=closest(aim)
    assert led<855,(edge,fps,speed,parallax,led)
    checks+=1
    if speed==1 and parallax==0:
     old=closest(edge)
     assert old>855,(edge,fps,old)
     checks+=1
print(f'PASS {checks} recorded-shot trajectory cases: old edge aim misses; moving-centre lead hits unchanged855cm radius')
