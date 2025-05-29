# Midi2XInput

MIDI キーボードのノート情報を **Xbox/XInput 互換 BLE HID** に変換する  
M5AtomS3 Lite（ESP32-S3）向け Arduino スケッチです。  
MIDI ノート 48–72 の 25 鍵を 2 通りのレイアウト(※)で Xbox コントローラーにマッピングし、  
Velocity に応じて本体の Neopixel を発光させます。

※「Sky 星を紡ぐ子どもたち」の演奏時に使用するコントローラー配置

---

## 特長

* USB-MIDI (USB Host) → Xbox BLE HID をリアルタイム変換  
* 物理ボタン（短押し）で **マッピング 1 / 2** を即時切替  
* 物理ボタン（長押し）で **RS ボタン**を送信（ゲーム内のキー配置を変更）  
* Velocity を色相＋輝度に変換して LED 表示  
* D-Pad（十字キー）の斜め／同時入力には対応していますが、上下／左右といった逆方向の同時入力には対応していません。  
* アナログスティックは斜めも含めて同時入力には対応していません。  

---

## ハードウェア要件

| パーツ | 役割 | 備考 |
| ------ | ---- | ---- |
| **M5AtomS3 Lite** | 本体（ESP32-S3） | Grove/USB-C 端子付き |
| **USB Host ハブ** | 本体 & MIDI キーボード給電 | セルフパワー推奨 |
| **MIDI キーボード** | 任意の USB-MIDI Class 準拠機器 | 25 鍵以上 |
| **給電用 5V 電源** | USB Host ハブ用 | 1A 以上推奨 |

> **注意**  
> ATOMS3／ATOMS3 Lite は USB-C 端子を1つだけ備えており、通常はここから電源を供給します。   
> そのため、USB-C に MIDI キーボードなどを直接接続すると、本体に電源を供給できなくなります。   
> USB Host 機能を使用する場合は、セルフパワー式の USB Host ハブ、または Grove 端子や 5V ピンから本体に電源を供給してください。   
>    
> ATOMS3 の基板設計上、USB-C 端子からは VBUS（5V）が出力されないため、USB 機器には電力が供給されません。
> USB 機器への電力供給のためにも、セルフパワー式 USB Host ハブを用いて電源を供給するすることを推奨します。   
> ただし、接続する機器の消費電力が極めて小さい場合は、Grove 端子や 5V ピンからの電源供給時に USB 経由の微弱な漏れ電流で動作することもあります。
---

## ソフトウェア要件

