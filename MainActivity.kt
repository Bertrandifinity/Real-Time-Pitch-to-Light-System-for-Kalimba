package com.example.ly

import android.Manifest
import android.annotation.SuppressLint
import android.bluetooth.BluetoothManager
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat

class MainActivity : AppCompatActivity() {
    private lateinit var noteText: TextView
    private val REQUEST_BLE_CODE = 1001

    // Cache the previously displayed content to avoid repeated UI refreshes
    private var lastNote = ""

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        noteText = findViewById(R.id.note_text)
        checkBlePermissions()
    }

    private fun checkBlePermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED ||
                ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED
            ) {
                ActivityCompat.requestPermissions(
                    this,
                    arrayOf(Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT),
                    REQUEST_BLE_CODE
                )
                return
            }
        }
        startBleScan()
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == REQUEST_BLE_CODE) {
            if (grantResults.all { it == PackageManager.PERMISSION_GRANTED }) {
                startBleScan()
            } else {
                noteText.text = "Permission was denied"
            }
        }
    }

    @SuppressLint("MissingPermission")
    private fun startBleScan() {
        noteText.text = "Scanning for Bluetooth…"

        val bluetoothManager = getSystemService(BLUETOOTH_SERVICE) as BluetoothManager
        val adapter = bluetoothManager.adapter

        if (adapter == null || !adapter.isEnabled) {
            noteText.text = "Please turn on the Bluetooth"
            return
        }

        val scanner = adapter.bluetoothLeScanner

        val scanSettings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)      
            .setReportDelay(0)                                     
            .setCallbackType(ScanSettings.CALLBACK_TYPE_ALL_MATCHES)
            .build()

        scanner.startScan(null, scanSettings, object : ScanCallback() {
            override fun onScanResult(callbackType: Int, result: ScanResult) {
                super.onScanResult(callbackType, result)

                val manuData = result.scanRecord?.manufacturerSpecificData
                val data = manuData?.get(0x0100) ?: return

                val noteStr = String(data, 0, 2)

                if (noteStr == lastNote) return
                lastNote = noteStr

                runOnUiThread {
                    noteText.text = when (noteStr) {
                        "--", "SL" -> "Silent"
                        else -> noteStr
                    }
                }
            }
        })
    }
}
