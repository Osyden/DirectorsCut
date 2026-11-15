#include "DCCrewNavigation.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCCrewNavigationTest,"DirectorsCut.Crew.DeckRoutes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCCrewNavigationTest::RunTest(const FString&)
{
	TestFalse(TEXT("Deck-wide static rope assembly is not a solid cannon"),FDCCrewNavigation::IsCompactFitting(TEXT("SM_Rope-cannon-support"),false));
	TestTrue(TEXT("Individual gun keeps its solid clearance"),FDCCrewNavigation::IsCompactFitting(TEXT("SK_Cannon_2-L9"),true));
	TestTrue(TEXT("Wheel keeps its solid clearance"),FDCCrewNavigation::IsCompactFitting(TEXT("SK_Wheel"),true));
	TestFalse(TEXT("Whole-rig mast physics is handled by shaft footprints"),FDCCrewNavigation::IsCompactFitting(TEXT("SK_Mast"),true));
	FDCCrewNavigation Graph;
	auto Node=[&](FVector Feet) { FDCCrewNavigation::FNode N; N.Feet=Feet; return Graph.Nodes.Add(MoveTemp(N)); };
	auto Link=[&](int32 A,int32 B) { Graph.Nodes[A].Links.Add(B); Graph.Nodes[B].Links.Add(A); };
	// Opposite sides of a cabin: only the side aisle connects them. The elevated
	// quarterdeck is reachable through the stairs, never a diagonal wall crossing.
	const int32 Start=Node(FVector(0,0,500)), Aisle=Node(FVector(0,160,500));
	const int32 Around=Node(FVector(240,160,500)), Goal=Node(FVector(240,0,500));
	Link(Start,Aisle); Link(Aisle,Around); Link(Around,Goal);
	int32 Previous=Around;
	for (int32 I=1; I<=4; ++I) { const int32 Step=Node(FVector(240+I*40,160,500+I*40)); Link(Previous,Step); Previous=Step; }
	const int32 Isolated=Node(FVector(40,0,500));
	TArray<int32> Route;
	TestTrue(TEXT("Routes around cabin through side aisle"),Graph.FindNodeRoute(Start,Goal,Route));
	TestEqual(TEXT("Wall detour visits four nodes"),Route.Num(),4);
	if (Route.Num()==4) { TestEqual(TEXT("Uses aisle instead of wall"),Route[1],Aisle); }
	TestTrue(TEXT("Stairs connect quarterdeck"),Graph.FindNodeRoute(Start,Previous,Route));
	TestEqual(TEXT("Climbs all four stairs"),Route.Num(),7);
	TestFalse(TEXT("No route never falls back to a direct line"),Graph.FindNodeRoute(Start,Isolated,Route));
	TestTrue(TEXT("Failed routes discard stale waypoints"),Route.IsEmpty());
	TestFalse(TEXT("Bad station index rejected"),Graph.FindNodeRoute(INDEX_NONE,Goal,Route));
	FVector Feet;
	TestTrue(TEXT("Correct an initially sunk foot placement"),Graph.Project(FVector(0,0,420),Feet));
	TestEqual(TEXT("Feet sit on the upper deck"),Feet.Z,500.0);
	TestFalse(TEXT("Cannot project a distant point onto the ship"),Graph.Project(FVector(5000,0,500),Feet));
	TArray<FVector> Points;
	TestTrue(TEXT("Grounded station requests use the same route"),Graph.FindRoute(FVector(0,0,480),FVector(240,0,480),Points));
	TestEqual(TEXT("Projected route retains cabin detour"),Points.Num(),4);
	return true;
}
#endif
