package com.example.ly

import android.Manifest
import android.annotation.SuppressLint
import android.bluetooth.BluetoothManager
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat

class MainActivity : AppCompatActivity() {
    private lateinit var noteText: TextView
    private val REQUEST_BLE_CODE = 1001 // 权限请求码

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        noteText = findViewById(R.id.note_text)

        // 检查权限
        checkBlePermissions()
    }

    // 核心：检查并申请权限
    private fun checkBlePermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            // Android 12+ 需要动态申请两个新权限
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

        // 权限有了，开始扫描
        startBleScan()
    }

    // 权限申请结果回调
    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == REQUEST_BLE_CODE) {
            if (grantResults.all { it == PackageManager.PERMISSION_GRANTED }) {
                startBleScan() // 权限全部通过，开始扫描
            } else {
                noteText.text = "权限被拒绝，无法扫描"
            }
        }
    }

    // 蓝牙扫描逻辑
    @SuppressLint("MissingPermission")
    private fun startBleScan() {
        noteText.text = "正在扫描树莓派..."

        val bluetoothManager = getSystemService(BLUETOOTH_SERVICE) as BluetoothManager
        val bluetoothAdapter = bluetoothManager.adapter

        // 判空：如果蓝牙未开启或不可用，显示提示
        if (bluetoothAdapter == null || !bluetoothAdapter.isEnabled) {
            noteText.text = "请开启蓝牙开关"
            return
        }

        val scanner = bluetoothAdapter.bluetoothLeScanner

        // 开始扫描（只扫描我们指定的设备 ID：0x2211）
        scanner.startScan(object : ScanCallback() {
            override fun onScanResult(callbackType: Int, result: ScanResult) {
                super.onScanResult(callbackType, result)

                val manuData = result.scanRecord?.manufacturerSpecificData
                val targetId = 0x2211 // 对应树莓派代码的 ID

                // 找到对应数据
                manuData?.get(targetId)?.let { bytes ->
                    val note = String(bytes).trim()
                    // 显示逻辑
                    val displayText = if (note.isEmpty() || note == "None") "Silent" else note

                    // 必须在主线程更新 UI
                    runOnUiThread { noteText.text = displayText }
                }
            }
        })
    }
}