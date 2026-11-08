package in.dimensionz.portal;

import android.content.*;
import android.graphics.*;
import android.os.SystemClock;
import android.hardware.SensorManager;
import android.view.*;
import java.util.*;

/** Self-contained stereo calibration world. Never reads or injects into another app. */
public final class PortalView extends View {
    private final SharedPreferences prefs;
    private final Paint paint = new Paint(3);
    private final float[] matrix = new float[9], landscape = new float[9], angles = new float[3];
    private final float[] trackingOrigin={1,0,0,0,1,0,0,0,1}, relativePose=new float[9];
    private int trackingRotation=-1;
    private final Set<Integer> held = new HashSet<>();
    private final String[] actions = {"Fire", "Crouch", "Reload", "Jump", "Aim", "Sprint", "Settings", "Recenter"};
    private final int[] defaults = {96,97,99,100,102,103,108,109};
    private final int[] bindings = defaults.clone();
    private final float[] values = {85, 1, 64, 0, 0, 0, 0, 1, .15f, 0, 0, 0};
    private final String[] labels = {"Field of view", "Image size", "Stereo baseline", "Eye separation", "Left vertical", "Right vertical", "Lens warp", "Look sensitivity", "Stick deadzone", "Vignette", "Left horizontal", "Right horizontal"};
    private final int[] displayOrder={0,1,2,3,10,11,4,5,6,7,8,9};
    private final float[] mins = {60,.75f,0,-.08f,-.06f,-.06f,-.25f,.3f,.05f,0,-.08f,-.08f};
    private final float[] maxs = {105,1.12f,74,.08f,.06f,.06f,.25f,2,.35f,.6f,.08f,.08f};
    private final float[] steps = {5,.025f,2,.005f,.005f,.005f,.025f,.1f,.025f,.1f,.005f,.005f};
    private final float[][] boxes = makeDistrict();
    private static float[][] makeDistrict(){
        ArrayList<float[]> b=new ArrayList<>();
        // Portal camp, solar yard and relay courtyard: simplified reference geometry.
        b.add(new float[]{-6,1,7,2,1,2,0});b.add(new float[]{6,1,12,2,1,2,0});b.add(new float[]{-7,2,23,1,2,2,0});b.add(new float[]{7,2,26,1,2,2,0});
        for(int i=0;i<12;i++){double a=i*Math.PI/6;b.add(new float[]{(float)Math.cos(a)*2,2.6f+(float)Math.sin(a)*2,-5,.32f,.32f,.25f,2});}
        for(int i=0;i<4;i++){b.add(new float[]{-10+i*2,1,16,.75f,.12f,1,3});b.add(new float[]{-10+i*2,.5f,16,.08f,.5f,.08f,1});}
        // Silver Delta-inspired practice bot, dark visor and cyan chest core.
        b.add(new float[]{0,1.35f,8,.38f,.43f,.22f,1});b.add(new float[]{0,2.05f,8,.3f,.28f,.25f,1});
        b.add(new float[]{0,2.07f,7.73f,.26f,.09f,.025f,3});b.add(new float[]{0,1.4f,7.75f,.12f,.15f,.035f,2});
        for(int sign:new int[]{-1,1}){b.add(new float[]{sign*.2f,.48f,8,.13f,.48f,.16f,1});b.add(new float[]{sign*.57f,1.3f,8,.12f,.44f,.13f,1});b.add(new float[]{sign*.57f,1.01f,7.8f,.1f,.1f,.17f,2});}
        // Scarab silhouette at the relay.
        b.add(new float[]{2,.75f,21,.7f,.4f,.8f,3});for(int i=0;i<3;i++)for(int s:new int[]{-1,1})b.add(new float[]{2+s*.85f,.35f,20.4f+i*.6f,.3f,.12f,.1f,1});
        return b.toArray(new float[0][]);
    }
    private Bitmap eyeBitmap;
    private Canvas eyeCanvas;
    private final float[] mesh = new float[17*17*2];
    private final ArrayList<Face> faces = new ArrayList<>();
    public boolean running, hasTracking;
    private boolean menu = true, trackingEnabled = true, invertLook = false, swapSticks = false, firstRotation = true;
    private int tab, selected, learning = -1;
    private float sensorYaw, sensorPitch, originYaw, originPitch, yaw, pitch, stickYaw, stickPitch;
    private float menuYaw, menuPitch, moveX, moveY, lookX, lookY, px, pz, jump;
    private long lastTime, lastShot, gazeStart, menuDwell, lastHat, startupRecenterAt=SystemClock.uptimeMillis()+1500;
    private int shots, ammo = 30, gazeIndex = -1, leftPointer = -1, rightPointer = -1;
    private float leftTouchX, leftTouchY, rightTouchX, rightTouchY;
    private String status = "Welcome. Calibrate before wearing. BGMI integration unavailable.";
    private String controller = "No controller input yet";
    private RectF[] hitRects = new RectF[24];
    private boolean gazeLatched, gazeEnabled=false;
    private long gazeSuspendedUntil;

