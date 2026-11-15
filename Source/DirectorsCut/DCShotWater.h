#pragma once
#include <algorithm>
#include <cmath>

/** Engine-independent first-entry sweep; the height callback is the rendered surface. */
namespace DCShotWater
{
    template<class Height>
    bool FirstEntry(double X0,double Y0,double Z0,double X1,double Y1,double Z1,Height Surface,double& Fraction)
    {
        auto Depth=[&](double T) { return Z0+(Z1-Z0)*T-Surface(X0+(X1-X0)*T,Y0+(Y1-Y0)*T); };
        if(Depth(0)<=0) { Fraction=0; return true; }
        const double Length=std::sqrt((X1-X0)*(X1-X0)+(Y1-Y0)*(Y1-Y0)+(Z1-Z0)*(Z1-Z0));
        const int Steps=std::max(1,int(std::ceil(Length/200.)));
        double Above=0;
        for(int I=1;I<=Steps;++I)
        {
            double Below=double(I)/Steps;
            if(Depth(Below)>0) { Above=Below; continue; }
            for(int J=0;J<12;++J)
            {
                const double Middle=(Above+Below)*.5;
                if(Depth(Middle)>0) { Above=Middle; } else { Below=Middle; }
            }
            Fraction=Below; return true;
        }
        return false;
    }
    inline double Lifetime(double MuzzleZ,double SeaZ,double VelocityZ,double Gravity,double Flight)
    {
        const double Return=(VelocityZ+std::sqrt(VelocityZ*VelocityZ+2*Gravity*std::max(0.,MuzzleZ-SeaZ)))/Gravity;
        return std::min(15.,std::max(Flight+.8,Return+2.));
    }
    inline bool FreshCapture(double Now,double Age,double FirstQueryTime)
    { return Age>=0 && Age<=.25 && Now-Age>=FirstQueryTime-.001; }
    inline double ProjectHeight(double Height,double NX,double NY,double NZ,double DX,double DY,double VZ,double Age)
    { return Height-(NX*DX+NY*DY)/std::max(NZ,.25)+std::clamp(VZ*Age,-100.,100.); }
}
