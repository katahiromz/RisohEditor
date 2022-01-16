(Japanese UTF-8)

# リソーエディタの構築方法

## 構築に必要なもの

- Visual Studio 2019 (with Visual C++)
- CMake
- Inno Setup
- MSYS2

このソリューションは、C++11以降が必須である。
ATLサポートのためには、Visual C++が必須である。
MSYS2でもビルドできるが、その場合、XPでは実行できなくなってしまう。

Inno Setupはインストーラ作成のときに使う。

## 構築の前に

ダウンロードしたリソースファイルの `src/RisohEditor_res.rc` はUTF-8でエンコードされている。
Visual Studio 2019 でUTF-8でエンコードされたリソースファイルをコンパイルすると、
特定の文字列でごみ文字が発生する不具合が確認されている。
よって、構築の前にUTF-16で上書きしないといけない。

ビルド済みのリソーエディタを使って、`src/RisohEditor_res.rc` を開き、
「名前を付けて保存」で同名で保存する。ただし、保存のときに表示される「保存オプション」で
一番下の項目「RC ファイルを UTF-16 で出力する (非推奨)」にチェックを入れなければいけない。
これにチェックを入れて保存すると、UTF-16で出力できる。

ただし、UTF-16はMSYS2ではビルドできないので、MSYS2でビルドしたい場合はUTF-8に
戻さないといけない。

## 構築方法

0. 3個のREADMEファイルに更新履歴を書く。
1. Visual Studio 2019 x86 Command Prompt で CMake を次のように実行する。

```cmd
cd C:\Users\katahiromz\Documents\DEV\ProjectRisohEditor\RisohEditor
"C:\Program Files\CMake\bin\cmake.exe" -G "Visual Studio 16 2019" -A "Win32" -T v141_xp -DCMAKE_BUILD_TYPE=MinSizeRel -DATL_SUPPORT=ON .
```

しばらく待つと実行が完了して次のようなメッセージが表示される。

```txt
-- Build files have been written to: C:/Users/katahiromz/Documents/DEV/ProjectRisohEditor/RisohEditor
```

この場合、CMakeに成功。

2. CMakeによって出力されたファイル`RisohEditor.sln`をVisual Studio 2019で開く。
3. 「Debug」から「MinSizeRel」に変更する。
4. キーボードのCtrlを押しながらプロジェクトをすべてクリックしてプロジェクトをすべて選択する。
5. 選択されているプロジェクトアイコンを右クリックして「プロパティ」を選ぶ。「プロパティ ページ」が表示される。
6. 「プロパティ ページ」の「構成プロパティ」をクリックして、「全般」をクリックして、
「プラットフォーム ツールセット」を「Visual Studio 2017 - Windows XP (v141_xp)」に変更する。
7. 「プロパティ ページ」の「OK」をクリックする。
8. プロジェクト「ALL_BUILD」のみを選択して、「ALL_BUILD」を右クリックして「リビルド」を選択する。
ソリューションがビルドされる。
9. 警告やエラーが表示されたら、ソースコードを修正してやり直し。
10. すべて正常終了したら、次のように簡単に実行テストを行う。プロジェクト「RisohEditor」を右クリックして
「スタートアップ プロジェクトに設定」をクリックする。「デバッグ」メニューの「デバッグ開始」を選ぶ。
デバッグが開始されるので簡単にテストを行う。
11. テストが完了したら、Visual Studio を閉じる。
12. ビルドによって`build`というフォルダが作成された。
さらに`build`フォルダの中に`MinSizeRel`というフォルダが作成されている。
`MinSizeRel`の中身を`build`フォルダに貼り付ける。
13. Inno Setupで次のようにインストーラを作成する。
Inno Setupでファイル`installer.iss`もしくは`installer-the-world.iss`を開き、
メニューから`Build`→`Compile`を選ぶ。
14. しばらく待つと、インストーラのファイル`RisohEditor-x.x.x.exe`が作成される。
これをどこかのリリース用のフォルダに貼り付ける。
15. ZIPファイル `RisohEditor-x.x.x.zip`を作成するために、
MSYS2で次のようにシェルスクリプトファイル`pack.sh`を実行する。
```bash
$ ./pack.sh
```
しばらく待って次のようなメッセージが表示されたら成功。
```txt
Success. build/RisohEditor-x.x.x.zip was generated.
```
`build`フォルダの中にファイル`RisohEditor-x.x.x.zip`が作成されている。
これをどこかのリリース用のフォルダに貼り付ける。

これで構築完了。
