#include "DCCrewNavigation.h"
#include "DCRideDirector.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include <initializer_list>

namespace
{
	constexpr float Cell = 40.f;
	constexpr float Radius = 24.f;
	constexpr float MaxStep = 50.f;

	struct FDeckSampler
	{
		FTransform Deck;
		UPrimitiveComponent* Hull = nullptr;
		TArray<UPrimitiveComponent*> Parts;
		FCollisionQueryParams Query { FName(TEXT("CrewDeckNavigation")), true };

		bool Floor(float X, float Y, FVector& Feet) const
		{
			FHitResult Hit;
			// The brigantine's upper deck is about 500 cm, quarterdeck about 700.
			// Query its actual triangles, not Visibility-channel props or the gun deck.
			if (!Hull->LineTraceComponent(Hit, Deck.TransformPosition(FVector(X,Y,1050)), Deck.TransformPosition(FVector(X,Y,420)), Query)) { return false; }
			Feet = Deck.InverseTransformPosition(Hit.ImpactPoint);
			return Feet.Z >= 440.f && Feet.Z <= 850.f && Deck.InverseTransformVectorNoScale(Hit.ImpactNormal).Z > .65f;
		}

		bool ClearLine(const FVector& A, const FVector& B) const
		{
			const FVector Start = Deck.TransformPosition(A), End = Deck.TransformPosition(B);
			FBox Segment(ForceInit); Segment += Start; Segment += End;
			for (UPrimitiveComponent* Part : Parts)
			{
				if (!Part->Bounds.GetBox().Intersect(Segment.ExpandBy(1.f))) { continue; }
				FHitResult Hit;
				if (Part->LineTraceComponent(Hit, Start, End, Query)) { return false; }
			}
			return true;
		}

		bool Standing(const FVector& Feet) const
		{
			for (UPrimitiveComponent* Part : Parts)
			{
				const FString Name=Part->GetName();
				if (!Name.Contains(TEXT("Cannon")) && !Name.Contains(TEXT("Anchor-support")) && Name!=TEXT("SK_Wheel") && Name!=TEXT("SK_Door_Brigantine")) { continue; }
				// These compact solid fittings must also reject a sample wholly
				// inside them, where a surface ray could start behind every triangle.
				const FBox Box=Part->CalcBounds(Part->GetComponentTransform()*Deck.Inverse()).GetBox();
				if (Feet.Z+170.f>Box.Min.Z && Feet.Z+25.f<Box.Max.Z &&
					Feet.X>Box.Min.X-Radius && Feet.X<Box.Max.X+Radius && Feet.Y>Box.Min.Y-Radius && Feet.Y<Box.Max.Y+Radius) { return false; }
			}
			for (const FVector2D Offset : { FVector2D(Radius,0), FVector2D(-Radius,0), FVector2D(0,Radius), FVector2D(0,-Radius) })
			{
				FVector Edge;
				if (!Floor(Feet.X+Offset.X, Feet.Y+Offset.Y, Edge) || FMath::Abs(Edge.Z-Feet.Z)>MaxStep) { return false; }
			}
			// Shoulder-width clearance excludes masts, cabin walls, wheels and gun
			// carriages even when their tops are not part of the floor mesh.
			for (const float Height : { 55.f, 110.f, 170.f })
			{
				const FVector At = Feet + FVector(0,0,Height);
				if (!ClearLine(At-FVector(Radius,0,0),At+FVector(Radius,0,0)) ||
					!ClearLine(At-FVector(0,Radius,0),At+FVector(0,Radius,0))) { return false; }
			}
			return ClearLine(Feet+FVector(0,0,55), Feet+FVector(0,0,180));
		}

		bool Connected(const FVector& A, const FVector& B) const
		{
			if (FMath::Abs(A.Z-B.Z)>MaxStep) { return false; }
			FVector Mid;
			if (!Floor((A.X+B.X)*.5f,(A.Y+B.Y)*.5f,Mid) || FMath::Abs(Mid.Z-(A.Z+B.Z)*.5f)>MaxStep*.6f) { return false; }
			const FVector Side = FVector::CrossProduct((B-A).GetSafeNormal2D(),FVector::UpVector)*Radius;
			for (const float Height : { 55.f, 150.f })
			{
				for (const float Offset : { -1.f, 0.f, 1.f })
				{
					if (!ClearLine(A+FVector(0,0,Height)+Side*Offset, B+FVector(0,0,Height)+Side*Offset)) { return false; }
				}
			}
			return true;
		}
	};
}