* Arduino IDE 2.3 以降
* Espressif **esp32**   ボードパッケージ 3.2 以降  
* M5Stack   **M5Stack** ボードパッケージ 3.2 以降  
* ライブラリ  
  | ライブラリ | バージョン | ライセンス |
  | ---------- | ---------- | ---------- |
  | [M5AtomS3](https://github.com/m5stack/M5AtomS3) | 1.0.2 | MIT |
  | [ESP32-BLE-CompositeHID](https://github.com/Mystfit/ESP32-BLE-CompositeHID) | 0.3.1 | MIT |
  | [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) | 2.3.0 | Apache-2.0 |
  | [Callback](https://github.com/tomstewart89/Callback) | 1.1 | MIT |
  | [ESP32_Host_MIDI](https://github.com/sauloverissimo/ESP32_Host_MIDI) | 1.0.3 | MIT |

・ESP32-BLE-CompositeHID はリポジトリから ZIP をダウンロードして   
  Arduino IDE の「Sketch> Include Library> Add .ZIP Library」でライブラリにインストールしてください。   
・ESP32_Host_MIDI は`USB_Conexion`のみ使用しているのでスケッチフォルダに同梱しています。   
  ※ライブラリ全体をインストールするとBLE 関連（NimBLE）が二重定義となりビルドできなくなります。   
・上記以外のライブラリはライブラリマネージャからインストール可能です。   

---

## ビルド & 書き込み手順

1. このリポジトリをクローン / ZIP ダウンロードして展開  
2. `Midi2XInput.ino` を Arduino IDE で開く  
3. 上記依存ライブラリをインストール  
4. 「File> Preferences> Additional boards manager URLs:」に以下のURLを追加する   
   https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json
5. 「tools> Board> M5Stack> M5AtomS3」を選択  
6. ポートを選択（PC 環境によりポート番号は変わります）
7. 「USB CDC On Boot: **"Disabled"**」を選択
8. 「USB Mode: **"USB-OTG (TinyUSB)"**」を選択
9. **書き込み**  

> **注意**  
> USB-OTG モードで書き込みした後は、そのままでは書き込みできなくなります。   
> ATOMS3 Lite のリセットボタンを 2 秒以上押すと LED が緑色に一瞬だけ光り書き込みできるようになります。   


> ### USB Host とシリアルモニタについて
>
> AtomS3 Lite の USB-C 端子は通常 **CDC-ACM (Serial)** として PC に認識されますが、  
> **USB Host モードで MIDI キーボードを接続している間はシリアル機能が無効** になります。  
> `Serial.print()` の出力は PC 側の Arduino シリアルモニタには届きません。
>
> #### 代替デバッグ手段
>
> | 方法 | メリット | 概要 |
> |------|----------|------|
> | **Wi-Fi ログ転送** | BLE と干渉しない／ケーブルレス | `WiFi.begin()` → UDP/TCP/HTTP でログを送出し、PC のターミナルや syslog で受信<br>例：`WiFiUdp udp; udp.beginPacket(host, port); udp.printf(...); udp.endPacket();` |
>

---

## 使い方

1. AtomS3 Lite と USB-MIDI キーボードをセルフパワー Host HUB で接続  
2. AtomS3 Lite の BLE アドバタイズ名 **"Midi2XInput"** をホスト機器（iPhone/iPad/Android など）が検出  
   ※ホスト機器によってはアドバタイズ名ではなく AtomS3 Lite の MAC アドレスが表示されることがあります。  
     また、そもそも Bluetooth の一覧に表示されない場合は `Midi2XInput.ino` のBLEアドバタイズデータ送信部分をコメントアウトしてみてください。  
3. ペアリング完了後、キーボードのノート ON/OFF が Xbox ゲームパッド入力として送信  
4. AtomS3 Lite 本体の物理ボタン   
   * **短押し** … マッピング 1 ↔ 2 切替  
   * **長押し (≥1 s)** … Xbox **RS** ボタンを押しっぱなし
5. LED の色 = ノート番号、明るさ = Velocity

### マッピング一覧

| ノート | **マッピング 1** | **マッピング 2** |
|------:|-----------------|-----------------|
| 48 | LT | D-Pad ↓ |
| 49 | — | — |
| 50 | RT | D-Pad ← |
| 51 | — | — |
| 52 | D-Pad ↓ | D-Pad ↑ |
| 53 | A | L-Stick ↓ |
| 54 | — | — |
| 55 | D-Pad ← | L-Stick ← |
| 56 | — | — |
| 57 | X | LB |
| 58 | — | — |
| 59 | D-Pad ↑ | LT |
| 60 | Y | R-Stick ↓ |
| 61 | — | — |
| 62 | D-Pad → | R-Stick → |
| 63 | — | — |
| 64 | B | R-Stick ↑ |
| 65 | LB | A |
| 66 | — | — |
| 67 | RB | B |
| 68 | — | — |
| 69 | L-Stick ← | Y |
| 70 | — | — |
| 71 | R-Stick ← | RB |
| 72 | L-Stick → | RT |

---

## ライセンス

* **Midi2XInput (本コード)** — MIT  
* **USB_Conexion (ESP32_Host_MIDI 部分)** — MIT   
* そのほか M5AtomS3 / ESP32-BLE-CompositeHID (MIT) / NimBLE-Arduino (Apache-2.0) / Callback (MIT) については各リポジトリを参照。  
  まとめは [`THIRD_PARTY_LICENSES.md`](./THIRD_PARTY_LICENSES.md) に記載。

> **「Sky 星を紡ぐ子どもたち」（英語名：Sky Children of the Light）** は thatgamecompany の米国およびその他の国における登録商標です。  
> 本プロジェクトは thatgamecompany とは一切関係ありません。

> **Xbox®** は Microsoft Corporation の米国およびその他の国における登録商標です。  
> 本プロジェクトは Microsoft とは一切関係ありません。

