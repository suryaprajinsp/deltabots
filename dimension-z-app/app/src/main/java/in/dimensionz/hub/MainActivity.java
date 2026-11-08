package in.dimensionz.hub;

import android.app.Activity;
import android.os.Bundle;
import android.content.Intent;
import android.net.Uri;
import android.graphics.*;
import android.graphics.drawable.GradientDrawable;
import android.view.*;
import android.widget.*;

/** Small native launcher. No account, embedded API key, tracking or background service. */
public final class MainActivity extends Activity {
    private static final String REVIEW="https://suryaprajinsp.github.io/deltabots/";
    private int dp(float v){return Math.round(v*getResources().getDisplayMetrics().density);}
    @Override public void onCreate(Bundle state){
        super.onCreate(state);getWindow().setStatusBarColor(0xff081625);getWindow().setNavigationBarColor(0xff081625);
        ScrollView scroll=new ScrollView(this);scroll.setFillViewport(true);scroll.setBackgroundColor(0xff081625);
        LinearLayout body=new LinearLayout(this);body.setOrientation(1);body.setPadding(dp(26),dp(20),dp(26),dp(30));scroll.addView(body);
        ImageView logo=new ImageView(this);logo.setImageResource(in.dimensionz.hub.R.drawable.dimension_z_icon);
        body.addView(logo,new LinearLayout.LayoutParams(dp(100),dp(100)));
        text(body,"DIMENSION Z",30,0xffe7faff,14);
        text(body,"BEYOND THE THIRD AXIS",11,0xff59f4df,4);
        text(body,"Worlds in development.\nOne place to explore them.",21,0xffb6c8dc,20);
        card(body,"DELTA BOTS","Open the installed Unreal game",()->launch("com.dimensionz.deltabots",REVIEW));
        card(body,"PORTAL Z","VR display and controller calibration",()->launch("in.dimensionz.portal",REVIEW+"downloads/"));
        card(body,"EXPLORE ONLINE","World previews, builds and development notes",()->web(REVIEW));
        card(body,"DIMENSION Z HUB","Visit Dimension Z on ShopperSpot",()->web("https://shopperspot.in/dimension-z/"));
        text(body,"VR / AR / XR is our direction. This prototype opens installed apps and online previews; AR features are not implemented.",12,0xff8ca1b7,20);
        text(body,"0.1.0 · Development launcher · Android 8+",11,0xff6e879e,12);
        setContentView(scroll);
    }
    private void text(LinearLayout parent,String value,int size,int color,int top){
        TextView t=new TextView(this);t.setText(value);t.setTextSize(size);t.setTextColor(color);t.setLineSpacing(dp(3),1);
        LinearLayout.LayoutParams p=new LinearLayout.LayoutParams(-1,-2);p.topMargin=dp(top);parent.addView(t,p);
    }
    private void card(LinearLayout parent,String title,String caption,Runnable action){
        Button b=new Button(this);b.setAllCaps(false);b.setText(title+"  ↗\n"+caption);b.setTextSize(15);b.setTextColor(0xffe7faff);b.setGravity(Gravity.LEFT|Gravity.CENTER_VERTICAL);b.setPadding(dp(18),dp(15),dp(18),dp(15));b.setMinHeight(dp(82));
        GradientDrawable normal=new GradientDrawable(GradientDrawable.Orientation.LEFT_RIGHT,new int[]{0xff112a3c,0xff172639});normal.setCornerRadius(dp(14));normal.setStroke(dp(1),0xff28546a);
        android.graphics.drawable.StateListDrawable background=new android.graphics.drawable.StateListDrawable();
        GradientDrawable focus=new GradientDrawable();focus.setColor(0xff24576a);focus.setCornerRadius(dp(14));focus.setStroke(dp(2),0xff59f4df);
        background.addState(new int[]{android.R.attr.state_focused},focus);background.addState(new int[]{android.R.attr.state_pressed},focus);background.addState(new int[]{},normal);b.setBackground(background);
        LinearLayout.LayoutParams p=new LinearLayout.LayoutParams(-1,-2);p.topMargin=dp(12);parent.addView(b,p);b.setOnClickListener(v->action.run());
    }
    private void launch(String id,String fallback){
        Intent intent=getPackageManager().getLaunchIntentForPackage(id);
        if(intent==null){Toast.makeText(this,"App is not installed. Opening its review/download page.",Toast.LENGTH_LONG).show();web(fallback);return;}
        try{startActivity(intent);}catch(android.content.ActivityNotFoundException e){web(fallback);}
    }
    private void web(String url){try{startActivity(new Intent(Intent.ACTION_VIEW,Uri.parse(url)));}catch(android.content.ActivityNotFoundException e){Toast.makeText(this,"Install a browser to open this page.",Toast.LENGTH_LONG).show();}}
}
