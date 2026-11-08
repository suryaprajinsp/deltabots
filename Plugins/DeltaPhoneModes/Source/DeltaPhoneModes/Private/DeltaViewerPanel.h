#pragma once
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"

// Draw in source-eye coordinates. Touches are unwarped through the same lens
// equation before hit testing; Slate's rectangular screen hit boxes cannot do that.
class SDeltaViewerPanel : public SLeafWidget
{
public:
 SLATE_BEGIN_ARGS(SDeltaViewerPanel) {} SLATE_END_ARGS()
 TFunction<FString()> Description;
 TFunction<bool()> IsOpen;
 TFunction<bool()> ShowGrid;
 void Construct(const FArguments&) {}
 FVector2D ComputeDesiredSize(float) const override {return FVector2D(600,600);}
 int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool) const override
 {
  const FVector2D Size=G.GetLocalSize();
  auto Text=[&](FString Value,float X,float Y,int32 FontSize,FLinearColor Color)
  {
   auto Font=FCoreStyle::GetDefaultFontStyle("Bold",FontSize);
   const FVector2D Pos(X*Size.X,Y*Size.Y);
   FSlateDrawElement::MakeText(Out,Layer+1,G.ToPaintGeometry(FVector2D(1,1),FSlateLayoutTransform(Pos)),Value,Font,ESlateDrawEffect::None,Color);
  };
  auto Button=[&](const TCHAR* Value,float X,float Y)
  {
   const FVector2D Pos((X-.085f)*Size.X,(Y-.035f)*Size.Y),Box(.17f*Size.X,.07f*Size.Y);
   FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(Box,FSlateLayoutTransform(Pos)),FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,FLinearColor(.015,.08,.12,.94));
   Text(Value,X-.065f,Y-.015f,17,FLinearColor(.4,1,1));
  };
  if(ShowGrid&&ShowGrid())
  {
   const auto Brush=FCoreStyle::Get().GetBrush("WhiteBrush");
   for(int32 I=1;I<10;++I){float T=I*.1f;const FLinearColor C=I==5?FLinearColor(1,.8,.2,.8):FLinearColor(.1,.8,.9,.3);
    FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(FVector2D(1,Size.Y),FSlateLayoutTransform(FVector2D(T*Size.X,0))),Brush,ESlateDrawEffect::None,C);
    FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(FVector2D(Size.X,1),FSlateLayoutTransform(FVector2D(0,T*Size.Y))),Brush,ESlateDrawEffect::None,C);}
   Text(TEXT("ALIGN CENTER + HORIZON"),.25f,.86f,14,FLinearColor(.4,1,1));
  }
  Button(TEXT("MAP"),.5f,.25f);
  Button(TEXT("CENTER"),.3f,.18f);Button(TEXT("ADJUST"),.5f,.18f);Button(TEXT("EXIT VR"),.7f,.18f);
  if(IsOpen&&IsOpen())
  {
   FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(FVector2D(.65f*Size.X,.44f*Size.Y),FSlateLayoutTransform(FVector2D(.175f*Size.X,.29f*Size.Y))),FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,FLinearColor(.01,.025,.04,.98));
   Text(Description(),.21f,.33f,18,FLinearColor::White);
   Button(TEXT("PREV"),.3f,.55f);Button(TEXT("AUTO FIT"),.5f,.55f);Button(TEXT("NEXT"),.7f,.55f);
   Button(TEXT("-"),.3f,.66f);Button(TEXT("SAVE"),.5f,.66f);Button(TEXT("+"),.7f,.66f);
  }
  return Layer+1;
 }
};
