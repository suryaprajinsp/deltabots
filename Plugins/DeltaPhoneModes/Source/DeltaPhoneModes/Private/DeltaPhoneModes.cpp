#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "UObject/UnrealType.h"
#include "StereoRendering.h"
#include "StereoRenderTargetManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/ScopeLock.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#include "DeltaLens.h"
#include "DeltaViewerPanel.h"
#include "DeltaViewerInput.h"
#include "DeltaWorldNavigator.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerInput.h"
#include <atomic>

DEFINE_LOG_CATEGORY_STATIC(LogDeltaPhoneModes, Log, All);
static const TCHAR* SettingsSection=TEXT("DeltaBots.PhoneViewer");

class FDeltaPhoneStereo final : public IStereoRendering, public IStereoRenderTargetManager
{
public:
 std::atomic<bool> bEnabled{false};
 float IPD=0.064f,FOV=100;
 mutable float Aspect=1;
 mutable FCriticalSection LensLock;
 FDeltaLensSettings Lens;
 FDeltaLensSettings GetLens() const { FScopeLock Lock(&LensLock);return Lens; }
 void SetLens(FDeltaLensSettings Value){FScopeLock Lock(&LensLock);Lens=Value;}
 bool IsStereoEnabled() const override {return bEnabled.load();}
 bool EnableStereo(bool On) override {bEnabled.store(On);return On;}
 IStereoRenderTargetManager* GetRenderTargetManager() override {return this;}
 bool ShouldUseSeparateRenderTarget() const override {return IsStereoEnabled();}
 void CalculateRenderTargetSize(const FViewport&,uint32&,uint32&) override {}
 bool NeedReAllocateViewportRenderTarget(const FViewport& Viewport) override {return IsStereoEnabled() && Viewport.GetRenderTargetTextureSizeXY()!=Viewport.GetSizeXY();}
 void AdjustViewRect(int32 Eye,int32& X,int32&,uint32& W,uint32& H) const override
 {W/=2;X+=W*Eye;Aspect=H?float(W)/H:1;}
 void CalculateStereoViewOffset(int32 Eye,FRotator& Rotation,float Meters,FVector& Location) override
 {if(Eye!=INDEX_NONE) Location+=Rotation.Quaternion().RotateVector(FVector(0,(Eye==0?-0.5f:0.5f)*IPD*Meters,0));}
 FMatrix GetStereoProjectionMatrix(int32) const override
 {const float X=1/FMath::Tan(FMath::DegreesToRadians(FOV*0.5f));return FMatrix(FPlane(X,0,0,0),FPlane(0,X*Aspect,0,0),FPlane(0,0,0,1),FPlane(0,0,GNearClippingPlane,0));}
 void RenderTexture_RenderThread(FRDGBuilder& Graph,FRDGTextureRef Back,FRDGTextureRef Source,FVector2f) const override
 {DeltaDrawLens(Graph,Back,Source,GetLens());}
};

class FDeltaPhoneModesModule final : public IModuleInterface
{
 TSharedPtr<FDeltaPhoneStereo,ESPMode::ThreadSafe> Stereo;
 TSharedPtr<IStereoRendering,ESPMode::ThreadSafe> PreviousStereo;
 TSharedPtr<SWidget> Overlay;
 TWeakObjectPtr<UGameViewportClient> OverlayViewport;
 TWeakObjectPtr<APlayerController> BoundPlayer;
 FTSTicker::FDelegateHandle TickHandle;
 TArray<IConsoleObject*> Commands;
 bool bInstalled=false,bSettings=false,bCaptured=false,bToggleHeld=false,bCenterHeld=false,bSensor=false,bCentered=false,bInvertLookY=false,bExitHeld=false;
 bool bInvertGyroYaw=false,bInvertGyroPitch=false,bGyroEnabled=true;
 int32 DisplayRotation=-1;
 FQuat SensorCenter=FQuat::Identity,FilteredHead=FQuat::Identity;
 float BodyYaw=0,BodyPitch=0;
 FVector2D SmoothStick=FVector2D::ZeroVector;
 bool bWorldMap=false;
 bool bViewerMenu=false,bMenuHeld=false,bTouchHeld=false,bNavHeld=false,bAutoHeld=false;
 static constexpr int32 ViewerRowCount=19;
 int32 ViewerRow=0, LensSlot=1;
 bool bFineLens=false,bCalibrationGrid=false;
 float NavRepeat=0;
 TWeakObjectPtr<APawn> MenuPawn;
 TSharedPtr<FDeltaViewerInput> ViewerInput;
 TSet<FKey> FrameButtons;
 bool Down(APlayerController* PC,FKey Key) const {return ViewerInput&&Key.IsGamepadKey()?FrameButtons.Contains(Key):PC->IsInputKeyDown(Key);}
 float Axis(APlayerController* PC,FKey Key) const {const float* V=ViewerInput?ViewerInput->Axes.Find(Key):nullptr;return V?*V:PC->GetInputAnalogKeyState(Key);}
 void AutoFit()
 {
  FDeltaLensSettings L;L.Size=.94f;L.Inset=.10f;L.Curve=.28f;
  Stereo->SetLens(L);Stereo->FOV=95;Stereo->IPD=.064f;SaveSettings();
 }

