"""Compile the actual shader's FLOW to check the authored river chart, CPU only."""
import ast
from pathlib import Path
import re
import subprocess
import sys

sys.dont_write_bytecode = True
from probe_maelstrom_geometry import SHIM, block


def main():
    root = Path(__file__).resolve().parents[1]
    tree = ast.parse((root/'Tools/make_maelstrom_materials.py').read_text())
    flow = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
                and any(isinstance(t, ast.Name) and t.id == 'FLOW' for t in n.targets))
    flow = re.sub(r'\.([xy])\b', lambda m: '.'+m[1].upper(), flow).replace('[unroll]', '')
    # Compile the current production mesh's exact point construction instead
    # of maintaining a second test-only chart that can drift from geometry.
    source = (root/'Source/DirectorsCut/DCMaelstrom.cpp').read_text()
    mesh = block(source, 'void ADCMaelstrom::BuildCataract')
    declarations=[]
    for name in ('Q', 'A', 'SideAngle', 'P'):
        match=re.search(r'const (?:float|FVector2D) '+name+r'\b[^;]+;', mesh)
        if not match:
            raise RuntimeError('Current BuildCataract point construction changed: '+name)
        declarations.append(match[0])
    native_point='float2 point(double r,double across){float R=float(r*100.),Across=float(across*100.),Radius=160000.;\n'
    native_point+='\n'.join(declarations)+'\nreturn P*.01;\n}\n'
    code = SHIM[:SHIM.index('struct FRandomStream')]
    code += r'''
struct Result {double r,across,s;float2 down,q,q2;};
Result chart(float2 UV,float T=0,float Cataract=1,float mask=1){
 float RadiusM=1600,Calm=1,Tunnel=0,Handedness=-1,Flow=1;
 struct {float r;}VC{mask};
'''+flow+r'''
 return {riverR,riverAcross,riverAlong,riverDown,q,q2};
}
'''+native_point+r'''
int main(){double radiusError=0,acrossError=0,directionError=0,joinError=0;int count=0;
 for(double r=832;r<=1728;r+=2)for(double across=-220;across<=220;across+=5){
  auto p=point(r,across);auto c=chart(p/100.);
  radiusError=max(radiusError,std::abs(c.r-r));acrossError=max(acrossError,std::abs(c.across-across));
  // Native float32 trigonometry needs a metre-wide difference. The analytic
  // tangent is allowed 0.12 degrees at the clamped smoothstep endpoints.
  auto down=normalize(point(r-.5,across)-point(r+.5,across));
  directionError=max(directionError,length(c.down-down));++count;
 }
 for(float t:{0.f,1.f,3.57f,7.14f,14.27f,300.f})for(double a:{-220.,0.,220.}){
  auto uv=point(928.,a)/100.;auto lake=chart(uv,t,0,0),foot=chart(uv,t,1,0);
  joinError=max(joinError,max(length(lake.q-foot.q),length(lake.q2-foot.q2)));
 }
 std::printf("%d chart points: R error %.6fm; across error %.6fm; tangent vector error %.8f; lake/foot wave-phase difference %.9fm\n",count,radiusError,acrossError,directionError,joinError);
 return radiusError<.005 && acrossError<.005 && directionError<.002 && joinError<1.e-6 ? 0:1;
}
'''
    out = root/'Saved/CataractChartProbe'
    out.mkdir(parents=True, exist_ok=True)
    cpp=out/'chart.cpp';cpp.write_text(code)
    exe=out/'chart.exe'; batch=out/'build.cmd'
    batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build/vcvars64.bat" >nul\n'
        f'cl /nologo /std:c++17 /EHsc /O2 "{cpp}" /Fe:"{exe}" /Fo:"{out/"chart.obj"}"\n')
    subprocess.run(['cmd','/d','/c',str(batch)],cwd=out,check=True)
    raise SystemExit(subprocess.run([str(exe)],cwd=out).returncode)


if __name__ == '__main__':
    main()
