package in.dimensionz.portal;

import android.app.Activity;
import android.os.Bundle;
import android.view.*;
import android.hardware.*;

public final class MainActivity extends Activity implements SensorEventListener, android.hardware.input.InputManager.InputDeviceListener {
    private PortalView portal;
    private SensorManager sensors;
    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getWindow().getDecorView().setSystemUiVisibility(5894);
        portal = new PortalView(this);
        setContentView(portal);
        sensors = (SensorManager)getSystemService(SENSOR_SERVICE);
        ((android.hardware.input.InputManager)getSystemService(INPUT_SERVICE)).registerInputDeviceListener(this,null);
    }
    @Override public void onResume() {
        super.onResume();
        Sensor sensor = sensors.getDefaultSensor(Sensor.TYPE_GAME_ROTATION_VECTOR);
        if (sensor == null) sensor = sensors.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR);
        portal.hasTracking = sensor != null;
        if (sensor != null) sensors.registerListener(this, sensor, SensorManager.SENSOR_DELAY_GAME);
        portal.running = true; portal.resetClock(); portal.invalidate();
    }
    @Override public void onPause() { portal.running = false; portal.clearInput(); sensors.unregisterListener(this); super.onPause(); }
    @Override public void onSensorChanged(SensorEvent event) { portal.rotation(event.values); }
    @Override public void onAccuracyChanged(Sensor sensor, int accuracy) {}
    @Override public void onInputDeviceAdded(int id) {}
    @Override public void onInputDeviceChanged(int id) { portal.clearInput(); }
    @Override public void onInputDeviceRemoved(int id) { portal.clearInput(); }
    @Override public void onDestroy() { ((android.hardware.input.InputManager)getSystemService(INPUT_SERVICE)).unregisterInputDeviceListener(this); super.onDestroy(); }
    @Override public boolean dispatchKeyEvent(KeyEvent event) {
        if (portal.key(event)) return true;
        return super.dispatchKeyEvent(event);
    }
    @Override public boolean onGenericMotionEvent(MotionEvent event) {
        if (portal.motion(event)) return true;
        return super.onGenericMotionEvent(event);
    }
    @Override public void onBackPressed() { portal.toggleMenu(); }
}