 void LensProfile(bool Save)
 {
  const FString Section=FString::Printf(TEXT("DeltaBots.LensSlot%d"),LensSlot);
  const TCHAR* Keys[]={TEXT("Size"),TEXT("Inset"),TEXT("Curve"),TEXT("IPD"),TEXT("FOV"),TEXT("LeftX"),TEXT("LeftY"),TEXT("RightX"),TEXT("RightY")};
  auto L=Stereo->GetLens();float V[]={L.Size,L.Inset,L.Curve,Stereo->IPD,Stereo->FOV,L.EyeOffset[0].X,L.EyeOffset[0].Y,L.EyeOffset[1].X,L.EyeOffset[1].Y};
  if(Save){for(int32 I=0;I<9;++I)GConfig->SetFloat(*Section,Keys[I],V[I],GGameUserSettingsIni);GConfig->SetBool(*Section,TEXT("Saved"),true,GGameUserSettingsIni);GConfig->Flush(false,GGameUserSettingsIni);}
  else{bool Saved=false;GConfig->GetBool(*Section,TEXT("Saved"),Saved,GGameUserSettingsIni);if(!Saved)return;for(int32 I=0;I<9;++I)GConfig->GetFloat(*Section,Keys[I],V[I],GGameUserSettingsIni);for(float F:V)if(!FMath::IsFinite(F))return;
   L.Size=FMath::Clamp(V[0],.85f,1.12f);L.Inset=FMath::Clamp(V[1],0.f,.15f);L.Curve=FMath::Clamp(V[2],0.f,.45f);Stereo->IPD=FMath::Clamp(V[3],.055f,.075f);Stereo->FOV=FMath::Clamp(V[4],85.f,110.f);L.EyeOffset[0]=FVector2f(FMath::Clamp(V[5],-.08f,.08f),FMath::Clamp(V[6],-.08f,.08f));L.EyeOffset[1]=FVector2f(FMath::Clamp(V[7],-.08f,.08f),FMath::Clamp(V[8],-.08f,.08f));Stereo->SetLens(L);SaveSettings();}
 }
 void ViewerMenu(bool Open)
 {
  if(Open)bWorldMap=false;
  bViewerMenu=Open;SmoothStick=FVector2D::ZeroVector;
  if(!Open)SaveSettings();
 }
 FString ViewerDescription() const
 {
  auto L=Stereo->GetLens();
  static const TCHAR* Names[]={TEXT("LEFT EYE HORIZONTAL"),TEXT("LEFT EYE VERTICAL"),TEXT("RIGHT EYE HORIZONTAL"),TEXT("RIGHT EYE VERTICAL"),TEXT("LENS SPACING"),TEXT("IMAGE SIZE"),TEXT("LENS CURVE"),TEXT("FIELD OF VIEW"),TEXT("VIRTUAL IPD"),TEXT("STICK Y INVERSION"),TEXT("GYRO YAW INVERSION"),TEXT("GYRO PITCH INVERSION"),TEXT("HEAD TRACKING"),TEXT("WORLD MAP"),TEXT("FINE ADJUSTMENTS"),TEXT("ALIGNMENT GRID"),TEXT("LENS PROFILE SLOT"),TEXT("SAVE LENS SLOT"),TEXT("LOAD LENS SLOT")};
  const float Values[]={L.EyeOffset[0].X,L.EyeOffset[0].Y,L.EyeOffset[1].X,L.EyeOffset[1].Y,L.Inset,L.Size,L.Curve,Stereo->FOV,Stereo->IPD*1000,bInvertLookY?1.f:0.f,bInvertGyroYaw?1.f:0.f,bInvertGyroPitch?1.f:0.f,bGyroEnabled?1.f:0.f,bWorldMap?1.f:0.f,bFineLens?1.f:0.f,bCalibrationGrid?1.f:0.f,float(LensSlot),0.f,0.f};
  return FString::Printf(TEXT("VR BOX CUSTOM PROFILE\n%d / 19  %s\nValue: %.3f\nD-pad: select / adjust\nTRIANGLE: reference preset\nOPTIONS: save and close"),ViewerRow+1,Names[ViewerRow],Values[ViewerRow]);
 }
 void ViewerAdjust(int32 Direction)
 {
  auto L=Stereo->GetLens();
  if(ViewerRow<4){float& V=(ViewerRow%2==0)?L.EyeOffset[ViewerRow/2].X:L.EyeOffset[ViewerRow/2].Y;V=FMath::Clamp(V+Direction*(bFineLens?.001f:.005f),-.08f,.08f);Stereo->SetLens(L);}
  else if(ViewerRow==9)bInvertLookY=!bInvertLookY;
  else if(ViewerRow==10)bInvertGyroYaw=!bInvertGyroYaw;
  else if(ViewerRow==11)bInvertGyroPitch=!bInvertGyroPitch;
  else if(ViewerRow==12){bGyroEnabled=!bGyroEnabled;Recenter();}
  else if(ViewerRow==13){ViewerMenu(false);bWorldMap=true;}
  else if(ViewerRow==14)bFineLens=!bFineLens;
  else if(ViewerRow==15)bCalibrationGrid=!bCalibrationGrid;
  else if(ViewerRow==16)LensSlot=(LensSlot-1+Direction+3)%3+1;
  else if(ViewerRow==17)LensProfile(true);
  else if(ViewerRow==18)LensProfile(false);
  else {const float Steps[]={.005f,.025f,.025f,2.f,.001f};Adjust(ViewerRow-4==0?1:ViewerRow-4==1?0:ViewerRow-4,Direction*Steps[ViewerRow-4]*(bFineLens?.25f:1.f));}
 }
 void ViewerTouch(float X,float Y,int32 Width,int32 Height)
 {
  if(Width<=0||Height<=0)return;
  const int32 Eye=X<Width*.5f?0:1;auto L=Stereo->GetLens();
  FVector2f R((X/Width*2-Eye)-(.5f+(Eye==0?1:-1)*L.Inset+L.EyeOffset[Eye].X),Y/Height-(.5f+L.EyeOffset[Eye].Y));
  R*=2.f/L.Size;const float R2=R.SizeSquared();
  const FVector2f UV=(R*(1+L.Curve*R2+L.Curve*.2f*R2*R2)+FVector2f(1,1))*.5f;
  auto Hit=[&](float CX,float CY){return FMath::Abs(UV.X-CX)<.085f&&FMath::Abs(UV.Y-CY)<.035f;};
  if(Hit(.5f,.25f)){bWorldMap=!bWorldMap;if(bWorldMap)ViewerMenu(false);}
  else if(Hit(.3f,.18f))Recenter();else if(Hit(.5f,.18f))ViewerMenu(!bViewerMenu);else if(Hit(.7f,.18f))SetMode(false);
  else if(bViewerMenu){if(Hit(.3f,.55f))ViewerRow=(ViewerRow+ViewerRowCount-1)%ViewerRowCount;else if(Hit(.5f,.55f))AutoFit();else if(Hit(.7f,.55f))ViewerRow=(ViewerRow+1)%ViewerRowCount;else if(Hit(.3f,.66f))ViewerAdjust(-1);else if(Hit(.7f,.66f))ViewerAdjust(1);else if(Hit(.5f,.66f))ViewerMenu(false);}
 }