bool FDCCrewNavigation::Build(ADCRideDirector* Director)
{
	Nodes.Reset();
	if (!Director || !Director->ShipActor) { return false; }
	FDeckSampler Sampler;
	Sampler.Deck = Director->DeckTransform;
	TInlineComponentArray<UPrimitiveComponent*> Components(Director->ShipActor);
	for (UPrimitiveComponent* Part : Components)
	{
		if (Part->GetName()==TEXT("HullShip")) { Sampler.Hull=Part; }
		if ((!Cast<UStaticMeshComponent>(Part) && !Cast<USkeletalMeshComponent>(Part)) || !Part->IsVisible() || Part->bHiddenInGame) { continue; }
		// The water/hull proxy is a coarse buoyancy shell, not the deck surface.
		if (Part->GetName()==TEXT("SM_Root") || Part->GetName().Contains(TEXT("water_collision"))) { continue; }
		Sampler.Parts.Add(Part);
	}
	if (!Sampler.Hull) { return false; }
	Hull=Sampler.Hull;
	TMap<FIntPoint,int32> Grid;
	// Keep the walkable graph on the vessel itself: no bowsprit, railing or net.
	for (int32 X=-47; X<=35; ++X)
	{
		for (int32 Y=-11; Y<=11; ++Y)
		{
			FVector Feet;
			if (!Sampler.Floor(X*Cell,Y*Cell,Feet) || !Sampler.Standing(Feet)) { continue; }
			FNode Node; Node.Feet=Feet+FVector(0,0,2.f);
			Grid.Add(FIntPoint(X,Y),Nodes.Add(MoveTemp(Node)));
		}
	}
	for (const auto& Pair : Grid)
	{
		for (const FIntPoint Offset : { FIntPoint(1,0), FIntPoint(0,1), FIntPoint(1,1), FIntPoint(1,-1) })
		{
			const int32* Other=Grid.Find(Pair.Key+Offset);
			if (!Other) { continue; }
			// No cutting diagonally between two blocked corners.
			if (Offset.X && Offset.Y && (!Grid.Contains(Pair.Key+FIntPoint(Offset.X,0)) || !Grid.Contains(Pair.Key+FIntPoint(0,Offset.Y)))) { continue; }
			if (!Sampler.Connected(Nodes[Pair.Value].Feet,Nodes[*Other].Feet)) { continue; }
			Nodes[Pair.Value].Links.Add(*Other); Nodes[*Other].Links.Add(Pair.Value);
		}
	}
	return !Nodes.IsEmpty();
}

int32 FDCCrewNavigation::Closest(const FVector& Feet, float MaxDistance) const
{
	int32 Best=INDEX_NONE;
	float Score=FMath::Square(MaxDistance);
	for (int32 I=0; I<Nodes.Num(); ++I)
	{
		// Accept an old bad floor trace without selecting the other deck through a
		// cabin roof: horizontal distance dominates only within one walkable level.
		if (FMath::Abs(Nodes[I].Feet.Z-Feet.Z)>260.f) { continue; }
		const float D=FVector::DistSquared(Nodes[I].Feet,Feet);
		if (D<Score) { Score=D; Best=I; }
	}
	return Best;
}

bool FDCCrewNavigation::Project(const FVector& Feet, FVector& OnDeck, float MaxDistance) const
{
	const int32 Node=Closest(Feet,MaxDistance);
	if (Node==INDEX_NONE) { return false; }
	OnDeck=Nodes[Node].Feet; return true;
}

bool FDCCrewNavigation::Ground(ADCRideDirector* Director, FVector& Feet) const
{
	if (!Director || !Hull.IsValid()) { return false; }
	const FTransform& Deck=Director->DeckTransform;
	FCollisionQueryParams Query(TEXT("CrewFeet"),true);
	FHitResult Hit;
	if (!Hull->LineTraceComponent(Hit,Deck.TransformPosition(Feet+FVector(0,0,65)),Deck.TransformPosition(Feet-FVector(0,0,90)),Query)) { return false; }
	if (Deck.InverseTransformVectorNoScale(Hit.ImpactNormal).Z<=.65f) { return false; }
	Feet.Z=Deck.InverseTransformPosition(Hit.ImpactPoint).Z+2.f;
	return true;
}

bool FDCCrewNavigation::FindNodeRoute(int32 Start, int32 Goal, TArray<int32>& Route) const
{
	Route.Reset();
	if (!Nodes.IsValidIndex(Start) || !Nodes.IsValidIndex(Goal)) { return false; }
	TArray<float> Cost; Cost.Init(BIG_NUMBER,Nodes.Num());
	TArray<int32> Parent; Parent.Init(INDEX_NONE,Nodes.Num());
	TArray<bool> Closed; Closed.Init(false,Nodes.Num());
	TArray<int32> Open { Start }; Cost[Start]=0;
	while (!Open.IsEmpty())
	{
		int32 BestSlot=0;
		float BestScore=BIG_NUMBER;
		for (int32 I=0; I<Open.Num(); ++I)
		{
			const int32 N=Open[I];
			const float Score=Cost[N]+FVector::Distance(Nodes[N].Feet,Nodes[Goal].Feet);
			if (Score<BestScore) { BestScore=Score; BestSlot=I; }
		}
		const int32 Current=Open[BestSlot]; Open.RemoveAtSwap(BestSlot);
		if (Current==Goal)
		{
			for (int32 N=Goal; N!=INDEX_NONE; N=Parent[N]) { Route.Insert(N,0); }
			return true;
		}
		Closed[Current]=true;
		for (const int32 Next : Nodes[Current].Links)
		{
			if (!Nodes.IsValidIndex(Next) || Closed[Next]) { continue; }
			const float NewCost=Cost[Current]+FVector::Distance(Nodes[Current].Feet,Nodes[Next].Feet);
			if (NewCost>=Cost[Next]) { continue; }
			Cost[Next]=NewCost; Parent[Next]=Current; Open.AddUnique(Next);
		}
	}
	return false;
}

bool FDCCrewNavigation::FindRoute(const FVector& FromFeet, const FVector& ToFeet, TArray<FVector>& Route) const
{
	Route.Reset();
	TArray<int32> Indices;
	if (!FindNodeRoute(Closest(FromFeet,300.f),Closest(ToFeet,300.f),Indices)) { return false; }
	for (const int32 I : Indices) { Route.Add(Nodes[I].Feet); }
	return true;
}
