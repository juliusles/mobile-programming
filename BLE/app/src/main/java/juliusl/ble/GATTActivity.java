package juliusl.ble;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.content.Intent;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.UUID;

import static android.nfc.NfcAdapter.EXTRA_DATA;

public class GATTActivity extends AppCompatActivity {

    private BluetoothGatt BtGatt;

    private static final int STATE_DISCONNECTED = 0;
    private static final int STATE_CONNECTING = 1;
    private static final int STATE_CONNECTED = 2;

    private TextView descriptionMac;
    private TextView descriptionServices;
    private TextView descriptionCharacteristic;
    private Button buttonReadCharacteristic;

    UUID SERVICE_UUID = convertFromInteger(0xaaa0);

    public UUID convertFromInteger(int i) {
        final long MSB = 0x0000000000001000L;
        final long LSB = 0x800000805f9b34fbL;
        long value = i & 0xFFFFFFFF;
        return new UUID(MSB | (value << 32), LSB);
    }

    public final BluetoothGattCallback mGattCallBack = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            super.onConnectionStateChange(gatt, status, newState);

            switch (newState){
                case STATE_CONNECTED:
                    Log.i("gattCallback", "STATE_CONNECTED");
                    gatt.discoverServices();
                    break;
                case STATE_DISCONNECTED:
                    Log.e("gattCallback", "STATE_DISCONNECTED");
                    break;
                case STATE_CONNECTING:
                    Log.i("gattCallback", "STATE_CONNECTING");
                    break;
                default:
                    Log.e("gattCallback", "STATE_OTHER");
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            super.onServicesDiscovered(gatt, status);

            final List<BluetoothGattService> services = gatt.getServices();

            for (int i = 0; i < services.size(); i++) {
                final int localIndex = i;
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        descriptionServices.append(services.get(localIndex).getUuid().toString() + "\n");
                    }
                });
            }
            buttonReadCharacteristic.setVisibility(View.VISIBLE);
        }

        @Override
        public void onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
            super.onCharacteristicRead(gatt, characteristic, status);
            Log.i("gattCallback", "CHARACTERISTIC_READ");

        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
            super.onCharacteristicChanged(gatt, characteristic);
        }

        @Override
        public void onCharacteristicWrite(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
            super.onCharacteristicWrite(gatt, characteristic, status);
        }
    };

    @Override
    protected void onPause() {
        super.onPause();
        BtGatt.disconnect();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_gatt);

        BluetoothDevice device = getIntent().getExtras().getParcelable("Bluetooth_Device");

        descriptionMac = (TextView) findViewById(R.id.device_description_mac);
        descriptionServices = (TextView) findViewById(R.id.device_description_services);
        descriptionCharacteristic = (TextView) findViewById(R.id.device_description_characteristics);
        buttonReadCharacteristic = (Button) findViewById(R.id.readCharacteristicsButton);
        buttonReadCharacteristic.setVisibility(View.INVISIBLE);

        descriptionMac.setText(device.getAddress());
        descriptionServices.setText("");
        descriptionCharacteristic.setText("");

        connect(device);
    }

    public void connect(BluetoothDevice device) {
        if (BtGatt == null) {
            BtGatt = device.connectGatt(GATTActivity.this, false, mGattCallBack);
        }
    }

    public void onClickReadCharacteristic(View button) {
        final List<BluetoothGattCharacteristic> characteristics = BtGatt.getService(SERVICE_UUID).getCharacteristics();

        descriptionServices.append("Reading Characteristics: \n");
        for (int i = 0; i < characteristics.size(); i++) {
            final int localIndex = i;
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    descriptionServices.append(characteristics.get(localIndex).getUuid().toString() + "\n");
                }
            });
        }
    }

    /*private void displayGattServices(List<BluetoothGattService> gattServices){
        if (gattServices == null) return;

        String uuid = null;

        String unknownServiceString = getResources().getString(R.string.unknown_service);
        String unknownCharaString = getResources().getString(R.string.unknown_characteristic);

        ArrayList<HashMap<String, String>> gattServiceData = new ArrayList<HashMap<String, String>>();
        ArrayList<HashMap<String, String>> gattCharacteristicData = new ArrayList<HashMap<String, String>>();

        mGattCharacteristics =
                new ArrayList<ArrayList<BluetoothGattCharacteristic>>();

        // Loops through available GATT Services.
        for (BluetoothGattService gattService : gattServices) {
            HashMap<String, String> currentServiceData =
                    new HashMap<String, String>();
            uuid = gattService.getUuid().toString();
            currentServiceData.put(
                    LIST_NAME, SampleGattAttributes.
                            lookup(uuid, unknownServiceString));
            currentServiceData.put(LIST_UUID, uuid);
            gattServiceData.add(currentServiceData);

            ArrayList<HashMap<String, String>> gattCharacteristicGroupData =
                    new ArrayList<HashMap<String, String>>();
            List<BluetoothGattCharacteristic> gattCharacteristics =
                    gattService.getCharacteristics();
            ArrayList<BluetoothGattCharacteristic> charas =
                    new ArrayList<BluetoothGattCharacteristic>();
            // Loops through available Characteristics.
            for (BluetoothGattCharacteristic gattCharacteristic :
                    gattCharacteristics) {
                charas.add(gattCharacteristic);
                HashMap<String, String> currentCharaData =
                        new HashMap<String, String>();
                uuid = gattCharacteristic.getUuid().toString();
                currentCharaData.put(
                        LIST_NAME, SampleGattAttributes.lookup(uuid,
                                unknownCharaString));
                currentCharaData.put(LIST_UUID, uuid);
                gattCharacteristicGroupData.add(currentCharaData);
            }
            mGattCharacteristics.add(charas);
            gattCharacteristicData.add(gattCharacteristicGroupData);
    }*/
}