 APlayerController* Player() const
 {UWorld* W=GEngine&&GEngine->GameViewport?GEngine->GameViewport->GetWorld():nullptr;return W&&W->IsGameWorld()?W->GetFirstPlayerController():nullptr;}
 static FVector2D Stick(float X,float Y)
 {FVector2D V(X,Y);float L=V.Size();return L<=0.18f?FVector2D::ZeroVector:V/L*FMath::Clamp((L-0.18f)/0.82f,0.f,1.f);}
 bool HeadPose(FQuat& Q,int32& Rotation)
 {
  JNIEnv* Env=FAndroidApplication::GetJavaEnv();
  if(!Env||!FJavaWrapper::GameActivityThis)return false;
  static jmethodID Method=FJavaWrapper::FindMethod(Env,FJavaWrapper::GameActivityClassID,"AndroidThunkJava_DeltaHeadPose","()[F",true);
  if(!Method)return false;
  jfloatArray Array=(jfloatArray)Env->CallObjectMethod(FJavaWrapper::GameActivityThis,Method);
  if(Env->ExceptionCheck()){Env->ExceptionClear();return false;}
  if(!Array)return false;
  float V[6]={};const bool Valid=Env->GetArrayLength(Array)>=6;
  if(Valid)Env->GetFloatArrayRegion(Array,0,6,V);Env->DeleteLocalRef(Array);
  if(!Valid||V[5]<0.5f)return false;
  Rotation=FMath::RoundToInt(V[4]);
  const FQuat Device(V[1],V[2],V[3],V[0]);
  // Express screen-local axes in the natural device frame. For ROTATION_90,
  // Android AXIS_Y / AXIS_MINUS_X remaps matrix columns to -Y,+X,Z.
  // This is Device * Rz(-90), not +90. Handle reverse landscape identically.
  const FQuat DeviceFromScreen(FVector(0,0,1),FMath::DegreesToRadians(-90.f*Rotation));
  Q=(Device*DeviceFromScreen).GetNormalized();
  return !Q.ContainsNaN();
 }
 void SaveSettings()
 {
  auto L=Stereo->GetLens();
  GConfig->SetFloat(SettingsSection,TEXT("ImageSize"),L.Size,GGameUserSettingsIni);
  GConfig->SetFloat(SettingsSection,TEXT("LensInset"),L.Inset,GGameUserSettingsIni);
  GConfig->SetFloat(SettingsSection,TEXT("Curve"),L.Curve,GGameUserSettingsIni);
  for(int32 Eye=0;Eye<2;++Eye){GConfig->SetFloat(SettingsSection,*FString::Printf(TEXT("Eye%dX"),Eye),L.EyeOffset[Eye].X,GGameUserSettingsIni);GConfig->SetFloat(SettingsSection,*FString::Printf(TEXT("Eye%dY"),Eye),L.EyeOffset[Eye].Y,GGameUserSettingsIni);}
  GConfig->SetFloat(SettingsSection,TEXT("IPD"),Stereo->IPD,GGameUserSettingsIni);
  GConfig->SetFloat(SettingsSection,TEXT("FOV"),Stereo->FOV,GGameUserSettingsIni);
  GConfig->SetBool(SettingsSection,TEXT("InvertLookY"),bInvertLookY,GGameUserSettingsIni);
  GConfig->SetBool(SettingsSection,TEXT("InvertGyroYaw"),bInvertGyroYaw,GGameUserSettingsIni);
  GConfig->SetBool(SettingsSection,TEXT("InvertGyroPitch"),bInvertGyroPitch,GGameUserSettingsIni);
  GConfig->SetBool(SettingsSection,TEXT("GyroEnabled"),bGyroEnabled,GGameUserSettingsIni);
  GConfig->Flush(false,GGameUserSettingsIni);
  GConfig->SetInt(SettingsSection,TEXT("ViewerProfileVersion"),5,GGameUserSettingsIni);GConfig->Flush(false,GGameUserSettingsIni);
  if(JNIEnv* Env=FAndroidApplication::GetJavaEnv())
  {
   static jmethodID Method=FJavaWrapper::FindMethod(Env,FJavaWrapper::GameActivityClassID,"AndroidThunkJava_DeltaSaveViewer","([F)V",true);
   if(Method){const float V[]={L.Size,L.Inset,L.Curve,Stereo->IPD,Stereo->FOV,bInvertLookY?1.f:0.f,L.EyeOffset[0].X,L.EyeOffset[0].Y,L.EyeOffset[1].X,L.EyeOffset[1].Y,5.f,bInvertGyroYaw?1.f:0.f,bInvertGyroPitch?1.f:0.f,bGyroEnabled?1.f:0.f};jfloatArray A=Env->NewFloatArray(14);if(A){Env->SetFloatArrayRegion(A,0,14,V);Env->CallVoidMethod(FJavaWrapper::GameActivityThis,Method,A);Env->DeleteLocalRef(A);}if(Env->ExceptionCheck())Env->ExceptionClear();}
  }
 }
 void LoadSettings()
 {
  if(JNIEnv* Env=FAndroidApplication::GetJavaEnv())
  {
   static jmethodID Method=FJavaWrapper::FindMethod(Env,FJavaWrapper::GameActivityClassID,"AndroidThunkJava_DeltaLoadViewer","()[F",true);
   if(Method){auto A=(jfloatArray)Env->CallObjectMethod(FJavaWrapper::GameActivityThis,Method);if(Env->ExceptionCheck()){Env->ExceptionClear();A=nullptr;}if(A){float V[14]={};V[13]=1;int32 Count=Env->GetArrayLength(A);bool Valid=Count==11||Count==14;if(Valid)Env->GetFloatArrayRegion(A,0,Count,V);Env->DeleteLocalRef(A);for(float F:V)Valid=Valid&&FMath::IsFinite(F);if(Valid&&(V[10]==3||V[10]==4||V[10]==5)){FDeltaLensSettings L;L.Size=FMath::Clamp(V[0],.85f,1.12f);L.Inset=FMath::Clamp(V[1],0.f,.15f);L.Curve=FMath::Clamp(V[2],0.f,.45f);Stereo->IPD=FMath::Clamp(V[3],.055f,.075f);Stereo->FOV=FMath::Clamp(V[4],85.f,110.f);bInvertLookY=V[5]>.5f;bInvertGyroYaw=V[11]>.5f;bInvertGyroPitch=V[12]>.5f;bGyroEnabled=V[13]>.5f;for(int32 E=0;E<2;++E)L.EyeOffset[E]=FVector2f(FMath::Clamp(V[6+E*2],-.08f,.08f),FMath::Clamp(V[7+E*2],-.08f,.08f));Stereo->SetLens(L);if(V[10]<5){bInvertLookY=false;bInvertGyroYaw=false;bInvertGyroPitch=false;SaveSettings();}return;}}}
  }
  FDeltaLensSettings L;
  for(int32 Eye=0;Eye<2;++Eye){GConfig->GetFloat(SettingsSection,*FString::Printf(TEXT("Eye%dX"),Eye),L.EyeOffset[Eye].X,GGameUserSettingsIni);GConfig->GetFloat(SettingsSection,*FString::Printf(TEXT("Eye%dY"),Eye),L.EyeOffset[Eye].Y,GGameUserSettingsIni);L.EyeOffset[Eye].X=FMath::Clamp(L.EyeOffset[Eye].X,-.08f,.08f);L.EyeOffset[Eye].Y=FMath::Clamp(L.EyeOffset[Eye].Y,-.08f,.08f);}
  GConfig->GetFloat(SettingsSection,TEXT("ImageSize"),L.Size,GGameUserSettingsIni);
  GConfig->GetFloat(SettingsSection,TEXT("LensInset"),L.Inset,GGameUserSettingsIni);
  GConfig->GetFloat(SettingsSection,TEXT("Curve"),L.Curve,GGameUserSettingsIni);
  L.Size=FMath::Clamp(L.Size,0.85f,1.12f);L.Inset=FMath::Clamp(L.Inset,0.f,0.15f);L.Curve=FMath::Clamp(L.Curve,0.f,0.45f);Stereo->SetLens(L);
  GConfig->GetFloat(SettingsSection,TEXT("IPD"),Stereo->IPD,GGameUserSettingsIni);Stereo->IPD=FMath::Clamp(Stereo->IPD,0.055f,0.075f);
  GConfig->GetFloat(SettingsSection,TEXT("FOV"),Stereo->FOV,GGameUserSettingsIni);Stereo->FOV=FMath::Clamp(Stereo->FOV,85.f,110.f);
  GConfig->GetBool(SettingsSection,TEXT("InvertLookY"),bInvertLookY,GGameUserSettingsIni);
  GConfig->GetBool(SettingsSection,TEXT("InvertGyroYaw"),bInvertGyroYaw,GGameUserSettingsIni);
  GConfig->GetBool(SettingsSection,TEXT("InvertGyroPitch"),bInvertGyroPitch,GGameUserSettingsIni);
  GConfig->GetBool(SettingsSection,TEXT("GyroEnabled"),bGyroEnabled,GGameUserSettingsIni);
  int32 ProfileVersion=0;GConfig->GetInt(SettingsSection,TEXT("ViewerProfileVersion"),ProfileVersion,GGameUserSettingsIni);if(ProfileVersion<3)AutoFit();if(ProfileVersion<5){bInvertLookY=false;bInvertGyroYaw=false;bInvertGyroPitch=false;SaveSettings();}
 }
 void CaptureInputs(APlayerController* PC,bool Capture)
 {if(bCaptured!=Capture){PC->SetIgnoreLookInput(Capture);PC->SetIgnoreMoveInput(Capture);bCaptured=Capture;}}
 void Recenter()
 {
  if(auto* PC=Player()){BodyYaw=PC->GetControlRotation().Yaw;BodyPitch=0;FilteredHead=FQuat::Identity;bCentered=HeadPose(SensorCenter,DisplayRotation);PC->SetControlRotation(FRotator(0,BodyYaw,0));}
  UE_LOG(LogDeltaPhoneModes,Display,TEXT("Recenter: fused orientation, level horizon"));
 }
 void SetMode(bool VR)
 {
  auto* PC=Player();if(!PC||Stereo->IsStereoEnabled()==VR)return;
  if(bViewerMenu)ViewerMenu(false);
  bSettings=false;bWorldMap=false;Stereo->EnableStereo(VR);SmoothStick=FVector2D::ZeroVector;
  if(VR){Recenter();PC->ConsoleCommand(TEXT("DisableAllScreenMessages"),false);}
  else{FRotator R=PC->GetControlRotation();R.Roll=0;PC->SetControlRotation(R);PC->ConsoleCommand(TEXT("EnableAllScreenMessages"),false);}
  CaptureInputs(PC,VR);
  if(auto* HUD=PC->GetHUD())HUD->bShowHUD=!VR;
  auto L=Stereo->GetLens();
  UE_LOG(LogDeltaPhoneModes,Display,TEXT("Mode=%s fused=%d image=%.2f lensInset=%.2f curve=%.2f IPD=%.0f"),VR?TEXT("VR"):TEXT("Mobile"),bCentered,L.Size,L.Inset,L.Curve,Stereo->IPD*1000);
 }
 FReply Adjust(int32 Which,float Delta)
 {
  auto L=Stereo->GetLens();
  if(Which==0)L.Size=FMath::Clamp(L.Size+Delta,0.85f,1.12f);
  if(Which==1)L.Inset=FMath::Clamp(L.Inset+Delta,0.f,0.15f);
  if(Which==2)L.Curve=FMath::Clamp(L.Curve+Delta,0.f,0.45f);
  if(Which==3)Stereo->FOV=FMath::Clamp(Stereo->FOV+Delta,85.f,110.f);
  if(Which==4)Stereo->IPD=FMath::Clamp(Stereo->IPD+Delta,0.055f,0.075f);
  Stereo->SetLens(L);SaveSettings();return FReply::Handled();
 }
 TSharedRef<SWidget> Button(const TCHAR* Label,TFunction<FReply()> Action)
 {return SNew(SButton).ButtonColorAndOpacity(FLinearColor(.015f,.12f,.18f,.96f)).ForegroundColor(FLinearColor(.3f,.95f,1.f)).ContentPadding(FMargin(16,10)).OnClicked_Lambda([Action]{return Action();})[SNew(STextBlock).Text(FText::FromString(Label)).Font(FCoreStyle::GetDefaultFontStyle("Bold",18))];}
 FText Status() const
 {
  auto* PC=Player();UObject* Pawn=PC?PC->GetPawn():nullptr;
  auto Value=[Pawn](const TCHAR* N)->double{if(!Pawn)return 0;const auto* P=FindFProperty<FNumericProperty>(Pawn->GetClass(),N);return P&&P->IsFloatingPoint()?P->GetFloatingPointPropertyValue(P->ContainerPtrToValuePtr<void>(Pawn)):0;};
  return FText::FromString(FString::Printf(TEXT("HP %.0f  SHIELD %.0f  SKILL %.0fs\nHEAD %s | SELECT: EXIT  R3: CENTER\nOPTIONS: ADJUST | TWO FINGERS: EXIT"),Value(TEXT("Health")),Value(TEXT("Shield")),Value(TEXT("ScarabSkillCooldown")),bSensor?TEXT("READY"):TEXT("WAITING")));
 }
 TSharedRef<SWidget> WorldMap()
 {
  auto Map=SNew(SDeltaWorldNavigator);Map->IsOpen=[this]{return bWorldMap;};
  Map->PlayerPosition=[this]{auto* PC=Player();return PC&&PC->GetPawn()?PC->GetPawn()->GetActorLocation():FVector::ZeroVector;};return Map;
 }
 TSharedRef<SWidget> EyeOverlay(int32 Eye)
 {
  // HUD is part of the source render target, so the lens mesh also positions/warps it.
  auto Panel=SNew(SDeltaViewerPanel);Panel->Description=[this]{return ViewerDescription();};Panel->IsOpen=[this]{return bViewerMenu;};Panel->ShowGrid=[this]{return bCalibrationGrid;};
  return SNew(SOverlay)
   +SOverlay::Slot()[Panel]
   +SOverlay::Slot()[WorldMap()]
   +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(STextBlock).Visibility_Lambda([this]{return bViewerMenu||bWorldMap?EVisibility::Collapsed:EVisibility::HitTestInvisible;}).Text(FText::FromString(TEXT("+"))).ColorAndOpacity(FLinearColor(0.05,0.85,1)).Font(FCoreStyle::GetDefaultFontStyle("Bold",22))]
   +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(0,180,0,0)[SNew(STextBlock).Visibility_Lambda([this]{return bViewerMenu||bWorldMap?EVisibility::Collapsed:EVisibility::HitTestInvisible;}).Text_Lambda([this]{return Status();}).Justification(ETextJustify::Center).Font(FCoreStyle::GetDefaultFontStyle("Regular",15))];
 }
 void AddOverlay()
 {
  auto Controls=SNew(SHorizontalBox).Visibility_Lambda([this]{return Stereo->IsStereoEnabled()?EVisibility::Collapsed:EVisibility::Visible;});
  Controls->AddSlot().AutoWidth()[Button(TEXT("MOBILE"),[this]{SetMode(false);return FReply::Handled();})];
  Controls->AddSlot().AutoWidth()[Button(TEXT("VR MODE"),[this]{SetMode(true);return FReply::Handled();})];
  Controls->AddSlot().AutoWidth()[Button(TEXT("CENTER"),[this]{Recenter();return FReply::Handled();})];
  Controls->AddSlot().AutoWidth()[Button(TEXT("MAP"),[this]{bWorldMap=!bWorldMap;return FReply::Handled();})];
  Controls->AddSlot().AutoWidth()[Button(TEXT("SETTINGS"),[this]{bSettings=!bSettings;return FReply::Handled();})];
  auto Settings=SNew(SVerticalBox);
  auto Row=[&](const TCHAR* Minus,const TCHAR* Plus,int32 Which,float Step){Settings->AddSlot().AutoHeight()[SNew(SHorizontalBox)+SHorizontalBox::Slot().FillWidth(1)[Button(Minus,[this,Which,Step]{return Adjust(Which,-Step*(bFineLens?.25f:1.f));})]+SHorizontalBox::Slot().FillWidth(1)[Button(Plus,[this,Which,Step]{return Adjust(Which,Step*(bFineLens?.25f:1.f));})]];};
  Row(TEXT("IMAGE -"),TEXT("IMAGE +"),0,0.025f);Row(TEXT("LENS IN -"),TEXT("LENS IN +"),1,0.01f);Row(TEXT("CURVE -"),TEXT("CURVE +"),2,0.025f);Row(TEXT("FOV -"),TEXT("FOV +"),3,5);Row(TEXT("IPD -"),TEXT("IPD +"),4,0.002f);
  Settings->AddSlot().AutoHeight()[Button(TEXT("FINE LENS STEP: TOGGLE"),[this]{bFineLens=!bFineLens;return FReply::Handled();})];
  Settings->AddSlot().AutoHeight()[Button(TEXT("CALIBRATION GRID: TOGGLE"),[this]{bCalibrationGrid=!bCalibrationGrid;return FReply::Handled();})];
  Settings->AddSlot().AutoHeight()[Button(TEXT("TOGGLE LOOK-Y INVERSION"),[this]{bInvertLookY=!bInvertLookY;SaveSettings();return FReply::Handled();})];
  Settings->AddSlot().AutoHeight()[Button(TEXT("GYRO YAW: TOGGLE DIRECTION"),[this]{bInvertGyroYaw=!bInvertGyroYaw;Recenter();SaveSettings();return FReply::Handled();})];
  Settings->AddSlot().AutoHeight()[Button(TEXT("GYRO PITCH: TOGGLE DIRECTION"),[this]{bInvertGyroPitch=!bInvertGyroPitch;Recenter();SaveSettings();return FReply::Handled();})];
  Settings->AddSlot().AutoHeight()[Button(TEXT("HEAD TRACKING: ON / OFF"),[this]{bGyroEnabled=!bGyroEnabled;Recenter();SaveSettings();return FReply::Handled();})];
  Settings->AddSlot().AutoHeight()[SNew(STextBlock).Text_Lambda([this]{auto L=Stereo->GetLens();return FText::FromString(FString::Printf(TEXT("Image %.0f%% | Inset %.2f | Curve %.2f\nFOV %.0f | IPD %.0f mm | Look Y %s"),L.Size*100,L.Inset,L.Curve,Stereo->FOV,Stereo->IPD*1000,bInvertLookY?TEXT("INVERTED"):TEXT("NORMAL")));})];
  auto Root=SNew(SOverlay)
   +SOverlay::Slot()[SNew(SBox).Visibility_Lambda([this]{return !Stereo->IsStereoEnabled()?EVisibility::HitTestInvisible:EVisibility::Collapsed;})[WorldMap()]]
   +SOverlay::Slot()[SNew(SHorizontalBox).Visibility_Lambda([this]{return Stereo->IsStereoEnabled()?EVisibility::HitTestInvisible:EVisibility::Collapsed;})+SHorizontalBox::Slot().FillWidth(1)[EyeOverlay(0)]+SHorizontalBox::Slot().FillWidth(1)[EyeOverlay(1)]]
   +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0,12,0,0)[Controls]
   +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0,90,0,0)[SNew(SBorder).BorderBackgroundColor(FLinearColor(0.015,0.035,0.06,0.98)).Padding(18).Visibility_Lambda([this]{return bSettings?EVisibility::Visible:EVisibility::Collapsed;})[SNew(SBox).MaxDesiredHeight(480)[SNew(SScrollBox)+SScrollBox::Slot()[Settings]]]];
  Overlay=Root;OverlayViewport=GEngine->GameViewport;GEngine->GameViewport->AddViewportWidgetContent(Root,100);
 }
 bool Tick(float Delta)
 {
  auto* PC=Player();if(!PC)return true;
  FrameButtons.Reset();if(ViewerInput){FrameButtons=ViewerInput->Held;FrameButtons.Append(ViewerInput->Pressed);ViewerInput->Pressed.Reset();}
  if(!bInstalled){PreviousStereo=GEngine->StereoRenderingDevice;GEngine->StereoRenderingDevice=Stereo;bInstalled=true;LoadSettings();UE_LOG(LogDeltaPhoneModes,Display,TEXT("Viewer v2 ready: lens compositor and fused landscape head pose"));}
  if(BoundPlayer.Get()!=PC){BoundPlayer=PC;bCaptured=false;bCentered=false;BodyYaw=PC->GetControlRotation().Yaw;BodyPitch=0;PC->SetMotionControlsEnabled(true);}
  if(!OverlayViewport.IsValid()||OverlayViewport.Get()!=GEngine->GameViewport){Overlay.Reset();AddOverlay();}
  const bool Toggle=Down(PC,EKeys::V)||Down(PC,EKeys::Gamepad_Special_Left);
  const bool Center=Down(PC,EKeys::C)||Down(PC,EKeys::Gamepad_RightThumbstick);
  if(Toggle&&!bToggleHeld)SetMode(!Stereo->IsStereoEnabled());if(Center&&!bCenterHeld)Recenter();bToggleHeld=Toggle;bCenterHeld=Center;
  float TX=0,TY=0;bool First=false,Second=false;
  PC->GetInputTouchState(ETouchIndex::Touch1,TX,TY,First);PC->GetInputTouchState(ETouchIndex::Touch2,TX,TY,Second);
  const bool Exit=First&&Second;
  if(Stereo->IsStereoEnabled()&&Exit&&!bExitHeld)SetMode(false);bExitHeld=Exit;
  // Re-read Touch1: the second query above overwrites its coordinates.
  PC->GetInputTouchState(ETouchIndex::Touch1,TX,TY,First);
  if(Stereo->IsStereoEnabled()&&First&&!Second&&!bTouchHeld){int32 W=0,H=0;PC->GetViewportSize(W,H);ViewerTouch(TX,TY,W,H);}bTouchHeld=First;
  const bool Menu=Down(PC,EKeys::Gamepad_Special_Right)||Down(PC,EKeys::M);
  if(Stereo->IsStereoEnabled()&&Menu&&!bMenuHeld)ViewerMenu(!bViewerMenu);bMenuHeld=Menu;
  if(bViewerMenu)
  {
   const bool Auto=Down(PC,EKeys::Gamepad_FaceButton_Top);if(Auto&&!bAutoHeld)AutoFit();bAutoHeld=Auto;
   const int32 Vertical=int32(Down(PC,EKeys::Gamepad_DPad_Down))-int32(Down(PC,EKeys::Gamepad_DPad_Up));
   const int32 Horizontal=int32(Down(PC,EKeys::Gamepad_DPad_Right))-int32(Down(PC,EKeys::Gamepad_DPad_Left));
   const bool Nav=Vertical!=0||Horizontal!=0;NavRepeat-=Delta;
   if(Nav&&(!bNavHeld||NavRepeat<=0)){if(Vertical)ViewerRow=(ViewerRow+Vertical+ViewerRowCount)%ViewerRowCount;else ViewerAdjust(Horizontal);NavRepeat=bNavHeld?.12f:.35f;}bNavHeld=Nav;
  }
  else {bNavHeld=false;bAutoHeld=false;}
  CaptureInputs(PC,Stereo->IsStereoEnabled()||bSettings||bWorldMap);
  if(auto* HUD=PC->GetHUD())HUD->bShowHUD=!(Stereo->IsStereoEnabled()||bSettings||bWorldMap);
  if(bSettings||(!Stereo->IsStereoEnabled()&&bWorldMap))return true;
  if(!Stereo->IsStereoEnabled())
  {
   // The input preprocessor consumes only the four analog sticks in mobile
   // mode. Touch look and digital actions continue through the existing HUD.
   const float Dt=FMath::Clamp(Delta,0.f,.05f);
   const FVector2D Look=Stick(Axis(PC,EKeys::Gamepad_RightX),Axis(PC,EKeys::Gamepad_RightY));
   SmoothStick=FMath::Lerp(SmoothStick,Look,1-FMath::Exp(-Dt*24));
   if(!SmoothStick.IsNearlyZero(.001f))
   {
    FRotator View=PC->GetControlRotation();View.Yaw+=SmoothStick.X*90*Dt;
    View.Pitch=FMath::Clamp(FRotator::NormalizeAxis(View.Pitch)+SmoothStick.Y*(bInvertLookY?-1:1)*65*Dt,-70.f,70.f);
    View.Roll=0;PC->SetControlRotation(View);
   }
   if(APawn* Pawn=PC->GetPawn())
   {
    const FBoolProperty* Defeated=FindFProperty<FBoolProperty>(Pawn->GetClass(),TEXT("bDefeated"));
    if(Defeated&&Defeated->GetPropertyValue_InContainer(Pawn))return true;
    const FVector2D Move=Stick(Axis(PC,EKeys::Gamepad_LeftX),Axis(PC,EKeys::Gamepad_LeftY));
    const FRotator Heading(0,PC->GetControlRotation().Yaw,0);
    Pawn->AddMovementInput(Heading.Vector(),Move.Y);Pawn->AddMovementInput(Heading.Quaternion().GetRightVector(),Move.X);
   }
   return true;
  }
  const float Dt=FMath::Clamp(Delta,0.f,0.05f);
  FQuat Sample;int32 Rotation=0;bSensor=bGyroEnabled&&HeadPose(Sample,Rotation);
  if(bSensor)
  {
   if(!bCentered||Rotation!=DisplayRotation){SensorCenter=Sample;DisplayRotation=Rotation;bCentered=true;FilteredHead=FQuat::Identity;}
   const FQuat Relative=(SensorCenter.Inverse()*Sample).GetNormalized();
   // Android screen axes: right/up/toward viewer. Unreal: forward/right/up, opposite handedness.
   FRotator HeadAngles=FQuat(Relative.Z,-Relative.X,-Relative.Y,Relative.W).Rotator();
   if(bInvertGyroYaw)HeadAngles.Yaw=-HeadAngles.Yaw;
   if(bInvertGyroPitch)HeadAngles.Pitch=-HeadAngles.Pitch;
   const FQuat Head=HeadAngles.Quaternion();
   FilteredHead=FQuat::Slerp(FilteredHead,Head,1-FMath::Exp(-Dt/0.018f)).GetNormalized();
  }
  else {bCentered=false;FilteredHead=FQuat::Identity;}
  FVector2D Look=Stick(Axis(PC,EKeys::Gamepad_RightX),Axis(PC,EKeys::Gamepad_RightY));
  SmoothStick=FMath::Lerp(SmoothStick,Look,1-FMath::Exp(-Dt*24));
  if(bViewerMenu||bWorldMap)SmoothStick=FVector2D::ZeroVector;
  BodyYaw+=SmoothStick.X*90*Dt;BodyPitch=FMath::Clamp(BodyPitch+SmoothStick.Y*(bInvertLookY?-1:1)*65*Dt,-70.f,70.f);
  const FRotator View=(FRotator(BodyPitch,BodyYaw,0).Quaternion()*FilteredHead).Rotator();PC->SetControlRotation(View);
  if(bViewerMenu||bWorldMap)return true;
  if(APawn* Pawn=PC->GetPawn())
  {
   const FBoolProperty* Defeated=FindFProperty<FBoolProperty>(Pawn->GetClass(),TEXT("bDefeated"));
   if(Defeated&&Defeated->GetPropertyValue_InContainer(Pawn))return true;
   FVector2D Move=Stick(Axis(PC,EKeys::Gamepad_LeftX),Axis(PC,EKeys::Gamepad_LeftY));
   const FRotator Heading(0,View.Yaw,0);Pawn->AddMovementInput(Heading.Vector(),Move.Y,true);Pawn->AddMovementInput(Heading.Quaternion().GetRightVector(),Move.X,true);
  }
  return true;
 }
