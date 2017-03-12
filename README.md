# Saba

* MMD を再生できるライブラリ
* 簡易 OpenGL Viewer

![saba_viewer](./images/saba_viewer_01.png)

© 2017 Pronama LLC

## 対応環境

* Window
* Linux
* Mac

## 対応ファイル

* OBJ
* PMD
* PMX
* VMD

## ビルド方法

ビルドには CMake を使用します。
事前にインストールしてください。

### 必要なライブラリ

external ディレクトリに必要なライブラリはまとまっていますが、以下のライブラリは事前に用意してください。

* OpenGL
* [Bullet Physics](http://bulletphysics.org/wordpress/)
* [GLFW](http://www.glfw.org/)

### Bullet Physics の準備 (Windows)

Bullet Physics をビルドする際、以下の設定を参考にしてください。

```
cmake -G "Visual Studio 14 2015 Win64" ^
    -D CMAKE_INSTALL_PREFIX=<bullet のインストールディレクトリ> ^
    -D INSTALL_LIBS=ON ^
    -D USE_MSVC_RUNTIME_LIBRARY_DLL=On ^
    -D BUILD_CPU_DEMOS=Off ^
    -D BUILD_OPENGL3_DEMOS=Off ^
    -D BUILD_BULLET2_DEMOS=Off ^
    -D BUILD_UNIT_TESTS=Off ^
    ..

cmake --build . --config Debug --target ALL_BUILD
cmake --build . --config Debug --target INSTALL
cmake --build . --config Release --target ALL_BUILD
cmake --build . --config Release --target INSTALL
```

`-G "Visual Studio 14 2015 Win64"` の部分は、環境により適宜変更してください。

### Bullet Physics の準備 (Mac / Linux)

Mac であれば `brew` 、Linux であれば `apt-get` 、 `yum` 等でインストールしてください。

### ソースコードのクローン

いくつかのライブラリは submodule となっています。
事前にサブモジュールの更新を行ってください。

```
git clone https://github.com/benikabocha/saba.git
cd saba
```

### CMake の実行

#### Window

```
mkdir build
cd build
cmake -G "Visual Studio 14 2015 Win64" ^
    -D SABA_BULLET_ROOT=<bullet のインストールディレクトリ> ^
    -D SABA_GLFW_ROOT=<GLFW のインストールディレクトリ> ^
    ..
```

作成された *.sln ファイルを開き、ビルドしてください。

"saba_viewer" プロジェクトがアプリケーションとなります。

#### Mac / Linux

```
mkdir build
cd build
cmake ..
make -j4
./saba_viewer
```

動作が重い場合は、以下をお試しください。

```
cmake -DCMAKE_BUILD_TYPE=RELEASE ..
make -j4
```

#### GLFW をビルドする場合 (Option)

GLFW をビルドする場合はサブモジュールの更新を行ってください。

```
git clone https://github.com/benikabocha/saba.git
cd saba
git submodule init
git submodule update
mkdir build
cd build
cmake -D SABA_FORCE_GLFW_BUILD ..
```

## 初期化設定

起動時のカレントディレクトリに"init.json"ファイルを配置することにより初期化時の設定を行うことができます。

```
{
    "MSAAEnable":	true,
    "MSAACount":	8,
    "Commands":[
        {
            "Cmd":"open",
            "Args":["test.pmx"]
        },
        {
            "Cmd":"open",
            "Args":["test.vmd"]
        }
    ]
}
```

### MSAAEnable

MSAA を有効にします。

### MSAACount

MSAA のサンプリング数を設定します。
MSAAEnable が true の場合のみ有効です。

### Commands

起動時に実行するコマンドを設定します。

## 操作方法

起動すると、以下の画面が表示されます。

![saba_viewer](./images/saba_viewer_02.png)

見たいモデルをドラッグアンドドロップしてください。
MMD の場合は、モデル (PMD、PMX) を読み込んだ後、アニメーション (VMD) を読み込ませると、アニメーションできます。

### カメラ

マウスをドラッグすることにより、カメラを操作できます。
トラックパッド等を使用時は、キーボードのキーを組み合わせて動作させることもできます。

* 左ボタン (z + 左ボタン): 回転
* 右ボタン (c + 左ボタン): 遠近
* 中ボタン (x + 左ボタン): 移動

### コマンド

saba_viewer で使用できるコマンドです。

#### open

`open <file path>`

ファイルを開きます。
動作はドラックアンドドロップと変わりません。

対応ファイル:

* PMD
* PMX
* VMD  (事前に PMD または PMX を開いてください)
* OBJ

モデルファイルの場合、開いたモデルは選択状態となります。
モデル名は `model_001` のように、 `model_` 後に読み込み順のIDが降られます。

#### select

`select <model name>`

モデルを選択します。

#### clear

`clear [-all]`

モデルをクリアします。

引数無しで呼び出した際、選択中のモデルをクリアします。

`-all` を指定した際、すべてのモデルをクリアします。

#### play

`play [-all]`

モデルを再生します（アニメーションが設定されていれば）。

引数無しで呼び出した際、選択中のモデルを再生します。

`-all` を指定した際、すべてのモデルを再生します。

#### stop

`stop [-all]`

モデルを停止します。

引数なしで呼び出した際、選択中のモデルを停止します。

`-all` を指定した際、すべてのモデルを停止します。

#### translate

`translate x y z`

選択中のモデルを移動します。

#### rotate

`rotate x y z`

選択中のモデルを回転します。

#### scale

`scale x y z`

選択中のモデルをスケールします。
