#pragma once
// Minimal math adapter ONLY for the standalone CPU replay. Runtime uses Unreal's CoreMinimal.
#include <algorithm>
#include <cmath>
using int32=int;
constexpr float UE_PI=3.14159265358979323846f,UE_TWO_PI=2.f*UE_PI;
struct FVector2D
{
    double X=0,Y=0;
    FVector2D()=default;
    FVector2D(double x,double y):X(x),Y(y){}
    FVector2D operator+(const FVector2D& B)const{return {X+B.X,Y+B.Y};}
    FVector2D operator-(const FVector2D& B)const{return {X-B.X,Y-B.Y};}
    FVector2D operator*(double S)const{return {X*S,Y*S};}
    FVector2D operator/(double S)const{return {X/S,Y/S};}
    double Size()const{return std::hypot(X,Y);}
    FVector2D GetSafeNormal()const{return Size()>1.e-8?*this/Size():FVector2D();}
};
struct FMath
{
    static float Clamp(float V,float A,float B){return std::clamp(V,A,B);}
    static int Max(int A,int B){return std::max(A,B);}
    static float Max(float A,float B){return std::max(A,B);}
    static float Min(float A,float B){return std::min(A,B);}
    static int CeilToInt(float A){return int(std::ceil(A));}
    static float Exp(float A){return std::exp(A);}
    static float UnwindRadians(float A){return std::remainder(A,UE_TWO_PI);}
    static float Abs(float A){return std::abs(A);}
    static float Sign(float A){return A>0?1.f:(A<0?-1.f:0.f);}
    static float Lerp(float A,float B,float U){return A+(B-A)*U;}
    static float SmoothStep(float A,float B,float V){float U=Clamp((V-A)/(B-A),0.f,1.f);return U*U*(3.f-2.f*U);}
    static float FInterpTo(float A,float B,float DT,float Speed){return std::abs(B-A)<1.e-4f?B:A+(B-A)*Clamp(DT*Speed,0.f,1.f);}
    static float Sin(float A){return std::sin(A);}
    static float Cos(float A){return std::cos(A);}
    static float Fmod(float A,float B){return std::fmod(A,B);}
    static float Atan2(float A,float B){return std::atan2(A,B);}
    static float RadiansToDegrees(float A){return A*180.f/UE_PI;}
    static float FindDeltaAngleDegrees(float A,float B){return std::remainder(B-A,360.f);}
};
