(Japanese UTF-8)

# リソーエディタの構築方法

## 構築に必要なもの

- Visual Studio 2012
- CMake (GUI)
- Inno Setup
- MSYS2

## 構築方法

0. 3個のREADMEファイルに更新履歴を書く。
1. Visual Studio 2012 x86 Command Prompt で CMake を次のように実行する。

```cmd
cd C:\Users\katahiromz\Documents\DEV\ProjectRisohEditor\RisohEditor
"C:\Program Files\CMake\bin\cmake.exe" -G
"C:\Program Files\CMake\bin\cmake.exe" -G "Visual Studio 11 2012" -DCMAKE_BUILD_TYPE=Release .
```

しばらく待つと実行が完了して次のようなメッセージが表示される。

```txt
-- Build files have been written to: C:/Users/katahiromz/Documents/DEV/ProjectRisohEditor/RisohEditor
```

この場合、CMakeに成功。

2. CMakeによって出力されたファイル`RisohEditor.sln`をVisual Studio 2012で開く。
3. 「Debug」から「Release」に変更する。
4. キーボードのCtrlを押しながらプロジェクトをすべてクリックしてプロジェクトをすべて選択する。
5. 選択されているプロジェクトアイコンを右クリックして「プロパティ」を選ぶ。「プロパティ ページ」が表示される。
6. 「プロパティ ページ」の「構成プロパティ」をクリックして、「全般」をクリックして、
「プラットフォーム ツールセット」を「Visual Studio 2012 (v110)」から「Visual Studio 2012 - Windows XP (v110_xp)」に
変更する。
7. 「プロパティ ページ」の「OK」をクリックする。
8. プロジェクト「ALL_BUILD」のみを選択して、「ALL_BUILD」を右クリックして「リビルド」を選択する。
ソリューションがビルドされる。
9. 警告やエラーが表示されたら、ソースコードを修正してやり直し。
10. すべて正常終了したら、次のように簡単に実行テストを行う。プロジェクト「RisohEditor」を右クリックして
「スタートアップ プロジェクトに設定」をクリックする。「デバッグ」メニューの「デバッグ開始」を選ぶ。
デバッグが開始されるので簡単にテストを行う。
11. テストが完了したら、Visual Studio を閉じる。
12. ビルドによって`build`というフォルダが作成された。
さらに`build`フォルダの中に`Release`というフォルダが作成されている。
`Release`の中身を`build`フォルダに貼り付ける。
13. Inno Setupで次のようにインストーラを作成する。
Inno Setupでファイル`installer.iss`もしくは`installer-the-world.iss`を開き、
メニューから`Build`→`Compile`を選ぶ。
14. しばらく待つと、インストーラのファイル`RisohEditor-x.x.x.exe`が作成される。
これをどこかのリリース用のフォルダに貼り付ける。
15. MSYS2で次のようにシェルスクリプトファイル`pack.sh`を実行する。
```bash
$ ./pack.sh
```
しばらく待って次のようなメッセージが表示されたら成功。
```txt
Success. build/RisohEditor-x.x.x.zip was generated.
```
`build`フォルダの中にファイル`RisohEditor-x.x.x.zip`が作成されている。
これをどこかのリリース用のフォルダに貼り付ける。

16. リリース用のフォルダに移動して再度、簡単にテストを行う。失敗したらやり直し。

17. GitHub・Vector・Softpedia・ホームページでリリースする。