    public PortalView(Context context) {
        super(context); setFocusableInTouchMode(true); requestFocus();
        prefs = context.getSharedPreferences("portal-z-v01", Context.MODE_PRIVATE);
        for (int i=0;i<values.length;i++) values[i]=prefs.getFloat("v"+i,values[i]);
        for (int i=0;i<bindings.length;i++) bindings[i]=prefs.getInt("b"+i,bindings[i]);
        trackingEnabled=prefs.getBoolean("tracking",true);gazeEnabled=prefs.getBoolean("gazeEnabled",false);
        invertLook=prefs.getBoolean("invert",false); swapSticks=prefs.getBoolean("swap",false);
        if(prefs.getInt("trackingVersion",0)<2){invertLook=false;prefs.edit().putBoolean("invert",false).putInt("trackingVersion",2).apply();}
        for(int i=0;i<hitRects.length;i++)hitRects[i]=new RectF();
    }
    void resetClock() { lastTime=SystemClock.uptimeMillis(); }
    void clearInput() { held.clear(); moveX=moveY=lookX=lookY=0; leftPointer=rightPointer=-1; learning=-1; }
    void save() {
        SharedPreferences.Editor e=prefs.edit();
        for(int i=0;i<values.length;i++)e.putFloat("v"+i,values[i]);
        for(int i=0;i<bindings.length;i++)e.putInt("b"+i,bindings[i]);
        e.putBoolean("gazeEnabled",gazeEnabled).putBoolean("tracking",trackingEnabled).putBoolean("invert",invertLook).putBoolean("swap",swapSticks).apply();
    }
    void rotation(float[] vector) {
        SensorManager.getRotationMatrixFromVector(matrix, vector);
        int rotation=((WindowManager)getContext().getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay().getRotation();
        int x=SensorManager.AXIS_X,y=SensorManager.AXIS_Y;
        if(rotation==Surface.ROTATION_90){x=SensorManager.AXIS_Y;y=SensorManager.AXIS_MINUS_X;}
        else if(rotation==Surface.ROTATION_180){x=SensorManager.AXIS_MINUS_X;y=SensorManager.AXIS_MINUS_Y;}
        else if(rotation==Surface.ROTATION_270){x=SensorManager.AXIS_MINUS_Y;y=SensorManager.AXIS_X;}
        SensorManager.remapCoordinateSystem(matrix,x,y,landscape);
        if(firstRotation||rotation!=trackingRotation){firstRotation=false;trackingRotation=rotation;recenter();}
        // Relative orientation avoids Euler pitch singularities in a phone viewer.
        for(int row=0;row<3;row++)for(int col=0;col<3;col++){
            float value=0;for(int k=0;k<3;k++)value+=trackingOrigin[k*3+row]*landscape[k*3+col];
            relativePose[row*3+col]=value;
        }
        sensorYaw=(float)Math.atan2(-relativePose[2],relativePose[8]);
        sensorPitch=(float)Math.asin(clamp(-relativePose[5],-1,1));
    }
    private float wrap(float a) { return (float)Math.atan2(Math.sin(a),Math.cos(a)); }
    private float clamp(float v,float a,float b) { return Math.max(a,Math.min(b,v)); }
    private float dead(float v) { float d=values[8]; return Math.abs(v)<d?0:Math.signum(v)*(Math.abs(v)-d)/(1-d); }
    private void recenter() { System.arraycopy(landscape,0,trackingOrigin,0,9);originYaw=originPitch=sensorYaw=sensorPitch=0; stickYaw=stickPitch=0; yaw=pitch=0; menuYaw=menuPitch=0; status="View recentered"; }
    void toggleMenu() { menu=!menu; learning=-1; clearInput(); menuYaw=yaw; menuPitch=pitch; gazeIndex=-1; gazeStart=SystemClock.uptimeMillis(); }
    private void suspendGaze(){gazeSuspendedUntil=SystemClock.uptimeMillis()+2000;gazeIndex=-1;gazeLatched=false;gazeStart=SystemClock.uptimeMillis();}
    private int count() { return tab==0?values.length+4:tab==1?16:4; }
    private void change(int direction) {
        if(tab==0 && selected<values.length) { int parameter=displayOrder[selected];values[parameter]=clamp(values[parameter]+steps[parameter]*direction,mins[parameter],maxs[parameter]); save(); }
        else activate();
    }
    private void activate() {
        if(tab==0) {
            if(selected<values.length)change(1);
            else if(selected==values.length){trackingEnabled=!trackingEnabled;save();}
            else if(selected==values.length+1)recenter();
            else if(selected==values.length+2){px=pz=0;ammo=30;shots=0;status="Test world reset";}
            else toggleMenu();
        } else if(tab==1) {
            if(selected<8){learning=selected;status="Press a gamepad button. Back cancels.";}
            else if(selected==8){swapSticks=!swapSticks;save();}
            else if(selected==9){invertLook=!invertLook;save();}
            else if(selected==10){System.arraycopy(defaults,0,bindings,0,8);save();status="Default bindings restored";}
            else if(selected==11){String json=exportProfile(); ((android.content.ClipboardManager)getContext().getSystemService(Context.CLIPBOARD_SERVICE)).setPrimaryClip(ClipData.newPlainText("Portal Z profile",json));status="Profile copied to clipboard";}
            else if(selected==12)importProfile();
            else if(selected==13)recenter();
            else if(selected==14){gazeEnabled=!gazeEnabled;save();suspendGaze();status=gazeEnabled?"Gaze enabled; controller and touch take priority":"Gaze disabled; settings stay fixed";}
            else toggleMenu();
        } else {
            if(selected==0){tab=0;selected=values.length+3;toggleMenu();}
            else if(selected==1){tab=1;selected=0;}
            else if(selected==2){tab=0;selected=0;}
            else toggleMenu();
        }
    }
    private String exportProfile() {
        org.json.JSONObject root=new org.json.JSONObject();
        try{root.put("schema","portal-z/2");org.json.JSONArray v=new org.json.JSONArray(),b=new org.json.JSONArray();for(float f:values)v.put(f);for(int k:bindings)b.put(k);root.put("display",v);root.put("buttons",b);root.put("tracking",trackingEnabled);root.put("swapSticks",swapSticks);root.put("invertLook",invertLook);}catch(Exception ignored){}
        return root.toString();
    }
    private void importProfile(){
        try{
            android.content.ClipboardManager c=(android.content.ClipboardManager)getContext().getSystemService(Context.CLIPBOARD_SERVICE);
            if(!c.hasPrimaryClip())throw new Exception();
            org.json.JSONObject root=new org.json.JSONObject(c.getPrimaryClip().getItemAt(0).coerceToText(getContext()).toString());
            if(!"portal-z/1".equals(root.getString("schema"))&&!"portal-z/2".equals(root.getString("schema")))throw new Exception();
            org.json.JSONArray v=root.getJSONArray("display"),b=root.getJSONArray("buttons");
            if((v.length()!=10&&v.length()!=12)||b.length()!=8)throw new Exception();
            float[] nv=new float[12];int[] nb=new int[8];Set<Integer> unique=new HashSet<>();
            for(int i=0;i<v.length();i++){nv[i]=(float)v.getDouble(i);if(!Float.isFinite(nv[i])||nv[i]<mins[i]||nv[i]>maxs[i])throw new Exception();}
            for(int i=0;i<8;i++){nb[i]=b.getInt(i);if(!KeyEvent.isGamepadButton(nb[i])||!unique.add(nb[i]))throw new Exception();}
            System.arraycopy(nv,0,values,0,12);System.arraycopy(nb,0,bindings,0,8);
            trackingEnabled=root.optBoolean("tracking",true);swapSticks=root.optBoolean("swapSticks",false);invertLook=root.optBoolean("invertLook",false);save();status="Profile imported";
        }catch(Exception e){status="Import failed: copy a valid Portal Z profile first";}
    }
    boolean key(KeyEvent event) {
        int k=event.getKeyCode(); boolean down=event.getAction()==KeyEvent.ACTION_DOWN;
        boolean pad=(event.getSource() & InputDevice.SOURCE_GAMEPAD)==InputDevice.SOURCE_GAMEPAD || KeyEvent.isGamepadButton(k);
        if(k==KeyEvent.KEYCODE_BACK){if(down&&event.getRepeatCount()==0){if(learning>=0)learning=-1;else toggleMenu();}return true;}
        if(!pad && !(k>=19&&k<=23) && k!=KeyEvent.KEYCODE_ENTER && k!=KeyEvent.KEYCODE_ESCAPE)return false;
        if(!down){held.remove(k);return true;}
        if(event.getRepeatCount()!=0)return true;
        suspendGaze();
        controller=event.getDevice()==null?"Controller":event.getDevice().getName();
        if(learning>=0){
            if(!KeyEvent.isGamepadButton(k)){status="Choose a gamepad button";return true;}
            int previous=bindings[learning];for(int i=0;i<8;i++)if(i!=learning&&bindings[i]==k)bindings[i]=previous;
            bindings[learning]=k;learning=-1;save();status="Binding saved; existing duplicate swapped";return true;
        }
        if(menu){
            if(k==19)selected=(selected+count()-1)%count();
            else if(k==20)selected=(selected+1)%count();
            else if(k==21)change(-1);
            else if(k==22)change(1);
            else if(k==96||k==23||k==66)activate();
            else if(k==102){tab=(tab+2)%3;selected=0;}
            else if(k==103){tab=(tab+1)%3;selected=0;}
            else if(k==108||k==111)toggleMenu();
        }else{
            held.add(k);
            for(int i=0;i<8;i++)if(bindings[i]==k){if(i==2){ammo=30;status="Reloaded";}else if(i==3&&jump==0)jump=.01f;else if(i==6)toggleMenu();else if(i==7)recenter();}
        }
        return true;
    }
    boolean motion(MotionEvent e) {
        if((e.getSource()&InputDevice.SOURCE_JOYSTICK)!=InputDevice.SOURCE_JOYSTICK)return false;
        controller=e.getDevice()==null?"Gamepad":e.getDevice().getName();
        float x=dead(e.getAxisValue(MotionEvent.AXIS_X)),y=dead(e.getAxisValue(MotionEvent.AXIS_Y));
        float rx=dead(e.getAxisValue(MotionEvent.AXIS_Z)),ry=dead(e.getAxisValue(MotionEvent.AXIS_RZ));
        if(Math.abs(x)+Math.abs(y)+Math.abs(rx)+Math.abs(ry)+Math.abs(e.getAxisValue(MotionEvent.AXIS_HAT_X))+Math.abs(e.getAxisValue(MotionEvent.AXIS_HAT_Y))>.1f)suspendGaze();
        if(swapSticks){moveX=rx;moveY=ry;lookX=x;lookY=y;}else{moveX=x;moveY=y;lookX=rx;lookY=ry;}
        if(menu && SystemClock.uptimeMillis()-lastHat>250){
            float hx=e.getAxisValue(MotionEvent.AXIS_HAT_X),hy=e.getAxisValue(MotionEvent.AXIS_HAT_Y);
            if(hy<-.5f)selected=(selected+count()-1)%count();else if(hy>.5f)selected=(selected+1)%count();
            else if(hx<-.5f)change(-1);else if(hx>.5f)change(1);
            if(Math.abs(hx)>.5f||Math.abs(hy)>.5f)lastHat=SystemClock.uptimeMillis();
        }
        return true;
    }
    private boolean action(int i){return held.contains(bindings[i]);}
    private void step(float dt) {
        if(startupRecenterAt!=0 && SystemClock.uptimeMillis()>=startupRecenterAt){recenter();startupRecenterAt=0;}
        if(!menu){stickYaw+=lookX*dt*1.8f*values[7];stickPitch=clamp(stickPitch+lookY*dt*values[7]*(invertLook?1:-1),-.9f,.9f);}
        yaw=wrap(stickYaw+(trackingEnabled&&hasTracking?wrap(sensorYaw-originYaw):0));
        pitch=clamp(stickPitch+(trackingEnabled&&hasTracking?(sensorPitch-originPitch):0),-1.2f,1.2f);
        if(!menu){
            if(gazeEnabled&&trackingEnabled&&hasTracking&&pitch<-.8f){if(menuDwell==0)menuDwell=SystemClock.uptimeMillis();if(SystemClock.uptimeMillis()-menuDwell>1400){toggleMenu();menuDwell=0;return;}}else menuDwell=0;
            float speed=action(5)?4:action(1)?1:2.2f;
            px=clamp(px+((float)Math.cos(yaw)*moveX-(float)Math.sin(yaw)*moveY)*speed*dt,-18,18);
            pz=clamp(pz+(-(float)Math.sin(yaw)*moveX-(float)Math.cos(yaw)*moveY)*speed*dt,-12,40);
            if(jump>0){jump+=dt*3;if(jump>Math.PI)jump=0;}
            if(action(0)&&SystemClock.uptimeMillis()-lastShot>200&&ammo>0){ammo--;shots++;lastShot=SystemClock.uptimeMillis();status="Test shot "+shots;}
        }
    }
    @Override protected void onDraw(Canvas output) {
        super.onDraw(output);long now=SystemClock.uptimeMillis();float dt=Math.min(.05f,Math.max(0,(now-lastTime)/1000f));lastTime=now;step(dt);
        int w=getWidth()/2,h=getHeight();if(w<1||h<1)return;
        int bw=Math.min(w,720),bh=Math.max(1,(int)((float)h*bw/w));
        if(eyeBitmap==null||eyeBitmap.getWidth()!=bw||eyeBitmap.getHeight()!=bh){if(eyeBitmap!=null)eyeBitmap.recycle();eyeBitmap=Bitmap.createBitmap(bw,bh,Bitmap.Config.ARGB_8888);eyeCanvas=new Canvas(eyeBitmap);}
        output.drawColor(Color.BLACK);
        for(int eye=0;eye<2;eye++){
            eyeCanvas.drawColor(Color.rgb(9,18,32));world(eyeCanvas,bw,bh,eye);
            if(menu)panel(eyeCanvas,bw,bh,eye==0);else hud(eyeCanvas,bw,bh);
            output.save();output.translate(eye*w,0);output.scale((float)w/bw,(float)h/bh);
            float cx=bw/2f+(eye==0?1:-1)*values[3]*bw+values[10+eye]*bw,cy=bh/2f+values[eye==0?4:5]*bh;
            for(int row=0;row<=16;row++)for(int col=0;col<=16;col++){
                float nx=2*col/16f-1,ny=2*row/16f-1,r2=nx*nx+ny*ny;
                float warp=values[1]*(1+values[6]*r2);int i=(row*17+col)*2;
                mesh[i]=cx+nx*bw/2f*warp;mesh[i+1]=cy+ny*bh/2f*warp;
            }
            output.drawBitmapMesh(eyeBitmap,16,16,mesh,0,null,0,paint);output.restore();
        }
        paint.setColor(Color.BLACK);output.drawRect(w-2,0,w+2,h,paint);
        if(menu&&gazeEnabled&&trackingEnabled&&hasTracking&&now>=gazeSuspendedUntil)gaze(now,bw,bh);
        if(running)postInvalidateDelayed(16);
    }
    private static class Face {float[] xy;float z;int color;Face(float[] p,float d,int c){xy=p;z=d;color=c;}}
    private float[] project(float x,float y,float z,float eye,int w,int h){
        float dx=x-px,dz=z-pz,dy=y-(action(1)?1.05f:1.65f)-(float)Math.sin(jump)*.5f;
        float xx=(float)(dx*Math.cos(yaw)-dz*Math.sin(yaw))-eye;
        float zz=(float)(dx*Math.sin(yaw)+dz*Math.cos(yaw));
        float yy=(float)(dy*Math.cos(pitch)-zz*Math.sin(pitch));
        float depth=(float)(dy*Math.sin(pitch)+zz*Math.cos(pitch));
        float f=(float)(w/2f/Math.tan(Math.toRadians(action(4)?values[0]*.75:values[0])/2));
        return new float[]{w/2f+xx*f/Math.max(.06f,depth),h/2f-yy*f/Math.max(.06f,depth),depth};
    }
    private void line(Canvas c,float x,float z,float xx,float zz,float eye,int w,int h){
        float[] a=project(x,0,z,eye,w,h),b=project(xx,0,zz,eye,w,h);
        if(a[2]>.1&&b[2]>.1){paint.setColor(0xff225267);paint.setStrokeWidth(1);c.drawLine(a[0],a[1],b[0],b[1],paint);}
    }
    private void world(Canvas c,int w,int h,int eyeIndex){
        float eye=(eyeIndex==0?-.5f:.5f)*values[2]/1000;
        for(int i=-20;i<=40;i+=2){line(c,-20,i,20,i,eye,w,h);line(c,i,-15,i,45,eye,w,h);}
        faces.clear();
        for(int n=0;n<boxes.length;n++){
            float[] b=boxes[n];float[][] vertices=new float[8][];
            for(int i=0;i<8;i++)vertices[i]=project(b[0]+((i&1)==0?-b[3]:b[3]),b[1]+((i&2)==0?-b[4]:b[4]),b[2]+((i&4)==0?-b[5]:b[5]),eye,w,h);
            int[][] quads={{0,1,3,2},{4,6,7,5},{0,4,5,1},{2,3,7,6},{0,2,6,4},{1,5,7,3}};
            int[] colors={0xff216c80,0xff15435b,0xff174454,0xff6de1db,0xff367e92,0xff275d77};
            if(b.length>6){int style=(int)b[6];if(style==1)colors=new int[]{0xffa9bbc4,0xff5b6c7a,0xff596575,0xffe4edf1,0xff899ca9,0xff758896};if(style==2)colors=new int[]{0xff53fff0,0xff1d8496,0xff17667b,0xffb9fff7,0xff51e5d8,0xff37a6bb};if(style==3)colors=new int[]{0xff203a59,0xff102035,0xff101b2b,0xff42698b,0xff294462,0xff1b3049};}
            for(int j=0;j<6;j++){float[] xy=new float[8];float depth=0;boolean visible=true;for(int k=0;k<4;k++){float[] v=vertices[quads[j][k]];xy[k*2]=v[0];xy[k*2+1]=v[1];depth+=v[2];if(v[2]<.1f)visible=false;}if(visible)faces.add(new Face(xy,depth/4,colors[j]));}
        }
        Collections.sort(faces,(a,b)->Float.compare(b.z,a.z));
        Path path=new Path();for(Face f:faces){path.reset();path.moveTo(f.xy[0],f.xy[1]);for(int i=2;i<8;i+=2)path.lineTo(f.xy[i],f.xy[i+1]);path.close();paint.setColor(f.color);paint.setStyle(Paint.Style.FILL);c.drawPath(path,paint);paint.setColor(0xff77ebea);paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(.7f);c.drawPath(path,paint);paint.setStyle(Paint.Style.FILL);}
        if(values[9]>0){paint.setShader(new RadialGradient(w/2f,h/2f,w*.7f,new int[]{0x00000000,((int)(values[9]*255)<<24)},null,Shader.TileMode.CLAMP));c.drawRect(0,0,w,h,paint);paint.setShader(null);}
    }
    private void text(Canvas c,String s,float x,float y,float size,int color){paint.setColor(color);paint.setTextSize(size);paint.setTypeface(Typeface.create("sans-serif",Typeface.NORMAL));c.drawText(s,x,y,paint);}
    private void hud(Canvas c,int w,int h){
        float u=w/480f;
        text(c,"PORTAL Z  /  FRONTIER LAB",24*u,32*u,14*u,0xff74e8e3);
        text(c,"DEMO WORLD · NOT BGMI",24*u,52*u,10*u,0xffa1b6c8);
        text(c,ammo+" / 30    "+(action(4)?"AIM":"READY"),24*u,h-58*u,14*u,Color.WHITE);
        text(c,"START: SETTINGS · LOOK DOWN + DWELL: MENU",24*u,h-38*u,9*u,0xffa1b6c8);
        paint.setColor(Color.WHITE);paint.setStrokeWidth(1.5f);c.drawLine(w/2f-7,h/2f,w/2f+7,h/2f,paint);c.drawLine(w/2f,h/2f-7,w/2f,h/2f+7,paint);
        if(SystemClock.uptimeMillis()-lastShot<100){paint.setColor(0xffe8d892);c.drawCircle(w*.7f,h*.72f,9*u,paint);}
        String[] buttons={"FIRE","JUMP","LOAD","MENU"};for(int i=0;i<4;i++){float x=w-(i%2==0?90:165)*u,y=h-(i<2?105:155)*u;paint.setColor(0x9945647c);c.drawRoundRect(x,y,x+65*u,y+36*u,8,8,paint);text(c,buttons[i],x+7*u,y+23*u,10*u,Color.WHITE);}
        paint.setColor(0x554fbdc9);c.drawCircle(85*u,h-110*u,38*u,paint);
        text(c,"DRAG",65*u,h-106*u,10*u,Color.WHITE);
    }
    private String displayValue(int i){
        if(i==0)return Math.round(values[i])+"°";
        if(i==1)return Math.round(values[i]*100)+"%";
        if(i==2)return Math.round(values[i])+" mm";
        return String.format(java.util.Locale.US,"%.3f",values[i]);
    }
    private String row(int i){
        if(tab==0){if(i<values.length){int parameter=displayOrder[i];return labels[parameter]+"     "+displayValue(parameter);}return new String[]{"Head tracking: "+(trackingEnabled?"ON":"OFF"),"Recenter view","Reset test world","ENTER STEREO WORLD"}[i-values.length];}
        if(tab==1){if(i<8)return actions[i]+"    "+KeyEvent.keyCodeToString(bindings[i]).replace("KEYCODE_BUTTON_","");return new String[]{"Stick layout: "+(swapSticks?"SWAPPED":"STANDARD"),"Invert look: "+(invertLook?"ON":"OFF"),"Restore default bindings","Copy profile","Import clipboard profile","Recenter view","Gaze selection: "+(gazeEnabled?"ON":"OFF"),"RETURN TO WORLD"}[i-8];}
        return new String[]{"Try the stereo test world","Edit controller bindings","Calibrate the viewer","RETURN TO WORLD"}[i];
    }
    private void panel(Canvas c,int w,int h,boolean hits){
        float u=Math.min(w/480f,h/360f),pw=420*u,ph=334*u;
        float ox=w/2f-pw/2-(gazeEnabled?wrap(yaw-menuYaw):0)*w*.75f,oy=h/2f-ph/2+(gazeEnabled?pitch-menuPitch:0)*w*.75f;
        paint.setColor(0xed0b1425);c.drawRoundRect(ox,oy,ox+pw,oy+ph,14*u,14*u,paint);
        text(c,"PORTAL Z",ox+18*u,oy+28*u,20*u,0xff7ef0e9);
        text(c,"0.1 · CALIBRATION PROTOTYPE",ox+145*u,oy+27*u,10*u,0xff9cacc0);
        String[] tabs={"DISPLAY","CONTROLS","BGMI STATUS"};
        for(int i=0;i<3;i++){float x=ox+(18+i*131)*u;paint.setColor(tab==i?0xff205466:0xff17283f);c.drawRoundRect(x,oy+40*u,x+123*u,oy+64*u,5,5,paint);text(c,tabs[i],x+8*u,oy+56*u,10*u,Color.WHITE);if(hits)hitRects[16+i].set(x,oy+40*u,x+123*u,oy+64*u);}
        int start=selected<8?0:Math.min(count()-8,selected-4);if(tab==2)start=0;
        for(int i=0;i<16;i++)if(hits)hitRects[i].setEmpty();
        if(hits){hitRects[22].setEmpty();hitRects[23].setEmpty();}
        float rowTop=oy+(tab==2?174:76)*u;
        if(tab==2){
            String[] lines={"BGMI INTEGRATION: UNAVAILABLE","No live capture, touch forwarding or native VR.","This build does not launch or modify BGMI.","True in-world BGMI needs publisher integration.","Stereo depth works in our own test world."};
            for(int i=0;i<lines.length;i++)text(c,lines[i],ox+18*u,oy+(86+i*16)*u,i==0?12*u:10*u,i==0?0xffffcb82:0xffb6c6d7);
        }
        for(int r=0;r<8&&start+r<count();r++){
            int i=start+r;float y=rowTop+r*23*u;if(tab==2)y=rowTop+r*23*u;
            paint.setColor(i==selected?0xff234958:0x5522354c);c.drawRoundRect(ox+14*u,y,ox+pw-14*u,y+21*u,4,4,paint);
            text(c,(learning==i&&tab==1?"PRESS BUTTON: ":"")+row(i),ox+22*u,y+15*u,11*u,i==selected?0xffa6fff0:0xffd0dce7);
            if(tab==0&&i<values.length){text(c,"−",ox+pw-64*u,y+15*u,13*u,Color.WHITE);text(c,"+",ox+pw-32*u,y+15*u,13*u,Color.WHITE);if(hits&&i==selected){hitRects[22].set(ox+pw-80*u,y,ox+pw-48*u,y+21*u);hitRects[23].set(ox+pw-48*u,y,ox+pw-14*u,y+21*u);}}
            if(hits)hitRects[i].set(ox+14*u,y,ox+pw-14*u,y+21*u);
        }
        String[] nav={"↑ PREVIOUS","↓ NEXT","CLOSE"};
        for(int i=0;i<3;i++){float x=ox+(18+i*131)*u;paint.setColor(0xff17283f);c.drawRoundRect(x,oy+264*u,x+123*u,oy+283*u,4,4,paint);text(c,nav[i],x+10*u,oy+277*u,9*u,Color.WHITE);if(hits)hitRects[19+i].set(x,oy+264*u,x+123*u,oy+283*u);}
        text(c,"LB/RB tabs · D-pad adjust · A confirm · Gaze dwell",ox+18*u,oy+296*u,9*u,0xffa1b6c8);
        String s=learning>=0?"Waiting for button · Back cancels":status;
        text(c,s.length()>62?s.substring(0,59)+"...":s,ox+18*u,oy+310*u,9*u,0xffffcf8a);
        text(c,"Alignment only; not prescription correction. "+(tab==1?controller:"Uncalibrated lens warp starts at zero."),ox+18*u,oy+324*u,8*u,0xff8a9db5);
        paint.setColor(0xfff2fcff);c.drawCircle(w/2f,h/2f,2.5f*u,paint);
        if(gazeIndex>=0&&!gazeLatched){paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(2*u);c.drawArc(w/2f-8*u,h/2f-8*u,w/2f+8*u,h/2f+8*u,-90,clamp((SystemClock.uptimeMillis()-gazeStart)/1400f,0,1)*360,false,paint);paint.setStyle(Paint.Style.FILL);}
    }
    private void gaze(long now,int w,int h){
        int target=-1;for(int i=0;i<24;i++)if(hitRects[i].contains(w/2f,h/2f))target=i;
        if(target!=gazeIndex){gazeIndex=target;gazeStart=now;gazeLatched=false;}
        if(target>=0&&!gazeLatched&&now-gazeStart>1400){
            if(tab==0&&target<values.length)selected=target;else menuTarget(target);
            gazeLatched=true;
        }
    }
    private void menuTarget(int target){
        if(target==22)change(-1);
        else if(target==23)change(1);
        else if(target==19)selected=(selected+count()-1)%count();
        else if(target==20)selected=(selected+1)%count();
        else if(target==21)toggleMenu();
        else if(target>=16){tab=target-16;selected=0;}
        else{selected=target;activate();}
    }
    @Override public boolean onTouchEvent(MotionEvent e){
        int index=e.getActionIndex(),id=e.getPointerId(index);int a=e.getActionMasked();
        if(a==MotionEvent.ACTION_DOWN||a==MotionEvent.ACTION_POINTER_DOWN||a==MotionEvent.ACTION_MOVE)suspendGaze();
        float ew=getWidth()/2f,scale=eyeBitmap==null?1:eyeBitmap.getWidth()/ew;
        float x=(e.getX(index)%ew)*scale,y=e.getY(index)*scale;
        if(a==MotionEvent.ACTION_CANCEL){clearInput();return true;}
        if(a==MotionEvent.ACTION_UP||a==MotionEvent.ACTION_POINTER_UP){if(id==leftPointer){leftPointer=-1;moveX=moveY=0;}if(id==rightPointer){rightPointer=-1;lookX=lookY=0;}held.remove(bindings[0]);return true;}
        if(a==MotionEvent.ACTION_DOWN||a==MotionEvent.ACTION_POINTER_DOWN){
            if(menu){
                // Unwarp the touched coordinate so calibration does not break settings hit tests.
                int eye=e.getX(index)<ew?0:1;float bw=eyeBitmap==null?ew:eyeBitmap.getWidth(),bh=eyeBitmap==null?getHeight():eyeBitmap.getHeight();
                float nx=(x-bw/2-(eye==0?1:-1)*values[3]*bw-values[10+eye]*bw)/(bw/2*values[1]),ny=(y-bh/2-values[eye==0?4:5]*bh)/(bh/2*values[1]);
                float ux=nx,uy=ny;for(int j=0;j<8;j++){float f=1+values[6]*(ux*ux+uy*uy);ux=nx/f;uy=ny/f;}x=bw/2+ux*bw/2;y=bh/2+uy*bh/2;
                for(int i=16;i<22;i++)if(hitRects[i].contains(x,y)){menuTarget(i);return true;}
                for(int i=0;i<count();i++)if(hitRects[i].contains(x,y)){selected=i;RectF r=hitRects[i];if(tab==0&&i<values.length)change(x>r.right-r.width()*.12f?1:-1);else activate();return true;}
                return true;
            }
            float w=eyeBitmap==null?ew:eyeBitmap.getWidth(),h=eyeBitmap==null?getHeight():eyeBitmap.getHeight(),u=w/480;
            for(int i=0;i<4;i++){float bx=w-(i%2==0?90:165)*u,by=h-(i<2?105:155)*u;if(x>=bx&&x<=bx+65*u&&y>=by&&y<=by+36*u){if(i==0)held.add(bindings[0]);if(i==1)jump=.01f;if(i==2)ammo=30;if(i==3)toggleMenu();return true;}}
            if(x<w*.45f){leftPointer=id;leftTouchX=e.getX(index);leftTouchY=e.getY(index);}else{rightPointer=id;rightTouchX=e.getX(index);rightTouchY=e.getY(index);}
        }
        if(a==MotionEvent.ACTION_MOVE&&!menu){for(int i=0;i<e.getPointerCount();i++){int p=e.getPointerId(i);if(p==leftPointer){moveX=clamp((e.getX(i)-leftTouchX)/80,-1,1);moveY=clamp((e.getY(i)-leftTouchY)/80,-1,1);}if(p==rightPointer){lookX=clamp((e.getX(i)-rightTouchX)/100,-1,1);lookY=clamp((e.getY(i)-rightTouchY)/100,-1,1);}}}
        return true;
    }
}
