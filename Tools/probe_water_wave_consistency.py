"""CPU-only checks of the shared production water FFT/swell weighting.

Compiles the literal HLSL scalar helper as C++, then compares the weighted
height derivative with the normal's product rule. No Unreal or GPU is run;
this does not validate the remaining vector FFT/lateral-compression Jacobian.
"""
import argparse
import ast
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--vcvars', type=Path, default=Path('C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build/vcvars64.bat'))
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    source = (root / 'Tools/make_maelstrom_materials.py').read_text()
    tree = ast.parse(source)
    helper = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
                  and any(isinstance(t, ast.Name) and t.id == 'WAVE_BLEND' for t in n.targets))
    # Ensure the shared helper is actually used by both emitted graph paths.
    for text in ('normal_code = NOISE + FLOW + WAVE_BLEND', 'wpo_code = NOISE + FLOW + WAVE_BLEND',
                 'swellWeight=Tunnel<.5 ? waveBlend.swell(fftWeight)',
                 'wave=wave*waveBlend.swell(liveWeight)',
                 'swellHeight*ddx(largeWeight*swellWeight)+breakerHeight*ddx(largeWeight)',
                 'wave*=lerp(1.,1-saturate(VC.r*1.5),saturate(Tunnel))'):
        if text not in source:
            raise RuntimeError('Missing production weighting connection: ' + text)
    code = r'''
#include <algorithm>
#include <cmath>
#include <cstdio>
float saturate(float x){return std::clamp(x,0.f,1.f);}
float step(float a,float x){return x>=a?1.f:0.f;}
float smoothstep(float a,float b,float x){float u=saturate((x-a)/(b-a));return u*u*(3-2*u);}
'''+helper+r'''
double hull(double x){return smoothstep(1200,4000,float(x));}
double geomFade(double x){return 1-smoothstep(120000,350000,float(x));}
double mask(double x){return hull(x)*geomFade(x);}
double swellMask(double x){return mask(x)*waveBlend.swell(waveBlend.fft(float(x),1,8));}
double height(double x){return 400*std::sin(x*.0007)*swellMask(x)+190*std::sin(x*.0013+.7)*mask(x);}
double derivative(double x){
 // Same product rule as the material, independent finite differences of masks.
 const double e=.5;
 return 400*.0007*std::cos(x*.0007)*swellMask(x)+190*.0013*std::cos(x*.0013+.7)*mask(x)
  +400*std::sin(x*.0007)*(swellMask(x+e)-swellMask(x-e))/(2*e)
  +190*std::sin(x*.0013+.7)*(mask(x+e)-mask(x-e))/(2*e);
}
int main(){
 bool pass=true;double previous=1,maxError=0,oldError=0;
 for(int x=0;x<=400000;x+=31){
  double f=waveBlend.fft(float(x),1,8),s=waveBlend.swell(float(f));
  pass&=f>=0&&f<=1&&f<=previous+1e-6&&s>=.25&&s<=1;previous=f;
  pass&=waveBlend.fft(float(x),0,8)==0&&waveBlend.fft(float(x),1,0)==0;
  if(x<=12000)pass&=s==.25;if(x>=50000)pass&=s==1;
  const double e=.5,fd=(height(x+e)-height(x-e))/(2*e);
  maxError=std::max(maxError,std::abs(fd-derivative(x)));
  double noProduct=400*.0007*std::cos(x*.0007)*swellMask(x)+190*.0013*std::cos(x*.0013+.7)*mask(x);
  oldError=std::max(oldError,std::abs(fd-noProduct));
 }
 pass&=maxError<2e-6&&oldError>.05;
 double tunnelError=0;
 for(float w:{0.f,.4f,1.f})for(float h:{-400.f,0.f,400.f})for(float d:{-120.f,80.f,300.f}){
  double old=h*(1-w)+(h*.25+d)*w;
  double now=h*waveBlend.swell(w)+d*w;
  tunnelError=std::max(tunnelError,std::abs(old-now));
 }
 pass&=tunnelError<3e-5;
 std::printf("Shared FFT120-500m/swell.25-1 weights, disabled fallback and hull fade: %s\n",pass?"PASS":"FAIL");
 std::printf("Weighted-height normal product-rule max error %.9g; omitted product-rule error %.6f; tunnel algebra error %.9g\n",maxError,oldError,tunnelError);
 return pass?0:1;
}
'''
    out = root / 'Saved/GeometryProbe/WaterWaveConsistency'
    out.mkdir(parents=True, exist_ok=True)
    cpp, exe = out/'probe.cpp', out/'probe.exe'
    cpp.write_text(code)
    batch = out/'build.cmd'
    batch.write_text(f'@echo off\ncall "{args.vcvars}" >nul\nif errorlevel 1 exit /b %errorlevel%\ncl /nologo /std:c++17 /EHsc /O2 "{cpp}" /Fe:"{exe}" /Fo:"{out / "probe.obj"}"\n')
    subprocess.run(['cmd','/d','/c',str(batch)],cwd=out,check=True)
    result = subprocess.run([str(exe)],cwd=out,capture_output=True,text=True)
    (out/'results.txt').write_text(result.stdout+result.stderr)
    print(result.stdout,end='');print(result.stderr,end='',file=sys.stderr)
    raise SystemExit(result.returncode)


if __name__ == '__main__':
    main()
