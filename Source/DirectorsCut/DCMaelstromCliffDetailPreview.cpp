#include "DCMaelstrom.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

namespace
{
bool ReadDetailJson(const FString& Path,TSharedPtr<FJsonObject>& Out)
{
    FString Text;
    return FFileHelper::LoadFileToString(Text,*Path)
        &&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Out)&&Out.IsValid();
}
bool DetailVector(const TSharedPtr<FJsonObject>& Object,const TCHAR* Key,FVector& Out)
{
    const TArray<TSharedPtr<FJsonValue>>* A=nullptr;
    if(!Object->TryGetArrayField(Key,A)||A->Num()!=3) { return false; }
    double X,Y,Z;
    if(!(*A)[0]->TryGetNumber(X)||!(*A)[1]->TryGetNumber(Y)||!(*A)[2]->TryGetNumber(Z)) { return false; }
    Out=FVector(X,Y,Z); return !Out.ContainsNaN();
}
}

void ADCMaelstrom::UpdateCliffDetailPreview()
{
    const bool bScanDemo=FParse::Param(FCommandLine::Get(),TEXT("DCCliffScanDemo"));
    if((!bScanDemo&&!FParse::Param(FCommandLine::Get(),TEXT("DCCliffDetailPreview")))
        ||!FParse::Param(FCommandLine::Get(),TEXT("DCCliffPreview"))||!CliffPreviewMesh||!bActive||Elapsed<0.f)
    { ClearCliffDetailPreview(); return; }
    if(bScanDemo)
    {
        // A small authored assembly of installed scans over the existing backed
        // preview. These decorations do not replace its contact/support surface.
        // Unlike FBX authoring charts, these transforms are directly arena cm.
        if(!bCliffDetailPreviewTried)
        {
            bCliffDetailPreviewTried=true;
            TSharedPtr<FJsonObject> Data;
            FString Schema;
            const TArray<TSharedPtr<FJsonValue>>* Instances=nullptr;
            if(!ReadDetailJson(FPaths::ProjectSavedDir()/TEXT("CliffScanDemo.json"),Data)
                ||!Data->TryGetStringField(TEXT("schema"),Schema)||Schema!=TEXT("directorscut.cliff_scan_demo.v1")
                ||!Data->TryGetArrayField(TEXT("instances"),Instances)||Instances->IsEmpty()||Instances->Num()>256)
            { UE_LOG(LogTemp,Warning,TEXT("DC_CLIFF_SCAN_DEMO missing/invalid CliffScanDemo.json")); return; }
            TMap<FString,UInstancedStaticMeshComponent*> Groups;
            int32 Added=0,Rejected=0;
            for(const TSharedPtr<FJsonValue>& Value:*Instances)
            {
                const TSharedPtr<FJsonObject>* Entry=nullptr;
                FString Asset,MaterialPath;
                FVector Position,Rotation,Scale;
                if(!Value->TryGetObject(Entry)||!(*Entry)->TryGetStringField(TEXT("mesh"),Asset)
                    ||!Asset.StartsWith(TEXT("/Game/"))
                    ||!DetailVector(*Entry,TEXT("position_cm"),Position)
                    ||!DetailVector(*Entry,TEXT("rotation_degrees"),Rotation)
                    ||!DetailVector(*Entry,TEXT("scale"),Scale)||Scale.GetMin()<=0.)
                { ++Rejected; continue; }
                if((*Entry)->HasField(TEXT("material"))
                    &&(!(*Entry)->TryGetStringField(TEXT("material"),MaterialPath)||!MaterialPath.StartsWith(TEXT("/Game/"))))
                { ++Rejected; continue; }
                const FString Key=Asset+TEXT("|")+MaterialPath;
                UInstancedStaticMeshComponent* Component=Groups.FindRef(Key);
                if(!Component)
                {
                    UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,*Asset);
                    UMaterialInterface* Material=MaterialPath.IsEmpty()?nullptr:LoadObject<UMaterialInterface>(nullptr,*MaterialPath);
                    if(!Mesh||(!MaterialPath.IsEmpty()&&!Material))
                    {
                        UE_LOG(LogTemp,Warning,TEXT("DC_CLIFF_SCAN_DEMO unavailable mesh=%s material=%s"),*Asset,*MaterialPath);
                        ++Rejected; continue;
                    }
                    Component=NewObject<UInstancedStaticMeshComponent>(this);
                    Component->SetupAttachment(Arena);
                    Component->SetMobility(EComponentMobility::Movable);
                    Component->SetStaticMesh(Mesh);
                    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                    Component->SetGenerateOverlapEvents(false);
                    Component->SetCanEverAffectNavigation(false);
                    Component->SetCastShadow(true);
                    // Preserve every imported slot/UV unless the manifest
                    // explicitly requests one material across the whole scan.
                    if(Material) for(int32 Slot=0;Slot<Mesh->GetStaticMaterials().Num();++Slot)
                    { Component->SetMaterial(Slot,Material); }
                    Component->RegisterComponent();
                    CliffDetailPreviewMeshes.Add(Component);
                    Groups.Add(Key,Component);
                }
                const FQuat Orientation=FRotator(Rotation.X,Rotation.Y,Rotation.Z).Quaternion();
                Component->AddInstance(FTransform(Orientation,Position,Scale),false);
                ++Added;
            }
            UE_LOG(LogTemp,Display,TEXT("DC_CLIFF_SCAN_DEMO ready instances=%d groups=%d rejected=%d arenaRelativeCm=1 runtimeContactsUnchanged=1"),Added,Groups.Num(),Rejected);
        }
        for(UInstancedStaticMeshComponent* Mesh:CliffDetailPreviewMeshes) { if(Mesh) { Mesh->SetVisibility(true); } }
        return;
    }
    if(!bCliffDetailPreviewTried)
    {
        bCliffDetailPreviewTried=true;
        const FString Saved=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
        TSharedPtr<FJsonObject> Data,Active,Sidecar;
        FString Schema,ExpectedFBX,ActualFBX,ExpectedSource,ActualSource,SidecarPath;
        if(!ReadDetailJson(Saved/TEXT("CliffDetailPreview.json"),Data)
            ||!ReadDetailJson(Saved/TEXT("CliffLandformPreviewImport.json"),Active)
            ||!Data->TryGetStringField(TEXT("schema"),Schema)||Schema!=TEXT("directorscut.cliff_detail_preview.v1")
            ||!Data->TryGetStringField(TEXT("cliff_fbx_sha256"),ExpectedFBX)||ExpectedFBX.Len()!=64
            ||!Active->TryGetStringField(TEXT("source_sha256"),ActualFBX)||ActualFBX!=ExpectedFBX
            ||!Data->TryGetStringField(TEXT("source_candidate_sha256"),ExpectedSource)||ExpectedSource.Len()!=64
            ||!Active->TryGetStringField(TEXT("export_sidecar"),SidecarPath)||!ReadDetailJson(SidecarPath,Sidecar)
            ||!Sidecar->TryGetStringField(TEXT("input_sha256"),ActualSource)||ActualSource!=ExpectedSource)
        { UE_LOG(LogTemp,Warning,TEXT("DC_CLIFF_DETAIL rejected stale/missing active preview contract")); return; }
        const TArray<TSharedPtr<FJsonValue>>* Assets=nullptr,*Instances=nullptr;
        if(!Data->TryGetArrayField(TEXT("assets"),Assets)||Assets->Num()!=3
            ||!Data->TryGetArrayField(TEXT("instances"),Instances)||Instances->IsEmpty()||Instances->Num()>100) { return; }
        TArray<UStaticMesh*> Meshes;
        for(int32 Index=0;Index<3;++Index)
        {
            const TSharedPtr<FJsonObject>* Entry=nullptr; FString Asset; FVector Size;
            const FString Expected=FString::Printf(TEXT("/Game/AncientRuins/Meshes/Cliffs/SM_Cliff_%02d.SM_Cliff_%02d"),Index+1,Index+1);
            if(!(*Assets)[Index]->TryGetObject(Entry)||!(*Entry)->TryGetStringField(TEXT("asset"),Asset)
                ||Asset!=Expected||!DetailVector(*Entry,TEXT("size_m"),Size)) { return; }
            UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,*Asset);
            if(!Mesh||!((Mesh->GetBoundingBox().GetSize()*.01)-Size).IsNearlyZero(.05)) { return; }
            Meshes.Add(Mesh);
        }
        for(UStaticMesh* Mesh:Meshes)
        {
            UInstancedStaticMeshComponent* Component=NewObject<UInstancedStaticMeshComponent>(this);
            Component->SetupAttachment(Arena);
            Component->SetMobility(EComponentMobility::Movable);
            Component->SetStaticMesh(Mesh); // Preserve every installed default material/UV.
            Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Component->SetGenerateOverlapEvents(false);
            Component->SetCanEverAffectNavigation(false);
            Component->SetCastShadow(true);
            Component->RegisterComponent();
            CliffDetailPreviewMeshes.Add(Component);
        }
        int32 Added=0;
        for(const TSharedPtr<FJsonValue>& Value:*Instances)
        {
            const TSharedPtr<FJsonObject>* Instance=nullptr;
            FVector Point,Normal; int32 AssetIndex=-1;
            double Yaw=0.,Scale=1.,Relief=0.;
            if(!Value->TryGetObject(Instance)||!(*Instance)->TryGetNumberField(TEXT("asset_index"),AssetIndex)
                ||!Meshes.IsValidIndex(AssetIndex)||!DetailVector(*Instance,TEXT("surface_arena_m"),Point)
                ||!DetailVector(*Instance,TEXT("normal_arena"),Normal)
                ||!(*Instance)->TryGetNumberField(TEXT("yaw_degrees"),Yaw)||!FMath::IsFinite(Yaw)
                ||!(*Instance)->TryGetNumberField(TEXT("uniform_scale"),Scale)||!FMath::IsFinite(Scale)||Scale<.85||Scale>1.05
                ||!(*Instance)->TryGetNumberField(TEXT("relief_m"),Relief)||!FMath::IsFinite(Relief)||Relief<1.||Relief>3.
                ||!FMath::IsNearlyEqual(Normal.SizeSquared(),1.,1.e-5)||FMath::Abs(Normal.Z)>.55
                ||Point.Z<-75.||Point.Z>25.) { continue; }
            const FQuat Rotation=FRotator(0.,Yaw,0.).Quaternion();
            const FBox Box=Meshes[AssetIndex]->GetBoundingBox();
            const FVector InLocal=Rotation.UnrotateVector(Normal).GetAbs();
            const double DepthCm=FVector::DotProduct(InLocal,Box.GetSize())*Scale;
            const double Buried=1.-Relief*100./DepthCm;
            if(Buried<.60-1.e-6||Buried>.80+1.e-6) { continue; }
            const FVector Center=Point*100.-Normal*(DepthCm*.5-Relief*100.);
            const FVector Origin=Center-Rotation.RotateVector(Box.GetCenter()*Scale);
            CliffDetailPreviewMeshes[AssetIndex]->AddInstance(FTransform(Rotation,Origin,FVector(Scale)),false);
            ++Added;
        }
        UE_LOG(LogTemp,Display,TEXT("DC_CLIFF_DETAIL ready instances=%d nativeScale=.85..1.05 relief=1..3m defaultMaterials=1 source=%s"),Added,*ExpectedSource);
    }
    for(UInstancedStaticMeshComponent* Mesh:CliffDetailPreviewMeshes) { if(Mesh) { Mesh->SetVisibility(true); } }
}

void ADCMaelstrom::ClearCliffDetailPreview()
{
    // Retain loaded native assets and transforms for backward review seeks.
    for(UInstancedStaticMeshComponent* Mesh:CliffDetailPreviewMeshes) { if(Mesh) { Mesh->SetVisibility(false); } }
}
