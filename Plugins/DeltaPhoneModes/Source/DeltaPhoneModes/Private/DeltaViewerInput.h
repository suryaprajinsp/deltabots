#pragma once
#include "Framework/Application/IInputProcessor.h"
#include "Input/Events.h"

// Observe the active Android gamepad before Slate focus / local-player routing.
// Native viewer controls must still work when gameplay look input is captured.
class FDeltaViewerInput : public IInputProcessor
{
public:
 TSet<FKey> Held,Pressed;
 TMap<FKey,float> Axes;
 TFunction<bool(FKey,bool)> RouteButton;
 TFunction<bool(FKey,float)> CaptureAnalog;
 void Tick(float,FSlateApplication&,TSharedRef<ICursor>) override {}
 bool HandleKeyDownEvent(FSlateApplication&,const FKeyEvent& E) override
 {
  if(!E.GetKey().IsGamepadKey())return false;
  if(!Held.Contains(E.GetKey()))Pressed.Add(E.GetKey());Held.Add(E.GetKey());
  return RouteButton?RouteButton(E.GetKey(),true):false;
 }
 bool HandleKeyUpEvent(FSlateApplication&,const FKeyEvent& E) override
 {
  if(!E.GetKey().IsGamepadKey())return false;
  Held.Remove(E.GetKey());return RouteButton?RouteButton(E.GetKey(),false):false;
 }
 bool HandleAnalogInputEvent(FSlateApplication&,const FAnalogInputEvent& E) override
 {
  if(!E.GetKey().IsGamepadKey())return false;
  Axes.Add(E.GetKey(),E.GetAnalogValue());return CaptureAnalog&&CaptureAnalog(E.GetKey(),E.GetAnalogValue());
 }
};
