# Saba

OpenGL の Viewer です。

一応、 Windows 、 Mac 、 Linux で動きます。

対応ファイルは以下です。

* OBJ
* PMD
* PMX
* VMD

## ビルド方法

ビルドには CMake を使用します。

external ディレクトリに必要なライブラリはまとまっていますが、以下のライブラリは事前に用意してください。

* bullet [bullet](http://bulletphysics.org/wordpress/)

### Bullet の準備 (Windows のみ)

Windows の場合、 Bullet のビルドをビルドする際、以下のように行ってください。

```
cmake -G "Visual Studio 14 2015 Win64" ^
    -D CMAKE_INSTALL_PREFIX=<bullet のインストールディレクトリ> ^
    -D INSTALL_LIBS=ON ^
    -D USE_MSVC_RUNTIME_LIBRARY_DLL=On ^
    -D BUILD_CPU_DEMOS=Off ^
    -D BUILD_OPENGL3_DEMOS=Off ^
    -D BUILD_BULLET2_DEMOS=Off ^
    ..

cmake --build . --config Debug --target ALL_BUILD
cmake --build . --config Debug --target INSTALL
cmake --build . --config Release --target ALL_BUILD
cmake --build . --config Release --target INSTALL
```

-G "Visual Studio 14 2015 Win64" の部分は、環境により適宜変更してください。

### CMake の実行

#### Window

```
mkdir build
cd build
cmake -G "Visual Studio 14 2015 Win64" ^
    -D SABA_BULLET_ROOT=<bullet のインストールディレクトリ> ^
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

## 初期化設定

起動時のカレントディレクトリに"init.json"ファイルを配置することにより初期化時の設定を行うことができます。

```
{
    "MSAAEnable":	true,
    "MSAACount":	8
}

```

### MSAAEnable

MSAA を有効にします。

### MSAACount

MSAA のサンプリング数を設定します。
MSAAEnable が true の場合のみ有効です。

## 操作方法

起動すると、以下の画面が表示されます。

![saba_viewer](./images/saba_viewer.png)

見たいモデルをドラッグアンドドロップしてください。
MMD の場合は、モデル (PMD、PMX) を読み込んだ後、アニメーション (VMD) を読み込ませると、アニメーションできます。

### カメラ

マウスをドラッグすることにより、カメラを操作できます。
トラックパッド等を使用時は、キーボードのキーを組み合わせて動作させることもできます。

* 左ボタン (z + 左ボタン): 回転
* 右ボタン (c + 左ボタン): 遠近
* 中ボタン (x + 左ボタン): 移動

### コマンド

Linux ではドラッグアンドドロップに対応していない環境もあるため、コマンドを用意してあります。

#### open

ファイルを開きます。
動作はドラックアンドドロップと変わりません。
