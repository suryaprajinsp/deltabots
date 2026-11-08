#pragma once
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"

// One map definition shared by the flat display and both viewer eyes.
class SDeltaWorldNavigator : public SLeafWidget
{
public:
 SLATE_BEGIN_ARGS(SDeltaWorldNavigator) {} SLATE_END_ARGS()
 TFunction<bool()> IsOpen;
 TFunction<FVector()> PlayerPosition;
 void Construct(const FArguments&) {}
 FVector2D ComputeDesiredSize(float) const override {return FVector2D(600,600);}
 int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool) const override
 {
  if(!IsOpen||!IsOpen())return Layer;
  const FVector2D Size=G.GetLocalSize();
  const FLinearColor Cyan(.12f,.9f,1.f),Dim(.04f,.28f,.34f),White(.8f,.94f,1.f);
  auto Point=[&](float X,float Y){return FVector2D(Size.X*(.5f+Y/36000.f*.68f),Size.Y*(.52f-X/36000.f*.52f));};
  auto Line=[&](TArray<FVector2D> P,FLinearColor C,float Width){FSlateDrawElement::MakeLines(Out,Layer+2,G.ToPaintGeometry(),P,ESlateDrawEffect::None,C,true,Width);};
  auto Text=[&](FString Value,float X,float Y,int32 Font){FSlateDrawElement::MakeText(Out,Layer+3,G.ToPaintGeometry(FVector2D(1,1),FSlateLayoutTransform(FVector2D(X,Y))),Value,FCoreStyle::GetDefaultFontStyle("Bold",Font),ESlateDrawEffect::None,White);};
  FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(FVector2D(Size.X*.82f,Size.Y*.62f),FSlateLayoutTransform(FVector2D(Size.X*.09f,Size.Y*.25f))),FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,FLinearColor(.008f,.025f,.045f,.97f));
  Text(TEXT("NEXUS // ROUTE NETWORK"),Size.X*.16f,Size.Y*.28f,19);
  const float X[]={0,0,12000,12000,0,12000},Y[]={0,-12000,-12000,0,12000,12000};
  const TCHAR* Names[]={TEXT("NEXUS"),TEXT("DUNE"),TEXT("FROST"),TEXT("CANOPY"),TEXT("TIDAL"),TEXT("SKY PORT")};
  const int32 A[]={0,0,0,1,3,3},B[]={1,3,4,2,2,5};
  for(int32 I=0;I<6;++I)Line({Point(X[A[I]],Y[A[I]]),Point(X[B[I]],Y[B[I]])},Dim,4);
  for(int32 I=0;I<6;++I){auto P=Point(X[I],Y[I]);Line({P+FVector2D(0,-9),P+FVector2D(9,0),P+FVector2D(0,9),P+FVector2D(-9,0),P+FVector2D(0,-9)},Cyan,2);Text(Names[I],P.X+12,P.Y-6,14);}
  const FVector Loc=PlayerPosition?PlayerPosition():FVector::ZeroVector;
  const FVector2D P=Point(FMath::Clamp(float(Loc.X),-17000.f,17000.f),FMath::Clamp(float(Loc.Y),-17000.f,17000.f));
  Line({P+FVector2D(0,-12),P+FVector2D(8,8),P+FVector2D(0,4),P+FVector2D(-8,8),P+FVector2D(0,-12)},FLinearColor(1,.65f,.1f),3);
  Text(TEXT("ORANGE: YOU   /   CYAN: DISTRICTS"),Size.X*.16f,Size.Y*.79f,14);
  return Layer+3;
 }
};