public:
 void StartupModule() override
 {
  Stereo=MakeShared<FDeltaPhoneStereo,ESPMode::ThreadSafe>();
  ViewerInput=MakeShared<FDeltaViewerInput>();
  ViewerInput->RouteButton=[this](FKey Key,bool Pressed)
  {
   if(!Stereo->IsStereoEnabled())return false;
   if(auto* PC=Player())
   {
    const bool ViewerKey=Key==EKeys::Gamepad_Special_Left||Key==EKeys::Gamepad_Special_Right||Key==EKeys::Gamepad_RightThumbstick;
    if(!ViewerKey&&(!bViewerMenu||!Pressed))PC->InputKey(FInputKeyParams(Key,Pressed?IE_Pressed:IE_Released,Pressed?1.0:0.0,true));
   }
   return true;
  };
  ViewerInput->CaptureAnalog=[this](FKey Key,float Value)
  {
   const bool StickAxis=Key==EKeys::Gamepad_LeftX||Key==EKeys::Gamepad_LeftY||Key==EKeys::Gamepad_RightX||Key==EKeys::Gamepad_RightY;
   if(!Stereo->IsStereoEnabled())return StickAxis;
   if(!bViewerMenu&&Key!=EKeys::Gamepad_LeftX&&Key!=EKeys::Gamepad_LeftY&&Key!=EKeys::Gamepad_RightX&&Key!=EKeys::Gamepad_RightY)
    if(auto* PC=Player())PC->InputKey(FInputKeyParams(Key,Value,1.f/60,1,true));
   return true;
  };
  if(FSlateApplication::IsInitialized())FSlateApplication::Get().RegisterInputPreProcessor(ViewerInput,0);
  Commands.Add(IConsoleManager::Get().RegisterConsoleCommand(TEXT("Delta.PhoneVR"),TEXT("0 mobile / 1 viewer mode"),FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& A){SetMode(A.Num()&&A[0]==TEXT("1"));})));
  Commands.Add(IConsoleManager::Get().RegisterConsoleCommand(TEXT("Delta.Recenter"),TEXT("Center fused head pose"),FConsoleCommandDelegate::CreateRaw(this,&FDeltaPhoneModesModule::Recenter)));
  TickHandle=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this,&FDeltaPhoneModesModule::Tick));
 }
 void ShutdownModule() override
 {
  if(bViewerMenu)ViewerMenu(false);
  if(ViewerInput&&FSlateApplication::IsInitialized())FSlateApplication::Get().UnregisterInputPreProcessor(ViewerInput);ViewerInput.Reset();
  FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);if(auto* PC=Player())CaptureInputs(PC,false);
  if(OverlayViewport.IsValid()&&Overlay.IsValid())OverlayViewport->RemoveViewportWidgetContent(Overlay.ToSharedRef());
  for(auto* C:Commands)IConsoleManager::Get().UnregisterConsoleObject(C);
  if(GEngine&&bInstalled&&GEngine->StereoRenderingDevice==Stereo)GEngine->StereoRenderingDevice=PreviousStereo;
  Overlay.Reset();Stereo.Reset();
 }
};
IMPLEMENT_MODULE(FDeltaPhoneModesModule,DeltaPhoneModes)


