# asc バージョン 0.16 ＆ aic バージョン 0.02

## 概要

ビジュアルアーツ系のゲームで使われているAVG32のシナリオデータをNScripter用に変換するツール群です。
変換元となるファイルはAVG32で動作するゲームがインストールされたフォルダにあるGAMEEXE.INIおよび同フォルダ内のDATフォルダにあるSEEN.TXTです。
SEEN.TXTはseentoolsもしくはAVG32シナリオファイル解凍・分割ツールでSEEN001.TXT、SEEN002.TXT…SEEN999.TXTのように分割しておいてください。
尚、asc及びaicはGCC 3.3.3のWindows用パッケージ「MinGW日本語版」のmingw-jp-20040224を使ってコンパイルしています。

NScripter  
http://www.nscripter.com/

seentools 0.0.1 for Windows  
http://d.hatena.ne.jp/nyanonon/20070808#p1

AVG32シナリオファイル解凍・分割ツール Vadecode.lzh  
http://hp.vector.co.jp/authors/VA028184/

## 変換方法

GAMEEXE.INI及びSEEN.TXTを分割したファイル、aic.exe、asc.exeを同じフォルダに用意して、下記のように実行するとNScripter用シナリオデータが0.txtとして出力されます。

aic > 0.txt
asc SEEN001.TXT >> 0.txt
asc SEEN002.TXT >> 0.txt
          :
asc SEEN999.TXT >> 0.txt

SEEN.TXTを分割したファイル（SEEN001.TXT、SEEN002.TXT…SEEN999.TXT）は変換したいものだけを任意に指定してもかまいません。

## 使用方法

NScripter用に変換したシナリオファイル 0.txtと同じフォルダに画像データや音楽データを用意してください。
基本的に元のゲームがインストールされていたフォルダから画像や音楽などのファイルを含むフォルダごとコピーするだけですが、KOEファイルやPDTファイルはNScripterで読めませんので変換しておく必要があります。
KOEファイルはavg32toolsのkoeunpac.exeでWAVファイルにしてkoewavフォルダに、PDTファイルはVisualArts系PDT/G00/MCG形式用Susie32プラグイン(透明色対応仕様)でBMPファイルにしてpdtbmpフォルダに格納しておいてください。

avg32tools  
http://www.geocities.co.jp/Playtown-Domino/8282/avg32tools_readme.html

VisualArts系PDT/G00/MCG形式用Susie32プラグイン(透明色対応仕様)  
http://homepage3.nifty.com/hiro-inoue/soft_spi0.html#SOFT_SPIKNN

以上の準備が出来たら、NScripterのプログラムファイル（nscr.exe）を同じフォルダに用意して実行すれば変換したゲームを遊ぶことが出来ます。
必要に応じてNScripterのカーソルファイル（uoncur.bmp、uoffcur.bmp）も同じフォルダに用意すると良いかも知れません。

ちなみに任意のSEENxxx.TXTだけを変換した場合は「ラベル*seen010は存在しません」のようにエラーが出て先に進めない場合があります。
そのような場合は0.txtから「goto *seen010」のようなコマンドの先頭に「;」を挿入してコメント化するか、「end」に置き換えれば良いかと思います。

## 動作仕様

変換できないAVG32の命令や変換が不完全な命令は「;」で始まるコメントとして、16進数や文字列のデータが出力されます。
テキストに含まれる＊ＡＢなどは、GAMEEXE.INIの#NAME.ABなどを元に置き換えるべきですが、今回は何も行っていません。

・NScripter について

C言語などでお馴染みの Hello World. は下記のようにテキストファイル 0.txt を作成して、同じフォルダに配置した nscr.exe を実行するだけで表示されます。

*define
game

*start
ハローワールド\
end

*define と続く game は必須。game で *start に飛ぶので、*start も必須。end で NScripter を自動的に終了させられる。
ハローワールドの末尾の「\」は、いわゆるクリック待ち。続く表示で改ページさせたくない場合は「@」を使う。空行は「br」を記述。
画像は「bg "ファイル名",1」で表示。末尾の1は即時表示の意味。
BGMは「bgm "ファイル名"」で演奏開始。「stop」で演奏終了。
音声は「dwave 0,"ファイル名"」で再生。最初の0はチャンネル番号。
音声に重ねて効果音を鳴らしたい場合はチャンネル番号をずらす。
http://www.nekomimist.org/d/200102.html#d26_t3_3
> …AVG32システムの開発元であるところのフェアザンメルンの…
