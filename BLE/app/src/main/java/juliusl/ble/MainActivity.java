package juliusl.ble;

import android.Manifest;
import android.app.ListActivity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Handler;
import android.support.v4.app.ActivityCompat;
import android.support.v4.widget.SwipeRefreshLayout;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.ListView;
import android.widget.Toast;

import juliusl.ble.adapter.BluetoothLeListAdapter;

public class MainActivity extends ListActivity implements SwipeRefreshLayout.OnRefreshListener {

    private SwipeRefreshLayout mSwipeRefreshLayout;

    private static final int REQUEST_CODE_COARSE_PERMISSION = 1;
    private static final int REQUEST_CODE_BLUETOOTH_PERMISSION = 2;

    private static long SCAN_PERIOD = 10000;

    private BluetoothManager mBtManager;
    private BluetoothAdapter mBtAdapter;

    private BluetoothLeScanner mBLEscanner;
    private BluetoothLeListAdapter mBLEDeviceListAdapter;

    private Handler mHandler;
    private boolean mScanning;

    private ScanCallback mBluetoothScanCallBack = new ScanCallback() {
        @Override
        public void onScanResult(int callbackType, final ScanResult result) {
            super.onScanResult(callbackType, result);
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    Log.d("DEV_DEBUG", "ON");
                    mBLEDeviceListAdapter.addDevice(result.getDevice());
                    mBLEDeviceListAdapter.notifyDataSetChanged();
                }
            });
        }

        @Override
        public void onScanFailed(int errorCode){
            super.onScanFailed(errorCode);
            Log.d("DEV_DEBUG", String.valueOf(errorCode));
            //Toast.makeText(MainActivity.this, errorCode, Toast.LENGTH_LONG).show();
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mSwipeRefreshLayout = (SwipeRefreshLayout) findViewById(R.id.swiperefresh);
        mSwipeRefreshLayout.setOnRefreshListener(this);

        mHandler = new Handler();

        mBtManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        mBtAdapter = mBtManager.getAdapter();
        mBLEDeviceListAdapter = new BluetoothLeListAdapter(this);

        if(!getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)){
            Toast.makeText(this, "ADS", Toast.LENGTH_SHORT).show();
            finish();
        }
        mBLEscanner = mBtAdapter.getBluetoothLeScanner();

        askPermissions();
    }

    @Override
    protected void onResume(){
        super.onResume();

        setListAdapter(mBLEDeviceListAdapter);
        if(mBtAdapter.isEnabled()){
            //scanBtDevices();
        }
    }

    @Override
    protected void onPause(){
        super.onPause();
        mBLEDeviceListAdapter.clear();
    }

    @Override
    protected void onListItemClick(ListView list, View view, int pos, long id){
        final BluetoothDevice device = mBLEDeviceListAdapter.getDevice(pos);
        Toast.makeText(this,  "Device: " + device.getName() + "\nAddress: " + device.getAddress(), Toast.LENGTH_SHORT).show();

        Intent intent = new Intent(this, DeviceDescriptionActivity.class);
        intent.putExtra("Bluetooth_Device", device);
        startActivity(intent);
    }

    public void scanBtDevices(View view) {
        if (mBLEscanner != null) {
            if (true) {
                // Stops scanning after a pre-defined scan period.
                mHandler.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        mScanning = false;
                        mBLEscanner.stopScan(mBluetoothScanCallBack);
                    }
                }, SCAN_PERIOD);
                mScanning = true;
                mBLEscanner.startScan(mBluetoothScanCallBack);
            } else {
                mScanning = false;
                mBLEscanner.stopScan(mBluetoothScanCallBack);
            }
        }
    }

    public void scanBtDevices() {
        if (mBLEscanner != null) {
            if (true) {
                // Stops scanning after a pre-defined scan period.
                mHandler.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        mScanning = false;
                        mBLEscanner.stopScan(mBluetoothScanCallBack);
                    }
                }, SCAN_PERIOD);
                mScanning = true;
                mBLEscanner.startScan(mBluetoothScanCallBack);
            } else {
                mScanning = false;
                mBLEscanner.stopScan(mBluetoothScanCallBack);
            }
        }
    }

    private void askPermissions() {

        if (this.checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED){
            if (ActivityCompat.shouldShowRequestPermissionRationale(this, Manifest.permission.ACCESS_COARSE_LOCATION)) {

            } else {
                ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.ACCESS_COARSE_LOCATION}, REQUEST_CODE_COARSE_PERMISSION);
                Log.d("DEV_DEBUG", "Coarse location permission");
            }
            return;
        } else if(!mBtAdapter.isEnabled()){
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableBtIntent, REQUEST_CODE_BLUETOOTH_PERMISSION);
        } else {
            Log.d("DEV_DEBUG", "All permissions ok!");
        }

        if(this.checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED){
            requestPermissions(new String[]{Manifest.permission.ACCESS_COARSE_LOCATION}, REQUEST_CODE_COARSE_PERMISSION);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults){
        switch (requestCode) {
            case REQUEST_CODE_COARSE_PERMISSION:{
                if(grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED){
                    Toast.makeText(this, "Coarse permisson ok", Toast.LENGTH_SHORT).show();
                } else {

                }
                return;
            }
        }
    }

    @Override
    public void onRefresh() {
        Toast.makeText(this, R.string.refresh_text, Toast.LENGTH_SHORT).show();
        scanBtDevices();

        new Handler().postDelayed(new Runnable() {
            @Override
            public void run() {
                mSwipeRefreshLayout.setRefreshing(false);
            }
        }, 2000);
    }
}